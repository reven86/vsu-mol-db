#pragma once

#ifndef __MOLECULE_H__
#define __MOLECULE_H__




class Molecule : public gameplay::Ref
{
public:
    struct Atom
    {
        int index;
        float charge;
        gameplay::Vector3 pos;
    };

    struct Link
    {
        int atomIndex1;
        int atomIndex2;
        int count;
    };

private:
    std::vector<Atom> _atoms;
    std::vector<Link> _links;

    gameplay::BoundingBox _bbox;

public:
    sigc::signal<void> atomsChangedSignal;
    sigc::signal<void> linksChangedSignal;
    sigc::signal<void> atomsAndLinksChangedSignal;

public:
    virtual ~Molecule();

    static Molecule * create(const std::vector<Atom>& atoms, const std::vector<Link>& links);

    void setup(const std::vector<Atom>& atoms, const std::vector<Link>& links);
    void setAtoms(const std::vector<Atom>& atoms);
    void setLinks(const std::vector<Link>& links);

    const std::vector<Atom>& getAtoms() const { return _atoms; };
    const std::vector<Link>& getLinks() const { return _links; };
    const gameplay::BoundingBox& getBBox() const { return _bbox; };

protected:
    Molecule();

    void updateBBox();
};



#endif