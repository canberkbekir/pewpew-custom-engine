#include "cbpch.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Renderer/Core/ShaderUniforms.h"
#include "CBEngine/Debug/Instrumentor.h"

namespace CB
{
    Scope<Renderer3D::SceneData> Renderer3D::s_SceneData = CreateScope<SceneData>();

    void Renderer3D::Init()
    {
        CB_PROFILE_FUNCTION();
    }

    void Renderer3D::Shutdown()
    {
        CB_PROFILE_FUNCTION();
    }

    void Renderer3D::BeginScene(const Camera& camera, const Vector3& cameraPosition)
    {
        CB_PROFILE_FUNCTION();
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
        s_SceneData->CameraPosition = cameraPosition;
    }

    void Renderer3D::EndScene()
    {
        CB_PROFILE_FUNCTION();
    }

    void Renderer3D::SetDirectionalLight(const Vector3& direction, const Vector3& color, float intensity)
    {
        s_SceneData->LightDirection = direction;
        s_SceneData->LightColor = color;
        s_SceneData->LightIntensity = intensity;
    }

    void Renderer3D::SetAmbientLight(const Vector3& color)
    {
        s_SceneData->AmbientColor = color;
    }

    void Renderer3D::Submit(const Ref<Shader>& shader, const Ref<Material>& material, const Ref<Mesh>& mesh,
                            const Mat4& transform, int entityID, bool useVertexColor,
                            const Ref<Texture2D>& paletteColorTex,
                            const Ref<Texture2D>& paletteMaterialTex)
    {
        CB_PROFILE_FUNCTION();
        using namespace ShaderUniforms;

        shader->Bind();

        // Upload transform uniforms
        shader->SetMat4(ViewProjection, s_SceneData->ViewProjectionMatrix);
        shader->SetMat4(Transform, transform);
        shader->SetFloat3(CameraPosition, s_SceneData->CameraPosition);

        // Entity ID for picking
        shader->SetInt(EntityID, entityID);

        // Instancing off for regular submit
        shader->SetInt(UseInstancing, 0);

        // Palette mode
        bool usePalette = paletteColorTex && paletteMaterialTex;
        shader->SetInt(UsePalette, usePalette ? 1 : 0);
        if (usePalette)
        {
            paletteColorTex->Bind(4);
            paletteMaterialTex->Bind(5);
            shader->SetInt(PaletteColorMap, 4);
            shader->SetInt(PaletteMaterialMap, 5);
            // Palette mode overrides vertex color mode
            shader->SetInt(UseVertexColor, 0);
        }
        else
        {
            // Vertex color mode (voxel meshes use baked vertex colors as albedo)
            shader->SetInt(UseVertexColor, useVertexColor ? 1 : 0);
        }

        // Upload light uniforms
        shader->SetFloat3(LightDirection, s_SceneData->LightDirection);
        shader->SetFloat3(LightColor, s_SceneData->LightColor);
        shader->SetFloat(LightIntensity, s_SceneData->LightIntensity);
        shader->SetFloat3(AmbientColor, s_SceneData->AmbientColor);

        // Bind material (textures and properties)
        material->Bind(shader);

        // Draw mesh
        mesh->Bind();
        RenderCommand::DrawIndexed(mesh->GetVertexArray());
    }

    void Renderer3D::SubmitVoxelBatch(const Ref<Shader>& shader, const Ref<Material>& material,
                                       const Ref<Mesh>& mesh,
                                       const Ref<Texture2D>& paletteColorTex,
                                       const Ref<Texture2D>& paletteMaterialTex,
                                       const std::vector<Mat4>& transforms,
                                       const std::vector<int>& entityIDs)
    {
        CB_PROFILE_FUNCTION();
        using namespace ShaderUniforms;

        if (transforms.empty())
            return;

        shader->Bind();

        // Scene uniforms
        shader->SetMat4(ViewProjection, s_SceneData->ViewProjectionMatrix);
        shader->SetFloat3(CameraPosition, s_SceneData->CameraPosition);

        // Instancing on
        shader->SetInt(UseInstancing, 1);

        // Palette mode
        shader->SetInt(UsePalette, 1);
        shader->SetInt(UseVertexColor, 0);
        paletteColorTex->Bind(4);
        paletteMaterialTex->Bind(5);
        shader->SetInt(PaletteColorMap, 4);
        shader->SetInt(PaletteMaterialMap, 5);

        // Light uniforms
        shader->SetFloat3(LightDirection, s_SceneData->LightDirection);
        shader->SetFloat3(LightColor, s_SceneData->LightColor);
        shader->SetFloat(LightIntensity, s_SceneData->LightIntensity);
        shader->SetFloat3(AmbientColor, s_SceneData->AmbientColor);

        material->Bind(shader);

        // Build per-instance data: mat4 transform + int entityID (padded to 80 bytes)
        struct InstanceData
        {
            Mat4 Transform;
            int EntityID;
            int _pad[3]; // Align to 16 bytes
        };

        uint32_t instanceCount = static_cast<uint32_t>(transforms.size());
        std::vector<InstanceData> instanceData(instanceCount);
        for (uint32_t i = 0; i < instanceCount; i++)
        {
            instanceData[i].Transform = transforms[i];
            instanceData[i].EntityID = entityIDs[i];
            instanceData[i]._pad[0] = 0;
            instanceData[i]._pad[1] = 0;
            instanceData[i]._pad[2] = 0;
        }

        // Create and upload instance buffer
        uint32_t bufferSize = instanceCount * sizeof(InstanceData);
        auto instanceBuffer = VertexBuffer::Create(bufferSize);
        instanceBuffer->SetSubData(instanceData.data(), bufferSize);
        instanceBuffer->SetLayout({
            { ShaderDataType::Mat4, "a_InstanceTransform" },
            { ShaderDataType::Int,  "a_InstanceEntityID" },
            { ShaderDataType::Int3, "a_InstancePad" },
        });

        // Create a temporary VAO for instanced drawing (avoid modifying mesh's VAO)
        auto tempVAO = VertexArray::Create();

        // Copy mesh's vertex buffers
        const auto& meshVAO = mesh->GetVertexArray();
        for (const auto& vb : meshVAO->GetVertexBuffers())
            tempVAO->AddVertexBuffer(vb);

        // Add instance buffer with divisor
        tempVAO->AddInstancedVertexBuffer(instanceBuffer, 1);

        // Set index buffer
        tempVAO->SetIndexBuffer(meshVAO->GetIndexBuffer());

        // Draw instanced
        tempVAO->Bind();
        RenderCommand::DrawIndexedInstanced(tempVAO, instanceCount);
    }
}
