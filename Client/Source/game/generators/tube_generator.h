#pragma once

#ifndef __TUBE_GENERATOR_H__
#define __TUBE_GENERATOR_H__

#include "game/molecule.h"



class TubeGenerator
{
public:
    TubeGenerator();
    virtual ~TubeGenerator();

    void generateCarbonTube(int n1, int n2, int k, float transition, std::vector<Molecule::Atom>& atoms, std::vector<Molecule::Link>& links);
};



#endif // __TUBE_GENERATOR_H__

