
#ifndef _FRED_EVOLUTION_FACTORY_H
#define _FRED_EVOLUTION_FACTORY_H

#include <map>

class Evolution;

/**
 * Factory class to create instances of Evolution objects
 */
class EvolutionFactory {
  public:

    /**
     * This static factory method is used to get an instance of a specific Evolution model.
     * Depending on the type parameter, it will create a specific Evolution Model and return
     * a pointer to it.
     *
     * @param the specific Evolution model type
     * @return a pointer to a specific Evolution model
     */
    static Evolution *newEvolution(int type);
};

#endif


