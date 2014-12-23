/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

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
 *  - TODO bloques can be 'linked' so that when an item is created in the 'parent' bloque,
 *         a slot with the corresponding index is automatically created in all of the 'child'
 *         bloques.  The ability to add slots directly to the 'child' bloques is lost
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

template < typename ObjectType, typename MaskType, typename LinkObjectType = int, typename LinkMaskType = char >
class bloque {

/*
 * private data members
 *
 */

private:

  friend class bloque< LinkObjectType, LinkMaskType, ObjectType, MaskType >;

  int is_linked, is_parent, is_child;
  std::vector< bloque< LinkObjectType, LinkMaskType, ObjectType, MaskType > * > links;

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

// *****************************************************************************
// ** public constructors, get/set methods *************************************
// *****************************************************************************

public:

  bloque() {
    is_linked = false;
    is_parent = false;
    is_child = false;
    numItems = 0;
    endIndex = 0;
    init();
    addBlock();
  }

  size_t get_index_size() { return endIndex; }

  void link_bloque( bloque< LinkObjectType, LinkMaskType, ObjectType, MaskType > * linked_child ) {
    assert( linked_child != NULL );
    // can only link bloques with the same size (for now)
    assert( blockVector.size() == linked_child->blockVector.size() );
    is_linked = true;
    is_parent = true;
    links.push_back( linked_child );
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

  int get_free_index() {
    assert( is_child == false );
    int free_index;
    #pragma omp critical(BLOQUE_RESIZE_LOCK)
    {
      if ( freeSlots.empty() ) {
        addBlock();
        if ( is_linked && is_parent ) {
          for ( int i = 0; i < links.size(); ++i ) {
            links[ i ]->addBlock();
          }
        }
      }
      itemPosition freePosition = freeSlots.front();
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
      freeSlots.pop_front();
      free_index = freePosition.asIndex(); 
    } // end critical(BLOQUE_RESIZE_LOCK)
    return free_index;
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
   * Returns true if the bit for the specified index is set in the
   * specified mask (assumes that the index is valid)
   */
  bool mask_is_set( MaskType mask, size_t index ) {
     itemPosition pos = itemPosition( index );
     assert( (       defaultMask[ pos.block ][ pos.slot / registerWidth ] ) & ( (BitType) 1 << ( pos.slot % registerWidth ) ) );
     return  ( userMasks[ mask ][ pos.block ][ pos.slot / registerWidth ] ) & ( (BitType) 1 << ( pos.slot % registerWidth ) );
  }

  template < typename Functor >
  void apply( Functor & f ) { apply( f, false ); }

  template < typename Functor >
  void parallel_apply( Functor & f ) { apply( f, true ); }

  template < typename Functor >
  void masked_apply( MaskType m, Functor & f ) { masked_apply( m, f, false ); }

  template < typename Functor >
  void parallel_masked_apply( MaskType m, Functor & f ) { masked_apply( m, f, true ); }

  /*
   * Generic, parallel 'apply' method for all items in container
   */
  template < typename Functor > 
  void apply( Functor & f, bool enable_parallelism ) {
    #pragma omp parallel for if(enable_parallelism)
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
  void masked_apply( MaskType m, Functor & f, bool enable_parallelism ) {
    mask & userMask = userMasks[ m ]; 
    #pragma omp parallel for if(enable_parallelism)
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

  /*
   * Generic, parallel 'not masked apply' method for all items in container
   *
   * Applies to all item that do NOT have the given mask set
   *
   */
  template < typename Functor > 
  void parallel_not_masked_apply( MaskType m, Functor & f ) {
    mask & userMask = userMasks[ m ]; 
    #pragma omp parallel for
    for ( int i = 0; i < blockVector.size(); ++i ) {
      for ( int j = 0; j < registersPerBlock; ++j ) {
        BitType reg = ( defaultMask[ i ][ j ] ) & ( ~( userMask[ i ][ j ] ) );  
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

  ObjectType & get_item_reference_by_index( size_t itemIndex ) {
    size_t block = itemIndex / bitsPerBlock;
    size_t slot = itemIndex % bitsPerBlock;
    assert( is_valid_item( block, slot ) );
    return blockVector[ block ][ slot ];
  }

  ObjectType * get_item_pointer_by_index( size_t itemIndex ) {
    size_t block = itemIndex / bitsPerBlock;
    size_t slot = itemIndex % bitsPerBlock;
    if ( is_valid_item( block, slot ) ) {
      return &( blockVector[ block ][ slot ] );
    }
    else { return NULL; }
  }

  bool is_valid_index( size_t itemIndex ) {
    size_t block = itemIndex / bitsPerBlock;
    size_t slot = itemIndex % bitsPerBlock;
    return is_valid_item( block, slot );
  }
/* ****************************************************************
 * private methods ************************************************
 * ****************************************************************
 */

private:

  bool is_valid_item( size_t block, size_t slot ) {
    return ( ( defaultMask[ block ][ slot / registerWidth ] ) & ( (BitType) 1 << ( slot % registerWidth ) ) );
  }

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
