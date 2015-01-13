//
// File: Bloque.h
//

#ifndef _FRED_BLOQUE_H
#define _FRED_BLOQUE_H

/*
 * A generic, vector-like container class that:
 *  - is implementated like a deque, hence the name
 *  - uses a vector of dynamic arrays as underlying container
 *  - never invalidates iterators/pointers/references as new blocks are allocated
 *  - never shrinks; doesn't allow deletion of individual elements
 *  - reclaims items marked for recycling (instead of deleting)
 *  - supports additional arbitrary bitmasks to control iteration
 *  - thread-safe
 *  - can traverse container and apply an arbitrary functor to each (possibly in parallel)
 */

#include <deque>
#include <map>
#include <utility>
#include <vector>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <cstdarg>
#include <stdint.h>
#include <assert.h>

/*
 * use SSE2 (8 128 XMM registers)
 */

#define numRegisters 16
#define registerWidth 64
#define registerBanksPerBlock 16

// registersPerBlock = numRegisters * registerBanksPerBlock
#define registersPerBlock 256
// bitsPerBlock = registersPerBlock * registerWidth
#define bitsPerBlock 16384 

typedef uint64_t BitType;

template < class ObjectType, class MaskType >
class bloque {

/*
 * private data members
 *
 */

private:

  size_t blockSize;
  size_t numItems;
  size_t endIndex;
  // blockVector stores the items
  std::deque< ObjectType * > blockVector;
  // each mask provides a bitset for the items in the container
  typedef std::deque< BitType * > mask;
  // default mask controls insertion/deletion in the container
  mask defaultMask;
  typedef std::map< MaskType, mask > MaskMap; 
  typedef typename MaskMap::iterator MaskMapItr;


  struct Range {
    int begin_index, end_index;
    int begin_block, end_block;
    int begin_slot, end_slot;
    int begin_register, end_register;
    int begin_bit, end_bit;
 
    std::vector< std::pair< int, int > > range_ij;
   
    Range() {
      begin_index = end_index = -1;
      begin_block = end_block = -1;
      begin_slot = end_slot = -1;
      begin_register = end_register = -1;
      begin_bit = end_bit = -1;
    }
    Range( int _begin_index, int _end_index ):
      begin_index( _begin_index ), end_index ( _end_index ) {
        
        begin_block = begin_index / bitsPerBlock;
        begin_slot = begin_index % bitsPerBlock;
        begin_register = begin_slot / registerWidth;
        begin_bit = begin_slot % registerWidth; 

        end_block = end_index / bitsPerBlock;
        end_slot = end_index % bitsPerBlock;
        end_register = end_slot / registerWidth;
        end_bit = end_slot % registerWidth; 

        for ( int i = begin_block; i < end_block + 1; ++i ) {
      
          int j0 = 0;
          int jN = registersPerBlock;

          if ( i == begin_block ) {
            j0 = begin_register;
          }
          if ( i == end_block ) {
            jN = end_register + 1;
          }

          for ( int j = j0; j < jN; ++j ) {
            range_ij.push_back( std::pair< int, int >( i, j ) );
          }
        }

    }

    int size() {
      assert( begin_index > 0 && end_index >= begin_index );
      return end_index - begin_index + 1;
    }
    bool contains( int index ) {
      return index >= begin_index && index <= end_index;
    }
  };

  // character labels for ranges of indexes
  std::map< char, Range > labeled_ranges;

  MaskMap userMasks;
  std::map< MaskType, int > user_mask_num_items;

  /*
   * itemPosition: class to simplify conversions to/from (block,slot) coordinates
   * and integer index
   */
  struct itemPosition {
    size_t block, slot;
    
    itemPosition() {
      block = 0;
      slot = 0;
    }

    itemPosition( size_t itemIndex ) {
      block = itemIndex / bitsPerBlock;
      slot = itemIndex % bitsPerBlock;
    }

    itemPosition & operator++ () {
      ++slot;
      if ( slot >= bitsPerBlock ) {
        slot = 0;
        ++block;
      }
      return *this;
    }

    bool operator== ( itemPosition const &other ) const {
      return slot == other.slot && block == other.block;
    }
    
    bool operator!= ( itemPosition const &other ) const {
      return slot != other.slot || block != other.block;
    }

    bool operator< ( itemPosition const &other ) const {
      return block < other.block || (block == other.block && slot < other.slot);
    }

    bool operator> ( itemPosition const &other ) const {
      return block > other.block || (block == other.block && slot > other.slot);
    }

    bool operator<= ( itemPosition const &other ) const {
      return *this < other || *this == other;
    }

    bool operator>= ( itemPosition const &other ) const {
      return *this > other || *this == other;
    }

    size_t asIndex() const {
      return ( block * bitsPerBlock ) + slot;
    }

    void print() {
      std::cout << block << " " << slot << std::endl;
    }

  };
  
  std::deque< itemPosition > freeSlots;

  itemPosition firstItemPosition, lastItemPosition;
  size_t firstItemIndex, lastItemIndex;

/* **************************************************************************
 * public constructors, get/set methods *************************************
 * **************************************************************************
 */

public:

  bloque() {
    numItems = 0;
    endIndex = 0;
    init();
    addBlock();
  }

  void add_mask( MaskType maskName ) {
    #pragma omp critical(BLOQUE_ADD_MASK)   
    if ( userMasks.find( maskName ) == userMasks.end() ) {
      user_mask_num_items[ maskName ] = 0;
      for ( size_t i = 0; i < blockVector.size(); ++i ) { 
        userMasks[ maskName ].push_back( new BitType[ registersPerBlock ] );
        for ( size_t j = 0; j < registersPerBlock; ++j ) {
          userMasks[ maskName ][ i ][ j ] = 0;    
        }
      }
    }
  }

  void add_labeled_range( char label, int begin, int end ) {
    assert( begin >= 0 );
    assert( end >= begin );
    labeled_ranges[ label ] = Range( begin, end );
  }

  int get_labeled_range_size( char label ) {
    assert( labeled_ranges.find( label ) != labeled_ranges.end() );
    return labeled_ranges[ label ].size();
  }

  int get_free_index() {
    if ( freeSlots.empty() ) {
      addBlock();
    }
    itemPosition freePosition = freeSlots.front();    // <---------------------------------- TODO: protect with mutex!
    if ( numItems == 0 ) {
      firstItemPosition = itemPosition();
      lastItemPosition = itemPosition();
    }
    if ( freePosition < firstItemPosition ) {
      firstItemPosition = freePosition;
    }
    if ( freePosition > lastItemPosition ) {
      lastItemPosition = freePosition;
    }
    freeSlots.pop_front();                            // <---------------------------------- TODO: protect with mutex!
    return freePosition.asIndex();
  }
 
  ObjectType * get_free_pointer( int itemIndex ) {
    assert( itemIndex >= 0 );
    size_t block = itemIndex / bitsPerBlock;
    size_t slot = itemIndex % bitsPerBlock;
    return &( blockVector[ block ][ slot ] );
  }

  ObjectType & operator[] ( int itemIndex ) {
    assert( itemIndex >= 0 );
    size_t block = itemIndex / bitsPerBlock;
    size_t slot = itemIndex % bitsPerBlock;
    return blockVector[ block ][ slot ];
  }

  size_t size() {
    return numItems;
  }

  size_t size( MaskType mask ) {
    return user_mask_num_items[ mask ];
  }

  void mark_valid_by_index( size_t index ) {
    
    itemPosition pos = itemPosition( index );
    setBit( defaultMask[ pos.block ][ pos.slot / registerWidth ], pos.slot % registerWidth );
    
    if ( pos > lastItemPosition ) {
      lastItemPosition = pos;
    }
    if ( pos < firstItemPosition ) {
      firstItemPosition = pos;
    }
    ++numItems;
  }


  /*
   * Marks item as invalid and frees slot for reallocation
   */
  void mark_invalid_by_index( size_t index ) {
    itemPosition pos = itemPosition( index );
    assert( ( defaultMask[ pos.block ][ pos.slot / registerWidth ] ) & ( (BitType) 1 << ( pos.slot % registerWidth ) ) );
    clearBit( defaultMask[ pos.block ][ pos.slot / registerWidth ], pos.slot % registerWidth );
    addSlot( index );
    if ( pos == lastItemPosition ) {
      lastItemPosition = getNextItemPosition( pos );
    }
    if ( pos == firstItemPosition ) {
      firstItemPosition = getNextItemPosition( pos );
    }
    // decrement number of items
    #pragma omp atomic
    --numItems;
    // update any user masks
    for ( MaskMapItr mit = userMasks.begin(); mit != userMasks.end(); ++mit ) {
      if ( ( (*mit).second[ pos.block ][ pos.slot / registerWidth ] ) & ( (BitType) 1 << ( pos.slot % registerWidth ) ) ) {
        // unset the bit for this index in this mask
        clearBit( (*mit).second[ pos.block ][ pos.slot / registerWidth ], ( pos.slot % registerWidth ) );
        // decrement the count for this mask; must be protected from concurrent writes
        #pragma omp atomic
        --( user_mask_num_items[ (*mit).first ] );
      }
    }
  }

  /*
   * Marks item's bit in specified user Mask
   */
  void set_mask_by_index( MaskType mask, size_t index ) {
    itemPosition pos = itemPosition( index );
    // set the bit for this index in this mask
    setBit( userMasks[ mask ][ pos.block ][ pos.slot / registerWidth ], pos.slot % registerWidth );
    // increment the count for this mask; must be protected from concurrent writes
    #pragma omp atomic
    ++( user_mask_num_items[ mask ] );
  }

  /*
   * Unmarks (clears) item's bit in specified user Mask
   */
  void clear_mask_by_index( MaskType mask, size_t index ) {
    itemPosition pos = itemPosition( index );
    // unset the bit for this index in this mask
    clearBit( userMasks[ mask ][ pos.block ][ pos.slot / registerWidth ], pos.slot % registerWidth );
    // decrement the count for this mask; must be protected from concurrent writes
    #pragma omp atomic
    --( user_mask_num_items[ mask ] );
  }
 
  void clear_mask( MaskType m ) {
    mask & userMask = userMasks[ m ]; 
    #pragma omp critical(BLOQUE_CLEAR_MASK) 
    {
      for ( int i = 0; i < blockVector.size(); ++i ) {
        for ( int j = 0; j < registersPerBlock; ++j ) {
          userMask[ i ][ j ] = (BitType) 0;
        }
      }
      user_mask_num_items[ m ] = 0;
    }
  }

  /*
   * Returs true if the bit for the specified index is set in the
   * specified mask (assumes that the index is valid)
   */
  bool mask_is_set( MaskType mask, size_t index ) {
     itemPosition pos = itemPosition( index );
     assert( (       defaultMask[ pos.block ][ pos.slot / registerWidth ] ) & ( (BitType) 1 << ( pos.slot % registerWidth ) ) );
     return  ( userMasks[ mask ][ pos.block ][ pos.slot / registerWidth ] ) & ( (BitType) 1 << ( pos.slot % registerWidth ) );
  }

  /*
   * Generic, sequential 'apply' method for all items in container
   */
  template < typename Functor > 
  void apply( Functor & f ) {
    for ( iterator i = begin(); i != end(); ++i ) {
      f( *i );
    }
  }

  /*
   * Sequentially apply functor for all items in container that have specified mask set
   */
  template < typename Functor > 
  void apply( MaskType mask, Functor & f ) {
    for ( iterator i = begin(); i != end(); ++i ) {
      itemPosition & pos = i.currentItemPosition;
      if ( ( userMasks[ mask ][ pos.block ][ pos.slot / registerWidth ] ) & ( (BitType) 1 << ( pos.slot % registerWidth ) ) ) {
        f( *i );
      }
    }
  }

  /*
   * Generic, parallel 'apply' method for all items in container
   */
  template < typename Functor > 
  void parallel_apply( Functor & f ) {
    #pragma omp parallel for
    for ( int i = 0; i < blockVector.size(); ++i ) {
      for ( int j = 0; j < registersPerBlock; ++j ) {
        if ( ( defaultMask[ i ][ j ] ) > ( (BitType) 0 ) ) {
          for ( int k = 0; k < registerWidth; ++k ) {
            if ( ( defaultMask[ i ][ j ] ) & ( (BitType) 1 << ( k ) ) ) {
              f( blockVector[ i ][ ( j * registerWidth ) + k ] );
            }
          }
        }
      }
    }
  }

  /*
   * Generic, parallel 'masked apply' method for all items in container
   */
  template < typename Functor > 
  void parallel_masked_apply( MaskType m, Functor & f ) {
    mask & userMask = userMasks[ m ]; 
    #pragma omp parallel for
    for ( int i = 0; i < blockVector.size(); ++i ) {
      for ( int j = 0; j < registersPerBlock; ++j ) {
        BitType reg = ( defaultMask[ i ][ j ] ) & ( userMask[ i ][ j ] );  
        if ( reg > ( (BitType) 0 ) ) {
          for ( int k = 0; k < registerWidth; ++k ) {
            if ( ( reg ) & ( (BitType) 1 << ( k ) ) ) {
              f( blockVector[ i ][ ( j * registerWidth ) + k ] );
            }
          }
        }
      }
    }
  }

  template < typename Functor > 
  void parallel_masked_apply_to_labeled_range( char label, MaskType m, Functor & f ) {
    mask & userMask = userMasks[ m ];
    Range r = labeled_ranges[ label ];

    f.number_applied = 0;
    int number_applied = 0;
    #pragma omp parallel for reduction(+:number_applied)
    for ( int i = r.begin_block; i < r.end_block + 1; ++i ) {
      
      int j0 = 0;
      int jN = registersPerBlock;

      if ( i == r.begin_block ) {
        j0 = r.begin_register;
      }
      if ( i == r.end_block ) {
        jN = r.end_register + 1;
      }

      for ( int j = j0; j < jN; ++j ) {

        int k0 = 0;
        int kN = registerWidth;

        if ( i == r.begin_block && j == r.begin_register ) {
          k0 = r.begin_bit;
        }
        if ( i == r.end_block && j == r.end_register ) {
          kN = r.end_bit; // TODO TODO TODO TODO something is not right here ... why not r.end_bit + 1 ????
        }

        BitType reg = ( defaultMask[ i ][ j ] ) & ( userMask[ i ][ j ] );  
        if ( reg > ( (BitType) 0 ) ) {
          for ( int k = k0; k < kN; ++k ) {
            if ( ( reg ) & ( (BitType) 1 << ( k ) ) ) {
              f( blockVector[ i ][ ( j * registerWidth ) + k ] );
              ++( number_applied );
            }
          }
        }
      }
    }
    f.number_applied = number_applied;
  }


  /*
   * Generic, parallel 'masked apply' method for all items in container;
   * passes the thread id to the functor.  This is intended to enable
   * parallel application of the functor to gather num_threads sub-results,
   * which are then subsequently merged to produce final output.
   *
   * Expects that the functor operator() takes two arguments: an object
   * reference and the thread_id
   *
   * Expects that the functor implements two methods: get_max_threads()
   * and get_thread_num().  These should perform the same function as
   * their similarly named omp_get_max_threads() and omp_get_thread_num()
   * counterparts declared in omp.h
   */
  template < typename Functor > 
  void parallel_masked_apply_with_thread_id( MaskType m, Functor & f ) {
    mask & userMask = userMasks[ m ];
    int num_threads = f.get_max_threads();
    #pragma omp parallel
    {
      int thread_id = f.get_thread_num();
      int block_items_per_thread = ( blockVector.size() / num_threads ) + 1 ;
      int begin = thread_id * block_items_per_thread;
      int end = begin + block_items_per_thread;
      end = ( end > (int) blockVector.size() ) ? ( (int) blockVector.size() ) : ( end );
      for ( int i = begin; i < end; ++i ) {
        for ( int j = 0; j < registersPerBlock; ++j ) {
          BitType reg = ( defaultMask[ i ][ j ] ) & ( userMask[ i ][ j ] );  
          if ( reg > ( (BitType) 0 ) ) {
            for ( int k = 0; k < registerWidth; ++k ) {
              if ( ( reg ) & ( (BitType) 1 << ( k ) ) ) {
                f( blockVector[ i ][ ( j * registerWidth ) + k ], thread_id );
              }
            }
          }
        }
      }
    }
  }

  // void parallelIndexedApply( std::vector< int > indices, Functor & f ) ...

  void sortFreeSlots() {
    // <----------------------------------------------------------------------------------- TODO: mutex/lock container!!! 
    std::sort( freeSlots.begin(), freeSlots.end() ); 
  }

  /*
   * returns the bloque position for the next valid item
   */
  itemPosition getNextItemPosition( itemPosition p ) {
    // make this more efficient by bitwise/sse examination of the bitsets (registers)
    for ( itemPosition i = itemPosition( p.asIndex() + 1 ); i <= lastItemPosition; ++i ) {
      if ( ( defaultMask[ i.block ][ i.slot / registerWidth ] ) & ( (BitType) 1 << ( i.slot % registerWidth ) ) ) {
        return itemPosition( i.asIndex() );
      }
    }
    return itemPosition( lastItemPosition.asIndex() + 1 ); // one past the last valid position
  }

  /*
   * returns the bloque position for the next valid item with the specified mask set
   */
  template < typename __MaskType >
  itemPosition getNextItemPosition( itemPosition p ) {
    // make this more efficient by bitwise/sse examination of the bitsets (registers)
    for ( itemPosition i = itemPosition( p.asIndex() + 1 ); i <= lastItemPosition; ++i ) {
      if ( ( defaultMask[ i.block ][ i.slot / registerWidth ] ) & ( (BitType) 1 << ( i.slot % registerWidth ) ) ) {
        return itemPosition( i.asIndex() );
      }
    }
    return itemPosition( lastItemPosition.asIndex() + 1 ); // one past the last valid position
  }

  ObjectType & get_item_by_position( itemPosition & pos ) {
    return blockVector[ pos.block ][ pos.slot ];
  }

  ObjectType & get_item_by_index( size_t itemIndex ) {
    size_t block = itemIndex / bitsPerBlock;
    size_t slot = itemIndex % bitsPerBlock;
    return blockVector[ block ][ slot ];
  }

  //////////// BEGIN ITERATORS ///////////////////////////
  
  /// Basic iterator for bloque (iterates over all valid items in container)

  template< class IteratedObjectType >
  struct bloque_iterator : std::iterator< std::forward_iterator_tag, IteratedObjectType > {

    IteratedObjectType & operator* () { return blq->get_item_by_position( currentItemPosition ); }

    template< class __IteratedObjectType >
    friend bool operator== ( bloque_iterator const &lhs, bloque_iterator< __IteratedObjectType > const &rhs ) {
      return lhs.currentItemPosition == rhs.currentItemPosition;
    }

    template< class __IteratedObjectType >
    friend bool operator!= ( bloque_iterator const &lhs, bloque_iterator< __IteratedObjectType > const &rhs ) {
      return lhs.currentItemPosition != rhs.currentItemPosition;
    }

    bloque_iterator & operator++ () {
      currentItemPosition = blq->getNextItemPosition( currentItemPosition );
      return *this;
    }

    template< class __IteratedObjectType >
    bool operator< ( bloque_iterator< __IteratedObjectType > const & other ) {
      return currentItemPosition < other.currentItemPosition;
    }

    //private:

    itemPosition currentItemPosition;
    bloque * blq;

    // private constructor for begin, end
    friend class bloque;
    bloque_iterator( bloque * __bloque, itemPosition __currentItemPosition ) {
      blq = __bloque;
      currentItemPosition = __currentItemPosition;
    }
  };
 
  typedef bloque_iterator< ObjectType > iterator;
  typedef bloque_iterator< ObjectType const > const_iterator;

  iterator begin() {
    return iterator( this, firstItemPosition );
  }

  iterator end() {
    return iterator( this, itemPosition( lastItemPosition.asIndex() + 1 ) );
  }

  /// Masked iterator for bloque (iterates over all valid items for which the bit for the specified mask is set)

  template< class IteratedObjectType, class IteratedMaskType >
  struct bloque_masked_iterator : std::iterator< std::forward_iterator_tag, IteratedObjectType > {

    IteratedObjectType & operator* () { return blq->get_item_by_position( currentItemPosition ); }

    template< class __IteratedObjectType, class __IteratedMaskType >
    friend bool operator== ( bloque_masked_iterator const &lhs, bloque_masked_iterator< __IteratedObjectType, __IteratedMaskType > const &rhs ) {
      return lhs.currentItemPosition == rhs.currentItemPosition;
    }

    template< class __IteratedObjectType, class __IteratedMaskType >
    friend bool operator!= ( bloque_masked_iterator const &lhs, bloque_masked_iterator< __IteratedObjectType, __IteratedMaskType > const &rhs ) {
      return lhs.currentItemPosition != rhs.currentItemPosition;
    }

    bloque_masked_iterator & operator++ () {
      currentItemPosition = blq->getNextItemPosition< IteratedMaskType >( currentItemPosition );
      return *this;
    }

    template< class __IteratedObjectType, class __IteratedMaskType >
    bool operator< ( bloque_masked_iterator< __IteratedObjectType, __IteratedMaskType > const & other ) {
      return currentItemPosition < other.currentItemPosition;
    }

    //private:

    itemPosition currentItemPosition;
    bloque * blq;

    // private constructor for begin, end
    friend class bloque;
    bloque_masked_iterator( bloque * __bloque, itemPosition __currentItemPosition ) {
      blq = __bloque;
      currentItemPosition = __currentItemPosition;
    }
  };

  // templated typedef workaround

  template< typename __MaskType >
  struct masked_iterator : bloque_masked_iterator< ObjectType, __MaskType > { };

  template< typename __MaskType >
  struct const_masked_iterator : bloque_masked_iterator< ObjectType const, __MaskType > { };

  // begin, end for masked iterators

  template< typename __MaskType >
  masked_iterator< __MaskType > begin() {
    return masked_iterator< __MaskType >( this, firstItemPosition );
  }

  template< typename __MaskType >
  const_masked_iterator< __MaskType > begin() {
    return const_masked_iterator< __MaskType >( this, firstItemPosition );
  }

  template< typename __MaskType >
  masked_iterator< __MaskType > end() {
    return masked_iterator< __MaskType >( this, itemPosition( lastItemPosition.asIndex() + 1 ) );
  }

  template< typename __MaskType >
  const_masked_iterator< __MaskType > end() {
    return const_masked_iterator< __MaskType >( this, itemPosition( lastItemPosition.asIndex() + 1 ) );
  }

  // TODO masked iterator needs work, prefer one of the 'apply' methods
  //
  ///////////// END ITERATORS /////////////////////////////////////////

  /*
  void print() {
    // print method for debugging
    
    //firstItemPosition.print();
    //lastItemPosition.print();
    for ( size_t i = 0; i < blockVector.size(); ++i ) {
      for ( size_t j = 0; j < bitsPerBlock; ++j ) {
        std::cout << i << " " << j << " " << blockVector[i][j] << " " << (bool) ( ( defaultMask[ i ][ j / registerWidth ] ) & ( (BitType) 1 << ( j % registerWidth ) ) ) << std::endl;
      }
    }
    
    /////////////////////////////

    for ( iterator i = begin(); i != end(); ++i ) {
      i.currentItemPosition.print();
      std::cout << *i << std::endl;
    }
  }
  */

  /*
   * overloaded stream; mainly for debugging
   *
  
  template < class __ObjectType, class __MaskType >
  friend std::ostream & operator<< ( std::ostream & os, const bloque< __ObjectType, __MaskType > & ) {
    std::cout << "hello there" << std::endl;
  }
  */

/* ****************************************************************
 * private methods ************************************************
 * ****************************************************************
 */

private:

  void init() {
    numItems = 0;
    blockSize = bitsPerBlock;
  }
  
  void addBlock() {
    
    size_t beginNewBlock = blockVector.size();
    size_t index = endIndex;
    blockVector.push_back( new ObjectType[ blockSize ] );
    defaultMask.push_back( new BitType[ registersPerBlock ] );

    for ( size_t i = beginNewBlock; i < blockVector.size(); ++i ) { 
      for ( MaskMapItr mit = userMasks.begin(); mit != userMasks.end(); ++mit ) {
        (*mit).second.push_back( new BitType[ registersPerBlock ] );
      }
      for ( size_t j = 0; j < registersPerBlock; ++j ) {
        defaultMask[ i ][ j ] = 0;
        for ( MaskMapItr mit = userMasks.begin(); mit != userMasks.end(); ++mit ) {
          (*mit).second[ i ][ j ] = 0;
        }
        for ( size_t k = 0; k < registerWidth; ++k ) {
          addSlot( index );
          //std::cout << "index added: " << index << std::endl;
          ++index;
        }
      }
    }
    endIndex += blockSize;
  }

  void addSlot( size_t slot_index ) {
    freeSlots.push_back( itemPosition( slot_index ) );        
  }
  
  void setBit( BitType & registerSet, size_t bit ) {
    #pragma omp atomic
    registerSet |= ( (BitType) 1 << bit );
  }
  
  void clearBit( BitType & registerSet, size_t bit ) {  
    #pragma omp atomic
    registerSet &= ~( (BitType) 1 << bit);
  }
    
  void flipBit( BitType & registerSet, size_t bit ) {
    #pragma omp atomic
    registerSet ^= (BitType) 1 << bit;
  }

};


#endif
