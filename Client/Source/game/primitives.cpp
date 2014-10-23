#include "pch.h"
#include "primitives.h"





void PrimitivesPool::generatePrimitives(const int quality)
{
    struct Vertex
    {
        gameplay::Vector4 pos;
        gameplay::Vector3 normal;
    };

    gameplay::VertexFormat::Element elements[] =
    {
        gameplay::VertexFormat::Element(gameplay::VertexFormat::POSITION, 4),
        gameplay::VertexFormat::Element(gameplay::VertexFormat::NORMAL, 3),
    };

    std::vector< Vertex > vertices;
    std::vector< uint16_t > indices;

    int i, j;

    for (i = 0; i < quality; i++)
    {
        float theta = MATH_PI * i / (quality - 1) - MATH_PIOVER2;
        for (j = 0; j < quality; j++)
        {
            float phi = MATH_PIX2 * j / quality;

            gameplay::Vector3 pos(cosf(phi) * cosf(theta), sinf(theta), sinf(phi) * cosf(theta));

            vertices.push_back(Vertex());
            vertices.back().pos.set(pos.x, pos.y, pos.z, 1.0f);
            vertices.back().normal = pos;

            if (i < quality - 1)
            {
                indices.push_back(static_cast<uint16_t>(i * quality + j));
                indices.push_back(static_cast<uint16_t>((i + 1) * quality + j));
                indices.push_back(static_cast<uint16_t>(i * quality + (j + 1) % quality));

                indices.push_back(static_cast<uint16_t>(i * quality + (j + 1) % quality));
                indices.push_back(static_cast<uint16_t>((i + 1) * quality + j));
                indices.push_back(static_cast<uint16_t>((i + 1) * quality + (j + 1) % quality));
            }
        }
    }

    gameplay::Mesh * mesh = gameplay::Mesh::createMesh(gameplay::VertexFormat(elements, 2), static_cast< unsigned >(vertices.size()));
    mesh->setVertexData(reinterpret_cast< float * >(&vertices[0]));

    gameplay::MeshPart * part = mesh->addPart(gameplay::Mesh::TRIANGLES, gameplay::Mesh::INDEX16, static_cast< unsigned >(indices.size()));
    part->setIndexData(&indices[0], 0, static_cast<unsigned>(indices.size()));

    _sphereMesh.reset(mesh);

    vertices.clear();
    indices.clear();

    for (i = 0; i < quality; i++)
    {
        float phi = MATH_PIX2 * i / quality;

        gameplay::Vector3 pos(cosf(phi), 0.0f, sinf(phi));

        vertices.push_back(Vertex());
        vertices.back().pos.set(pos.x, -1.0f, pos.z, 1.0f);
        vertices.back().normal = pos;
        vertices.push_back(Vertex());
        vertices.back().pos.set(pos.x, 1.0f, pos.z, 1.0f);
        vertices.back().normal = pos;

        indices.push_back(static_cast<uint16_t>(i * 2));
        indices.push_back(static_cast<uint16_t>(i * 2 + 1));
        indices.push_back(static_cast<uint16_t>(((i + 1) % quality) * 2));

        indices.push_back(static_cast<uint16_t>(((i + 1) % quality) * 2));
        indices.push_back(static_cast<uint16_t>(i * 2 + 1));
        indices.push_back(static_cast<uint16_t>(((i + 1) % quality) * 2 + 1));
    }

    mesh = gameplay::Mesh::createMesh(gameplay::VertexFormat(elements, 2), static_cast< unsigned >(vertices.size()));
    mesh->setVertexData(reinterpret_cast< float * >(&vertices[0]));

    part = mesh->addPart(gameplay::Mesh::TRIANGLES, gameplay::Mesh::INDEX16, static_cast< unsigned >(indices.size()));
    part->setIndexData(&indices[0], 0, static_cast<unsigned>(indices.size()));

    _cylinderMesh.reset(mesh);
}