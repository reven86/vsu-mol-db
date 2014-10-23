#pragma once

#ifndef __MOLECULE_MODEL_H__
#define __MOLECULE_MODEL_H__




class MoleculeModel : public gameplay::Ref
{
    RefPtr< class Molecule > _molecule;
    RefPtr< gameplay::Node > _rootNode;
    gameplay::Properties * _defaultMaterialProperties;

    gameplay::BoundingBox _bbox;

public:
    sigc::signal<void> modelChangedSignal;

    virtual ~MoleculeModel();

    static MoleculeModel * create(class Molecule * molecule);

    gameplay::Node * getRootNode() { return _rootNode; };

protected:
    MoleculeModel();

    void setupModel();
};




#endif // __MOLECULE_MODEL_H__