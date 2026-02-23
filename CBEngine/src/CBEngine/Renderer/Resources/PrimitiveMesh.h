#pragma once

#include "Mesh.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Asset/Asset.h"
#include "CBEngine/Asset/AssetManager.h"

#include <vector>
#include <unordered_map>
#include <cmath>

namespace CB
{
    // Well-known UUIDs for built-in primitive meshes.
    // Values 1-5 are reserved and will never collide with the random 64-bit UUID generator.
    namespace PrimitiveUUIDs
    {
        static constexpr uint64_t Cube     = 1;
        static constexpr uint64_t Sphere   = 2;
        static constexpr uint64_t Plane    = 3;
        static constexpr uint64_t Cylinder = 4;
        static constexpr uint64_t Capsule  = 5;
    }

    // Procedural primitive mesh factory.
    // Call InitAll() once after the OpenGL context is ready (e.g. in EditorLayer::OnAttach).
    // After that, GetCube() / GetSphere() etc. return cached Ref<Mesh> instances and are
    // also registered into AssetManager so that scene serialization round-trips correctly.
    class PrimitiveMesh
    {
    public:
        // Pre-generate all primitives and register them in the AssetManager.
        // Must be called on the main (GL) thread.
        static void InitAll()
        {
            GetCube();
            GetSphere();
            GetPlane();
            GetCylinder();
            GetCapsule();
        }

        static Ref<Mesh> GetCube()     { return GetOrCreate(UUID(PrimitiveUUIDs::Cube),     &GenerateCube); }
        static Ref<Mesh> GetSphere()   { return GetOrCreate(UUID(PrimitiveUUIDs::Sphere),   &GenerateSphere); }
        static Ref<Mesh> GetPlane()    { return GetOrCreate(UUID(PrimitiveUUIDs::Plane),    &GeneratePlane); }
        static Ref<Mesh> GetCylinder() { return GetOrCreate(UUID(PrimitiveUUIDs::Cylinder), &GenerateCylinder); }
        static Ref<Mesh> GetCapsule()  { return GetOrCreate(UUID(PrimitiveUUIDs::Capsule),  &GenerateCapsule); }

        static bool IsPrimitiveUUID(UUID uuid)
        {
            uint64_t v = static_cast<uint64_t>(uuid);
            return v >= PrimitiveUUIDs::Cube && v <= PrimitiveUUIDs::Capsule;
        }

        // Resolve a primitive UUID back to a Mesh (used during scene deserialization).
        // Returns nullptr if uuid is not a known primitive.
        static Ref<Mesh> TryResolve(UUID uuid)
        {
            switch (static_cast<uint64_t>(uuid))
            {
            case PrimitiveUUIDs::Cube:     return GetCube();
            case PrimitiveUUIDs::Sphere:   return GetSphere();
            case PrimitiveUUIDs::Plane:    return GetPlane();
            case PrimitiveUUIDs::Cylinder: return GetCylinder();
            case PrimitiveUUIDs::Capsule:  return GetCapsule();
            default: return nullptr;
            }
        }

    private:
        using GeometryFn = std::pair<std::vector<Vertex>, std::vector<uint32_t>>(*)();

        // ---- Geometry generators ----

        static std::pair<std::vector<Vertex>, std::vector<uint32_t>> GenerateCube()
        {
            struct FaceData
            {
                Vector3 normal;
                Vector3 tangent;
                Vector3 positions[4];
                Vector2 uvs[4];
            };

            // 6 faces, CCW winding viewed from outside
            FaceData faces[6] = {
                // Front (+Z)
                { {0,0,1}, {1,0,0},
                  {{-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}},
                  {{0,0},{1,0},{1,1},{0,1}} },
                // Back (-Z)
                { {0,0,-1}, {-1,0,0},
                  {{ 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}},
                  {{0,0},{1,0},{1,1},{0,1}} },
                // Right (+X)
                { {1,0,0}, {0,0,-1},
                  {{ 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}},
                  {{0,0},{1,0},{1,1},{0,1}} },
                // Left (-X)
                { {-1,0,0}, {0,0,1},
                  {{-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}},
                  {{0,0},{1,0},{1,1},{0,1}} },
                // Top (+Y)
                { {0,1,0}, {1,0,0},
                  {{-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}},
                  {{0,0},{1,0},{1,1},{0,1}} },
                // Bottom (-Y)
                { {0,-1,0}, {1,0,0},
                  {{-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}},
                  {{0,0},{1,0},{1,1},{0,1}} },
            };

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            vertices.reserve(24);
            indices.reserve(36);

            for (auto& face : faces)
            {
                uint32_t base = static_cast<uint32_t>(vertices.size());
                Vector3 bitangent = glm::cross(face.normal, face.tangent);

                for (int i = 0; i < 4; i++)
                {
                    Vertex v;
                    v.Position  = face.positions[i];
                    v.Normal    = face.normal;
                    v.TexCoords = face.uvs[i];
                    v.Tangent   = face.tangent;
                    v.Bitangent = bitangent;
                    vertices.push_back(v);
                }
                indices.insert(indices.end(), {base, base+1, base+2, base+2, base+3, base});
            }

            return { vertices, indices };
        }

        static std::pair<std::vector<Vertex>, std::vector<uint32_t>> GeneratePlane()
        {
            // 1x1 plane in the XZ plane, facing +Y
            Vector3 normal  = {0, 1, 0};
            Vector3 tangent = {1, 0, 0};
            Vector3 bitangent = {0, 0, -1};

            std::vector<Vertex> vertices = {
                { {-0.5f, 0, 0.5f}, normal, {0,0}, tangent, bitangent },
                { { 0.5f, 0, 0.5f}, normal, {1,0}, tangent, bitangent },
                { { 0.5f, 0,-0.5f}, normal, {1,1}, tangent, bitangent },
                { {-0.5f, 0,-0.5f}, normal, {0,1}, tangent, bitangent },
            };
            std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
            return { vertices, indices };
        }

        static std::pair<std::vector<Vertex>, std::vector<uint32_t>> GenerateSphere()
        {
            const int stacks = 16;
            const int slices = 32;
            const float radius = 0.5f;
            const float PI = glm::pi<float>();

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            vertices.reserve((stacks + 1) * (slices + 1));
            indices.reserve(stacks * slices * 6);

            for (int stack = 0; stack <= stacks; stack++)
            {
                float phi = PI * (float)stack / (float)stacks; // 0 (top) -> PI (bottom)
                float cosPhi = std::cos(phi);
                float sinPhi = std::sin(phi);
                float y = radius * cosPhi;
                float r = radius * sinPhi;

                for (int slice = 0; slice <= slices; slice++)
                {
                    float theta = 2.0f * PI * (float)slice / (float)slices;
                    float cosTheta = std::cos(theta);
                    float sinTheta = std::sin(theta);

                    Vertex v;
                    v.Position  = { r * cosTheta, y, r * sinTheta };
                    v.Normal    = { sinPhi * cosTheta, cosPhi, sinPhi * sinTheta };
                    v.TexCoords = { (float)slice / (float)slices, (float)stack / (float)stacks };
                    v.Tangent   = { -sinTheta, 0, cosTheta };
                    v.Bitangent = glm::cross(v.Normal, v.Tangent);
                    vertices.push_back(v);
                }
            }

            for (int stack = 0; stack < stacks; stack++)
            {
                for (int slice = 0; slice < slices; slice++)
                {
                    uint32_t a = stack       * (slices + 1) + slice;
                    uint32_t b = (stack + 1) * (slices + 1) + slice;
                    indices.insert(indices.end(), { a, b, a+1 });
                    indices.insert(indices.end(), { b, b+1, a+1 });
                }
            }

            return { vertices, indices };
        }

        static std::pair<std::vector<Vertex>, std::vector<uint32_t>> GenerateCylinder()
        {
            const int segments   = 32;
            const float radius   = 0.5f;
            const float halfH    = 0.5f;
            const float PI       = glm::pi<float>();

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            // --- Side ---
            // Two rings (bottom, top) interleaved: [bottom_0, top_0, bottom_1, top_1, ...]
            for (int i = 0; i <= segments; i++)
            {
                float theta = 2.0f * PI * (float)i / (float)segments;
                float x = radius * std::cos(theta);
                float z = radius * std::sin(theta);
                float u = (float)i / (float)segments;

                Vector3 normal  = glm::normalize(Vector3(x, 0, z));
                Vector3 tangent = { -std::sin(theta), 0, std::cos(theta) };
                Vector3 bitan   = {0, 1, 0};

                Vertex vb;
                vb.Position = {x, -halfH, z};
                vb.Normal = normal; vb.TexCoords = {u, 0}; vb.Tangent = tangent; vb.Bitangent = bitan;
                vertices.push_back(vb);

                Vertex vt;
                vt.Position = {x, halfH, z};
                vt.Normal = normal; vt.TexCoords = {u, 1}; vt.Tangent = tangent; vt.Bitangent = bitan;
                vertices.push_back(vt);
            }
            for (int i = 0; i < segments; i++)
            {
                uint32_t b0 = i * 2, t0 = i * 2 + 1;
                uint32_t b1 = b0 + 2, t1 = t0 + 2;
                indices.insert(indices.end(), {b0, b1, t0});
                indices.insert(indices.end(), {b1, t1, t0});
            }

            // --- Top cap ---
            uint32_t topCenter = (uint32_t)vertices.size();
            {
                Vertex v; v.Position = {0, halfH, 0}; v.Normal = {0,1,0}; v.TexCoords = {0.5f,0.5f};
                v.Tangent = {1,0,0}; v.Bitangent = {0,0,-1};
                vertices.push_back(v);
            }
            uint32_t topRingBase = (uint32_t)vertices.size();
            for (int i = 0; i <= segments; i++)
            {
                float theta = 2.0f * PI * (float)i / (float)segments;
                float x = radius * std::cos(theta);
                float z = radius * std::sin(theta);
                Vertex v; v.Position = {x, halfH, z}; v.Normal = {0,1,0};
                v.TexCoords = {0.5f + 0.5f * std::cos(theta), 0.5f - 0.5f * std::sin(theta)};
                v.Tangent = {1,0,0}; v.Bitangent = {0,0,-1};
                vertices.push_back(v);
            }
            for (int i = 0; i < segments; i++)
                indices.insert(indices.end(), {topCenter, topRingBase + i, topRingBase + i + 1});

            // --- Bottom cap ---
            uint32_t botCenter = (uint32_t)vertices.size();
            {
                Vertex v; v.Position = {0, -halfH, 0}; v.Normal = {0,-1,0}; v.TexCoords = {0.5f,0.5f};
                v.Tangent = {1,0,0}; v.Bitangent = {0,0,1};
                vertices.push_back(v);
            }
            uint32_t botRingBase = (uint32_t)vertices.size();
            for (int i = 0; i <= segments; i++)
            {
                float theta = 2.0f * PI * (float)i / (float)segments;
                float x = radius * std::cos(theta);
                float z = radius * std::sin(theta);
                Vertex v; v.Position = {x, -halfH, z}; v.Normal = {0,-1,0};
                v.TexCoords = {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)};
                v.Tangent = {1,0,0}; v.Bitangent = {0,0,1};
                vertices.push_back(v);
            }
            for (int i = 0; i < segments; i++)
                indices.insert(indices.end(), {botCenter, botRingBase + i + 1, botRingBase + i});

            return { vertices, indices };
        }

        static std::pair<std::vector<Vertex>, std::vector<uint32_t>> GenerateCapsule()
        {
            // radius=0.5, cylinderHalfH=0.5 → total height=2.0 (same proportions as Unity capsule)
            const int   hemiStacks = 8;
            const int   slices     = 32;
            const float radius     = 0.5f;
            const float cylHalfH   = 0.5f;
            const float PI         = glm::pi<float>();

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            int totalRings = hemiStacks + 1 + hemiStacks + 1; // top hemi + bottom hemi

            // Adds one ring at a given spherical phi, with yOffset applied to Y
            auto addRing = [&](float phi, float yOffset)
            {
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);
                float y = radius * cosPhi + yOffset;
                float r = radius * sinPhi;

                for (int slice = 0; slice <= slices; slice++)
                {
                    float theta    = 2.0f * PI * (float)slice / (float)slices;
                    float cosTheta = std::cos(theta);
                    float sinTheta = std::sin(theta);

                    Vertex v;
                    v.Position  = { r * cosTheta, y, r * sinTheta };
                    v.Normal    = { sinPhi * cosTheta, cosPhi, sinPhi * sinTheta };
                    v.TexCoords = { (float)slice / (float)slices, 0 }; // v set below
                    v.Tangent   = { -sinTheta, 0, cosTheta };
                    v.Bitangent = glm::cross(v.Normal, v.Tangent);
                    vertices.push_back(v);
                }
            };

            // Top hemisphere: phi 0 (pole) → PI/2 (equator), yOffset = +cylHalfH
            for (int s = 0; s <= hemiStacks; s++)
            {
                float phi = PI * 0.5f * (float)s / (float)hemiStacks;
                addRing(phi, cylHalfH);
            }
            // Bottom hemisphere: phi PI/2 (equator) → PI (pole), yOffset = -cylHalfH
            for (int s = 0; s <= hemiStacks; s++)
            {
                float phi = PI * 0.5f + PI * 0.5f * (float)s / (float)hemiStacks;
                addRing(phi, -cylHalfH);
            }

            // Quads between consecutive rings (covers both hemispheres + the cylinder gap)
            for (int ring = 0; ring < totalRings - 1; ring++)
            {
                for (int slice = 0; slice < slices; slice++)
                {
                    uint32_t a = ring       * (slices + 1) + slice;
                    uint32_t b = (ring + 1) * (slices + 1) + slice;
                    indices.insert(indices.end(), {a, b, a+1});
                    indices.insert(indices.end(), {b, b+1, a+1});
                }
            }

            return { vertices, indices };
        }

        static Ref<Mesh> GetOrCreate(UUID uuid, GeometryFn generator)
        {
            auto it = s_Cache.find(static_cast<uint64_t>(uuid));
            if (it != s_Cache.end())
                return it->second;

            auto [verts, inds] = generator();
            auto mesh = CreateRef<Mesh>(verts, inds);
            AssetManager::RegisterBuiltinAsset(uuid, mesh, AssetType::Mesh);

            s_Cache[static_cast<uint64_t>(uuid)] = mesh;
            return mesh;
        }

        // Inline static cache (C++17)
        static inline std::unordered_map<uint64_t, Ref<Mesh>> s_Cache;
    };
}
