#include "pch.h"
#include "molecule.h"




Molecule::Molecule()
{
}

Molecule::~Molecule()
{
}

Molecule * Molecule::create(const std::vector<Atom>& atoms, const std::vector<Link>& links)
{
    Molecule * res = new Molecule();
    res->_atoms = atoms;
    res->_links = links;

    return res;
}

void Molecule::setup(const std::vector<Atom>& atoms, const std::vector<Link>& links)
{
    _atoms = atoms;
    _links = links;
    atomsAndLinksChangedSignal();
}

void Molecule::setAtoms(const std::vector<Atom>& atoms)
{
    _atoms = atoms;
    atomsChangedSignal();
}

void Molecule::setLinks(const std::vector<Link>& links)
{
    _links = links;
    linksChangedSignal();
}