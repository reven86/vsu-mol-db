#include "pch.h"
#include "molecule_model.h"
#include "molecule.h"
#include "primitives.h"




MoleculeModel::MoleculeModel()
    : _defaultMaterialProperties(nullptr)
{
}

MoleculeModel::~MoleculeModel()
{
    SAFE_DELETE(_defaultMaterialProperties);
}

MoleculeModel * MoleculeModel::create(Molecule * molecule)
{
    MoleculeModel * res = new MoleculeModel();
    res->_molecule.reset(molecule);
    molecule->addRef();

    res->_defaultMaterialProperties = gameplay::Properties::create("materials/models/default.material");
    res->setupModel();

    res->_molecule->atomsChangedSignal.connect(sigc::mem_fun(res, &MoleculeModel::setupModel));
    res->_molecule->linksChangedSignal.connect(sigc::mem_fun(res, &MoleculeModel::setupModel));
    res->_molecule->atomsAndLinksChangedSignal.connect(sigc::mem_fun(res, &MoleculeModel::setupModel));

    return res;
}

void MoleculeModel::setupModel()
{
    if (_rootNode)
        _rootNode->removeAllChildren();
    else
        _rootNode.reset(gameplay::Node::create("rootNode"));

    _bbox = gameplay::BoundingBox::empty();

    for (auto it = _molecule->getAtoms().begin(), end_it = _molecule->getAtoms().end(); it != end_it; it++)
    {
        gameplay::Model * model = gameplay::Model::create(PrimitivesPool::getInstance()->getSphereMesh());

        _defaultMaterialProperties->rewind();
        gameplay::Material * material = gameplay::Material::create(_defaultMaterialProperties->getNextNamespace());
        model->setMaterial(material);
        SAFE_RELEASE(material);

        gameplay::Node * atomNode = gameplay::Node::create("atomNode");
        atomNode->setDrawable(model);
        SAFE_RELEASE(model);

        float scale = powf(static_cast<float>((*it).index), 0.33333f) * 0.25f;

        atomNode->setTranslation((*it).pos);
        atomNode->setScale(scale, scale, scale);
        _rootNode->addChild(atomNode);
        SAFE_RELEASE(atomNode);

        _bbox.enlarge((*it).pos);
    }

    if (!_molecule->getLinks().empty())
    {
        for (auto it = _molecule->getLinks().begin(), end_it = _molecule->getLinks().end(); it != end_it; it++)
        {
            if ((*it).atomIndex1 < 0 || (*it).atomIndex1 >= _molecule->getAtoms().size() ||
                (*it).atomIndex2 < 0 || (*it).atomIndex2 >= _molecule->getAtoms().size())
            {
                continue;
            }

            const gameplay::Vector3& pos1 = _molecule->getAtoms()[(*it).atomIndex1].pos;
            const gameplay::Vector3& pos2 = _molecule->getAtoms()[(*it).atomIndex2].pos;
            gameplay::Vector3 center = 0.5f * (pos1 + pos2);
            gameplay::Vector3 n = center;
            if (n.lengthSquared() < 0.0001f)
                n.set(0.0f, 1.0f, 0.0f);
            else
                n.normalize();

            gameplay::Vector3 dir(pos2 - pos1);

            gameplay::Vector3 up;
            gameplay::Vector3::cross(dir, gameplay::Vector3(dir.y, dir.z, dir.x), &up);
            up.normalize();

            const float radius = 0.03f;
            const float space = 0.07f;
            float totalSpace = (radius * 2.0f + space) * ((*it).count - 1);

            gameplay::Vector3 offset = 0.5f * totalSpace * up;
            gameplay::Vector3 doffset = -up * (space + radius * 2.0f);

            for (int i = 0; i < (*it).count; i++, offset += doffset)
            {
                gameplay::Model * model = gameplay::Model::create(PrimitivesPool::getInstance()->getCylinderMesh());

                _defaultMaterialProperties->rewind();
                gameplay::Material * material = gameplay::Material::create(_defaultMaterialProperties->getNextNamespace());
                model->setMaterial(material);
                SAFE_RELEASE(material);

                gameplay::Node * linkNode = gameplay::Node::create("linkNode");
                linkNode->setDrawable(model);
                SAFE_RELEASE(model);

                gameplay::Matrix transform;
                gameplay::Matrix::createLookAt(center, center + up, dir, &transform);

                linkNode->setTranslation(center + offset);
                linkNode->setScale(radius, (pos1 - pos2).length() * 0.5f, radius);
                linkNode->setRotation(transform);
                _rootNode->addChild(linkNode);
                SAFE_RELEASE(linkNode);
            }
        }
    }

    _rootNode->setTranslation(-_bbox.getCenter());

    modelChangedSignal();
}