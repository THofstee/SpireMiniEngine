#ifndef MESH_BUILDER_H
#define MESH_BUILDER_H

#include "Mesh.h"

namespace GameEngine
{
    class MeshBuilder
    {
        struct Vertex
        {
            VectorMath::Vec3 pos;
            VectorMath::Vec2 uv;
            VectorMath::Vec3 normal;
            VectorMath::Vec3 tangent;
        };

    private:
        CoreLib::List<MeshBuilder::Vertex> vertices;
        CoreLib::List<int> indices;

    public:
        void AddTriangle(VectorMath::Vec3 v1, VectorMath::Vec3 v2, VectorMath::Vec3 v3);
        void AddTriangle(VectorMath::Vec3 v1, VectorMath::Vec2 uv1, VectorMath::Vec3 v2, VectorMath::Vec2 uv2, VectorMath::Vec3 v3, VectorMath::Vec2 uv3);
        void AddQuad(VectorMath::Vec3 v1, VectorMath::Vec3 v2, VectorMath::Vec3 v3, VectorMath::Vec3 v4);
        void AddLine(VectorMath::Vec3 v1, VectorMath::Vec3 v2, VectorMath::Vec3 normal, float width);
        void AddDisk(VectorMath::Vec3 center, VectorMath::Vec3 normal, float radiusOuter, float radiusInner = 0.0f, int edges = 16);

        void AddBox(VectorMath::Vec3 vmin, VectorMath::Vec3 vmax);
        void AddFrustum(VectorMath::Vec3 v1, VectorMath::Vec3 v2, float radius1, float radius2, int edges = 16);

        Mesh ToMesh();
    };
}

#endif