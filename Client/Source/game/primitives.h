#pragma once

#ifndef __PRIMITIVES_H__
#define __PRIMITIVES_H__



class PrimitivesPool : public Singleton< PrimitivesPool >
{
    friend class Singleton< PrimitivesPool >;

    RefPtr< gameplay::Mesh > _sphereMesh;
    RefPtr< gameplay::Mesh > _cylinderMesh;

public:
    void generatePrimitives(const int quality = 10);

    gameplay::Mesh * getSphereMesh() { return _sphereMesh.get(); };
    gameplay::Mesh * getCylinderMesh() { return _cylinderMesh.get(); };

protected:
    PrimitivesPool() {};
    ~PrimitivesPool() {};
};



#endif // __PRIMITIVES_H__
