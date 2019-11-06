/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "ModelVertexColored.h"

#include "internal/noGraphicsAPI.h"

#include "objModelLoader.h"
#include "plyModelLoader.h"

namespace Saiga
{
void VertexColoredModel::createFullscreenQuad()
{
    vertices.push_back(VertexNC(vec3(-1, -1, 0), vec3(0, 0, 1)));
    vertices.push_back(VertexNC(vec3(1, -1, 0), vec3(0, 0, 1)));
    vertices.push_back(VertexNC(vec3(1, 1, 0), vec3(0, 0, 1)));
    vertices.push_back(VertexNC(vec3(-1, 1, 0), vec3(0, 0, 1)));
    addFace(0, 2, 3);
    addFace(0, 1, 2);
}

void VertexColoredModel::createCheckerBoard(ivec2 size, float quadSize, vec4 color1, vec4 color2)
{
    vec4 n(0, 1, 0, 0);
    for (int i = -size[0]; i < size[0]; ++i)
    {
        for (int j = -size[1]; j < size[1]; ++j)
        {
            vec4 c            = (j + i % 2) % 2 == 0 ? color1 : color2;
            VertexNC verts[4] = {
                {{(float)i, 0.f, (float)j, 1.f}, n, c},
                {{(float)i, 0.f, j + 1.f, 1.f}, n, c},
                {{(float)i + 1.f, 0.f, j + 1.f, 1.f}, n, c},
                {{(float)i + 1.f, 0.f, (float)j, 1.f}, n, c},
            };

            for (int i = 0; i < 4; ++i)
            {
                verts[i].position[0] *= quadSize;
                verts[i].position[2] *= quadSize;
            }

            addQuad(verts);
        }
    }
}

void VertexColoredModel::loadObj(const std::string& file)
{
    Saiga::ObjModelLoader loader(file);
    loader.computeVertexColorAndData();
    loader.toTriangleMesh(*this);
}

void VertexColoredModel::loadPly(const std::string& file)
{
    Saiga::PLYLoader loader(file);
    this->TriangleMesh<VertexNC, uint32_t>::operator=(loader.mesh);
}

void TexturedModel::loadObj(const std::string& file)
{
    Saiga::ObjModelLoader loader(file);
    loader.computeVertexColorAndData();
    loader.toTriangleMesh(*this);

    for (ObjTriangleGroup& otg : loader.triangleGroups)
    {
        if (otg.faces == 0) continue;

        TextureGroup tg;
        tg.indices    = otg.faces * 3;
        tg.startIndex = otg.startFace * 3;


        Material m;
        m.diffuse   = otg.material.map_Kd;
        tg.material = m;

        groups.push_back(tg);
    }
}

void VertexModel::createUniformPyramid()
{
    mesh.vertices.push_back(Vertex(vec3(0, 0, 0)));
    mesh.vertices.push_back(Vertex(vec3(1, -1, 1)));
    mesh.vertices.push_back(Vertex(vec3(1, -1, -1)));
    mesh.vertices.push_back(Vertex(vec3(-1, -1, -1)));
    mesh.vertices.push_back(Vertex(vec3(-1, -1, 1)));
    mesh.addFace(0, 1, 2);
    mesh.addFace(0, 2, 3);
    mesh.addFace(0, 3, 4);
    mesh.addFace(0, 4, 1);
    mesh.addFace(1, 3, 2);
    mesh.addFace(1, 4, 3);
}

void VertexModel::loadObj(const std::string& file)
{
    Saiga::ObjModelLoader loader(file);
    loader.toTriangleMesh(mesh);
}

/*void VertexModel::loadPly(const std::string& file)
{
    Saiga::PLYLoader loader(file);
    mesh = loader.mesh;
}*/
}  // namespace Saiga
