#include "MeshImportPanel.h"

#include <imgui.h>

#include "CBEngine/Core/Log.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "CBEngine/Renderer/Resources/Texture.h"

#include "../Widgets/AssetPicker.h"

#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <cmath>

namespace CB
{
    MeshImportPanel::MeshImportPanel()
        : Panel("Mesh Import", false)
          , m_RawCameraController(45.0f, 4.0f / 3.0f, 0.1f, 100.0f)
    {
        // Create framebuffers for dual viewports
        FramebufferSpecification fbSpec;
        fbSpec.Width = 400;
        fbSpec.Height = 300;
        m_RawFramebuffer = Framebuffer::Create(fbSpec);
        m_VoxelFramebuffer = Framebuffer::Create(fbSpec);

        // Load preview shader and material
        m_PreviewShader = Shader::Create("assets/shaders/PBR.glsl");
        m_PreviewMaterial = CreateRef<Material>();
        m_PreviewMaterial->SetAlbedo({0.8f, 0.8f, 0.8f});
        m_PreviewMaterial->SetRoughness(0.5f);
        m_PreviewMaterial->SetMetallic(0.0f);

        // Default voxel settings
        m_VoxelSettings.GridSize = 32;
        m_VoxelSettings.Solid = true;
        m_VoxelSettings.Padding = 0.01f;

        // Default slots
        m_MaterialSlots.push_back({"Default", UUID(0)});
        m_TextureSlots.push_back({"Albedo", UUID(0)});
    }

    void MeshImportPanel::OpenForFile(const std::filesystem::path& filePath,
                                      const std::filesystem::path& outputDirectory)
    {
        CB_PROFILE_FUNCTION();

        m_SourceFilePath = filePath;
        m_OutputDirectory = outputDirectory.empty() ? filePath.parent_path() : outputDirectory;
        m_IsOpen = true;
        m_Visible = true;
        m_IsReimport = false;
        m_ReimportUUID = UUID(0);
        m_GenerateVoxelMesh = false;
        m_VoxelMesh = nullptr;

        // Set output name from filename
        m_OutputName = filePath.stem().string();
        strncpy_s(m_OutputNameBuffer, m_OutputName.c_str(), sizeof(m_OutputNameBuffer) - 1);

        // Reset slots
        m_MaterialSlots.clear();
        m_MaterialSlots.push_back({"Default", UUID(0)});
        m_TextureSlots.clear();
        m_TextureSlots.push_back({"Albedo", UUID(0)});
        m_ColorTextureUUID = UUID(0);
        m_ColorTexturePath.clear();

        // Reset voxel settings
        m_VoxelSettings.GridSize = 32;
        m_VoxelSettings.Solid = true;
        m_VoxelSettings.Padding = 0.01f;

        // Load raw mesh with CPU data retained (for voxelization)
        m_RawMesh = Mesh::Load(filePath.string(), true);

        if (!m_RawMesh)
        {
            CB_CORE_ERROR("MeshImportPanel: Failed to load mesh: {0}", filePath.string());
            Close();
            return;
        }

        FitCameraToMesh();

        CB_CORE_INFO("MeshImportPanel: Opened for {0}", filePath.string());
    }

    void MeshImportPanel::OpenForReimport(UUID assetUUID)
    {
        CB_PROFILE_FUNCTION();

        const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(assetUUID);
        if (!metadata)
        {
            CB_CORE_ERROR("MeshImportPanel: Asset not found for reimport");
            return;
        }

        m_IsReimport = true;
        m_ReimportUUID = assetUUID;

        if (metadata->Type == AssetType::ProcessedMesh)
        {
            auto asset = AssetManager::GetAsset<ProcessedMeshAsset>(assetUUID);
            if (!asset) return;

            std::filesystem::path sourcePath = AssetManager::GetAssetDirectory() / asset->SourceFilePath;
            m_SourceFilePath = sourcePath;
            m_GenerateVoxelMesh = false;

            // Restore settings
            m_MaterialSlots = asset->MaterialSlots;
            m_TextureSlots = asset->TextureSlots;

            // Set output name
            m_OutputName = metadata->FilePath.stem().string();
            strncpy_s(m_OutputNameBuffer, m_OutputName.c_str(), sizeof(m_OutputNameBuffer) - 1);
        }
        else if (metadata->Type == AssetType::VoxelMesh)
        {
            auto asset = AssetManager::GetAsset<VoxelMeshAsset>(assetUUID);
            if (!asset) return;

            std::filesystem::path sourcePath = AssetManager::GetAssetDirectory() / asset->SourceFilePath;
            m_SourceFilePath = sourcePath;
            m_GenerateVoxelMesh = true;
            m_VoxelSettings = asset->VoxelSettings;
            m_ColorTextureUUID = asset->ColorTextureUUID;

            // Restore slots
            m_MaterialSlots = asset->MaterialSlots;
            m_TextureSlots = asset->TextureSlots;

            m_OutputName = metadata->FilePath.stem().string();
            strncpy_s(m_OutputNameBuffer, m_OutputName.c_str(), sizeof(m_OutputNameBuffer) - 1);
        }

        m_IsOpen = true;
        m_Visible = true;

        // Output directory: same folder as the existing asset
        std::filesystem::path assetFullPath = AssetManager::GetAssetDirectory() / metadata->FilePath;
        m_OutputDirectory = assetFullPath.parent_path();

        // Load raw mesh
        m_RawMesh = Mesh::Load(m_SourceFilePath.string(), true);
        if (!m_RawMesh)
        {
            CB_CORE_ERROR("MeshImportPanel: Failed to load source mesh: {0}", m_SourceFilePath.string());
            Close();
            return;
        }

        FitCameraToMesh();

        // Start initial voxel preview if generating voxel mesh
        if (m_GenerateVoxelMesh && m_RawMesh->HasCPUData())
        {
            RegenVoxelPreview();
        }

        CB_CORE_INFO("MeshImportPanel: Opened for reimport of {0}", m_SourceFilePath.string());
    }

    void MeshImportPanel::OnUpdate(Timestep ts)
    {
        if (!m_IsOpen || !m_Visible)
            return;

        CB_PROFILE_FUNCTION();

        // Handle debounced voxel regeneration
        if (m_VoxelSettingsChanged)
        {
            auto now = std::chrono::steady_clock::now();
            float elapsedMs = std::chrono::duration<float, std::milli>(now - m_LastSettingsChangeTime).count();
            if (elapsedMs >= s_DebounceDelayMs)
            {
                m_VoxelSettingsChanged = false;
                RegenVoxelPreview();
            }
        }

        // Poll voxel task completion (only create mesh once)
        if (m_VoxelTask.IsComplete() && !m_VoxelMesh)
        {
            const auto& result = m_VoxelTask.GetResult();
            // Prefer palette mesh if palette data is available
            if (result.HasPalette)
            {
                m_VoxelMesh = m_VoxelTask.CreatePaletteMeshFromResult();

                // Create palette textures for preview
                std::vector<uint8_t> colorData, materialData;
                result.Palette.GenerateColorTextureData(colorData);
                result.Palette.GenerateMaterialTextureData(materialData);
                m_PreviewPaletteColorTex = Texture2D::CreateFromData(256, 1, colorData.data(), true);
                m_PreviewPaletteMaterialTex = Texture2D::CreateFromData(256, 1, materialData.data(), true);
                m_PreviewHasPalette = true;
            }
            else
            {
                m_VoxelMesh = m_VoxelTask.CreateMeshFromResult();
                m_PreviewHasPalette = false;
            }
        }

        // Update raw camera
        if (m_RawViewportHovered)
            m_RawCameraController.OnUpdate(ts);

        // Auto-rotate previews
        m_AutoRotationAngle += ts * 30.0f; // 30 degrees per second
        if (m_AutoRotationAngle >= 360.0f)
            m_AutoRotationAngle -= 360.0f;

        // Resize raw framebuffer
        {
            auto spec = m_RawFramebuffer->GetSpecification();
            if (m_RawViewportSize.x > 0 && m_RawViewportSize.y > 0)
            {
                if (spec.Width != static_cast<uint32_t>(m_RawViewportSize.x) ||
                    spec.Height != static_cast<uint32_t>(m_RawViewportSize.y))
                {
                    m_RawFramebuffer->Resize(
                        static_cast<uint32_t>(m_RawViewportSize.x),
                        static_cast<uint32_t>(m_RawViewportSize.y));
                    m_RawCameraController.SetViewportSize(m_RawViewportSize.x, m_RawViewportSize.y);
                }
            }
        }

        // Resize voxel framebuffer
        {
            auto spec = m_VoxelFramebuffer->GetSpecification();
            if (m_VoxelViewportSize.x > 0 && m_VoxelViewportSize.y > 0)
            {
                if (spec.Width != static_cast<uint32_t>(m_VoxelViewportSize.x) ||
                    spec.Height != static_cast<uint32_t>(m_VoxelViewportSize.y))
                {
                    m_VoxelFramebuffer->Resize(
                        static_cast<uint32_t>(m_VoxelViewportSize.x),
                        static_cast<uint32_t>(m_VoxelViewportSize.y));
                }
            }
        }

        Mat4 rotation = rotate(Mat4(1.0f), glm::radians(m_AutoRotationAngle), Vector3(0.0f, 1.0f, 0.0f));

        // Render raw mesh to FBO
        if (m_RawMesh)
        {
            m_RawFramebuffer->Bind();
            RenderCommand::SetClearColor({0.15f, 0.15f, 0.18f, 1.0f});
            RenderCommand::Clear();

            Renderer3D::BeginScene(
                m_RawCameraController.GetCamera(),
                m_RawCameraController.GetCamera().GetPosition());
            Renderer3D::SetDirectionalLight(Vector3(-0.5f, -1.0f, -0.3f), Vector3(1.0f), 1.5f);
            Renderer3D::SetAmbientLight(Vector3(0.15f));
            Renderer3D::Submit(m_PreviewShader, m_PreviewMaterial, m_RawMesh, rotation);
            Renderer3D::EndScene();

            m_RawFramebuffer->Unbind();
        }

        // Render voxel mesh to FBO
        if (m_VoxelMesh)
        {
            m_VoxelFramebuffer->Bind();
            RenderCommand::SetClearColor({0.12f, 0.12f, 0.15f, 1.0f});
            RenderCommand::Clear();

            Renderer3D::BeginScene(
                m_RawCameraController.GetCamera(),
                m_RawCameraController.GetCamera().GetPosition());
            Renderer3D::SetDirectionalLight(Vector3(-0.5f, -1.0f, -0.3f), Vector3(1.0f), 1.5f);
            Renderer3D::SetAmbientLight(Vector3(0.15f));
            Renderer3D::Submit(m_PreviewShader, m_PreviewMaterial, m_VoxelMesh, rotation,
                               -1, false, m_PreviewPaletteColorTex, m_PreviewPaletteMaterialTex);
            Renderer3D::EndScene();

            m_VoxelFramebuffer->Unbind();
        }
    }

    void MeshImportPanel::OnImGuiRender()
    {
        if (!m_IsOpen || !m_Visible)
            return;

        String windowTitle = "Import Preview: " + m_SourceFilePath.filename().string();
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

        bool open = true;
        if (ImGui::Begin(windowTitle.c_str(), &open))
        {
            if (!open)
            {
                Close();
                ImGui::End();
                return;
            }

            // === Dual viewports ===
            float availWidth = ImGui::GetContentRegionAvail().x;
            float halfWidth = (availWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            float viewportHeight = 250.0f;

            // Raw mesh viewport
            ImGui::BeginChild("##RawViewport", ImVec2(halfWidth, viewportHeight + 20.0f), true);
            {
                ImGui::Text("Raw Mesh");
                ImGui::Separator();
                RenderRawViewport();
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // Voxel preview viewport
            ImGui::BeginChild("##VoxelViewport", ImVec2(halfWidth, viewportHeight + 20.0f), true);
            {
                ImGui::Text("Voxelized Preview");
                ImGui::Separator();
                RenderVoxelViewport();
            }
            ImGui::EndChild();

            ImGui::Separator();

            // === Settings ===
            RenderSettings();

            ImGui::Separator();

            // === Material/Texture Slots ===
            RenderMaterialSlots();
            RenderTextureSlots();

            ImGui::Separator();

            // === Output section ===
            RenderOutputSection();
        }
        ImGui::End();
    }

    void MeshImportPanel::RenderRawViewport()
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        m_RawViewportSize = {size.x, size.y};

        if (m_RawMesh)
        {
            uint64_t texID = m_RawFramebuffer->GetColorAttachmentRendererID();
            ImGui::Image(reinterpret_cast<void*>(texID), size, ImVec2(0, 1), ImVec2(1, 0));
            m_RawViewportHovered = ImGui::IsItemHovered();
        }
        else
        {
            ImGui::TextDisabled("No mesh loaded");
            m_RawViewportHovered = false;
        }
    }

    void MeshImportPanel::RenderVoxelViewport()
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        m_VoxelViewportSize = {size.x, size.y};

        if (m_VoxelTask.IsRunning())
        {
            ImGui::TextDisabled("Voxelizing...");
        }
        else if (m_VoxelMesh)
        {
            uint64_t texID = m_VoxelFramebuffer->GetColorAttachmentRendererID();
            ImGui::Image(reinterpret_cast<void*>(texID), size, ImVec2(0, 1), ImVec2(1, 0));
        }
        else if (!m_GenerateVoxelMesh)
        {
            ImGui::TextDisabled("Enable 'Generate Voxel Mesh' to preview");
        }
        else
        {
            ImGui::TextDisabled("No voxel preview");
        }
    }

    void MeshImportPanel::RenderSettings()
    {
        if (ImGui::CollapsingHeader("Voxel Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool prevGenerateVoxel = m_GenerateVoxelMesh;
            if (ImGui::Checkbox("Generate Voxel Mesh (.vmesh)", &m_GenerateVoxelMesh))
            {
                if (m_GenerateVoxelMesh && !prevGenerateVoxel && m_RawMesh && m_RawMesh->HasCPUData())
                {
                    RegenVoxelPreview();
                }
                if (!m_GenerateVoxelMesh)
                {
                    m_VoxelMesh = nullptr;
                }
            }

            if (m_GenerateVoxelMesh)
            {
                ImGui::Indent();

                bool changed = false;

                changed |= ImGui::SliderInt("Grid Size", &m_VoxelSettings.GridSize, 8, 128);
                changed |= ImGui::Checkbox("Solid", &m_VoxelSettings.Solid);
                changed |= ImGui::DragFloat("Padding", &m_VoxelSettings.Padding, 0.001f, 0.0f, 1.0f, "%.3f");

                // Color texture picker
                ImGui::Text("Color Texture:");
                ImGui::SameLine();
                if (AssetPicker::DrawTexture("##ColorTex", m_ColorTextureUUID))
                {
                    // Resolve path for voxelization
                    if (m_ColorTextureUUID.IsValid())
                    {
                        const AssetMetadata* texMeta = AssetManager::GetRegistry().GetMetadata(m_ColorTextureUUID);
                        if (texMeta)
                        {
                            m_ColorTexturePath = (AssetManager::GetAssetDirectory() / texMeta->FilePath).string();
                        }
                    }
                    else
                    {
                        m_ColorTexturePath.clear();
                    }
                    changed = true;
                }

                if (changed)
                {
                    m_VoxelSettingsChanged = true;
                    m_LastSettingsChangeTime = std::chrono::steady_clock::now();
                }

                // Show voxel stats
                if (m_VoxelTask.IsComplete())
                {
                    const auto& result = m_VoxelTask.GetResult();
                    ImGui::TextDisabled("Voxels: %llu", result.VoxelCount);
                    ImGui::TextDisabled("Grid: %dx%dx%d",
                                        result.Grid.size.x, result.Grid.size.y, result.Grid.size.z);
                }
                else if (m_VoxelTask.IsRunning())
                {
                    ImGui::TextDisabled("Voxelizing...");
                }

                ImGui::Unindent();
            }
        }

        // Mesh stats
        if (m_RawMesh && ImGui::CollapsingHeader("Mesh Statistics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Vertices: %u", m_RawMesh->GetVertexCount());
            ImGui::Text("Triangles: %u", m_RawMesh->GetIndexCount() / 3);
            ImGui::TextDisabled("Source: %s", m_SourceFilePath.filename().string().c_str());
        }
    }

    void MeshImportPanel::RenderMaterialSlots()
    {
        if (ImGui::CollapsingHeader("Material Slots"))
        {
            for (size_t i = 0; i < m_MaterialSlots.size(); i++)
            {
                ImGui::PushID(static_cast<int>(i));

                String label = m_MaterialSlots[i].Name.empty()
                                   ? "Slot " + std::to_string(i)
                                   : m_MaterialSlots[i].Name;

                ImGui::Text("%s:", label.c_str());
                ImGui::SameLine();
                if (AssetPicker::DrawMaterial("##MatSlot", m_MaterialSlots[i].MaterialUUID))
                {
                    // Apply selected material to preview (use first slot)
                    if (i == 0 && m_MaterialSlots[0].MaterialUUID.IsValid())
                    {
                        auto mat = AssetManager::GetAsset<Material>(m_MaterialSlots[0].MaterialUUID);
                        if (mat)
                            m_PreviewMaterial = mat;
                    }
                    else if (i == 0)
                    {
                        // Reset to default preview material
                        m_PreviewMaterial = CreateRef<Material>();
                        m_PreviewMaterial->SetAlbedo({0.8f, 0.8f, 0.8f});
                        m_PreviewMaterial->SetRoughness(0.5f);
                        m_PreviewMaterial->SetMetallic(0.0f);
                    }
                }

                ImGui::PopID();
            }

            if (ImGui::SmallButton("+ Add Slot"))
            {
                m_MaterialSlots.push_back({"Material " + std::to_string(m_MaterialSlots.size()), UUID(0)});
            }
        }
    }

    void MeshImportPanel::RenderTextureSlots()
    {
        if (ImGui::CollapsingHeader("Texture Slots"))
        {
            for (size_t i = 0; i < m_TextureSlots.size(); i++)
            {
                ImGui::PushID(static_cast<int>(i));

                String label = m_TextureSlots[i].Name.empty()
                                   ? "Slot " + std::to_string(i)
                                   : m_TextureSlots[i].Name;

                ImGui::Text("%s:", label.c_str());
                ImGui::SameLine();
                if (AssetPicker::DrawTexture("##TexSlot", m_TextureSlots[i].TextureUUID))
                {
                    // Apply first texture slot as albedo map on preview material
                    if (i == 0)
                    {
                        if (m_TextureSlots[0].TextureUUID.IsValid())
                        {
                            auto tex = AssetManager::GetAsset<Texture2D>(m_TextureSlots[0].TextureUUID);
                            if (tex)
                            {
                                const AssetMetadata* texMeta = AssetManager::GetRegistry().GetMetadata(
                                    m_TextureSlots[0].TextureUUID);
                                String texPath = texMeta ? texMeta->FilePath.string() : "";
                                m_PreviewMaterial->SetAlbedoMap(tex, texPath);
                            }
                        }
                        else
                        {
                            m_PreviewMaterial->SetAlbedoMap(nullptr, "");
                        }
                    }
                }

                ImGui::PopID();
            }

            if (ImGui::SmallButton("+ Add Slot##Tex"))
            {
                m_TextureSlots.push_back({"Texture " + std::to_string(m_TextureSlots.size()), UUID(0)});
            }
        }
    }

    void MeshImportPanel::RenderOutputSection()
    {
        // Output name
        ImGui::Text("Output:");
        ImGui::SameLine();
        ImGui::PushItemWidth(200.0f);
        ImGui::InputText("##OutputName", m_OutputNameBuffer, sizeof(m_OutputNameBuffer));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text(m_GenerateVoxelMesh ? ".vmesh" : ".mesh");

        ImGui::Spacing();

        // Buttons
        float buttonWidth = 120.0f;
        float availWidth = ImGui::GetContentRegionAvail().x;

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
        {
            Close();
        }

        ImGui::SameLine(availWidth - buttonWidth);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        if (ImGui::Button("Import", ImVec2(buttonWidth, 0)))
        {
            ConfirmImport();
        }
        ImGui::PopStyleColor(2);
    }

    void MeshImportPanel::RegenVoxelPreview()
    {
        if (!m_RawMesh || !m_RawMesh->HasCPUData())
            return;

        if (!m_GenerateVoxelMesh)
            return;

        // Clear old mesh and palette so OnUpdate picks up the new result when ready
        m_VoxelMesh = nullptr;
        m_PreviewPaletteColorTex = nullptr;
        m_PreviewPaletteMaterialTex = nullptr;
        m_PreviewHasPalette = false;

        CB_CORE_INFO("MeshImportPanel: Starting voxel preview regeneration");

        if (m_ColorTexturePath.empty())
        {
            m_VoxelTask.StartFromData(
                m_RawMesh->GetVertices(),
                m_RawMesh->GetIndices(),
                m_VoxelSettings);
        }
        else
        {
            m_VoxelTask.StartFromDataWithColors(
                m_RawMesh->GetVertices(),
                m_RawMesh->GetIndices(),
                m_ColorTexturePath,
                m_VoxelSettings);
        }
    }

    void MeshImportPanel::GenerateVoxelMeshAsset(String outputName, std::filesystem::path sourceDir,
                                                 std::filesystem::path relativeSourcePath, UUID sourceMeshUUID)
    {
        String extension = ".vmesh";
        std::filesystem::path outputPath = sourceDir / (outputName + extension);

        auto asset = CreateRef<VoxelMeshAsset>();
        asset->SourceFilePath = relativeSourcePath.string();
        asset->SourceMeshUUID = sourceMeshUUID;
        asset->VoxelSettings = m_VoxelSettings;
        asset->ColorTextureUUID = m_ColorTextureUUID;
        asset->MaterialSlots = m_MaterialSlots;
        asset->TextureSlots = m_TextureSlots;

        // Store voxel grid data if available
        if (m_VoxelTask.IsComplete())
        {
            const auto& result = m_VoxelTask.GetResult();
            asset->GridData = result.Grid;
            asset->VoxelCount = result.VoxelCount;

            // Store palette data
            if (result.HasPalette)
            {
                asset->Palette = result.Palette;
                asset->PaletteIndices = result.PaletteIndices;
                asset->HasPalette = true;
            }

            // Store per-voxel UVs for deferred texture sampling
            if (result.HasUVs)
            {
                asset->VoxelUVs = result.VoxelUVs;
                asset->HasUVs = true;
            }
        }

        if (asset->Save(outputPath))
        {
            // Import into asset manager
            std::filesystem::path relOutput = relative(
                outputPath, AssetManager::GetAssetDirectory());
            UUID importedUUID = AssetManager::ImportAsset(relOutput);

            // Update dependencies
            std::vector<UUID> deps;
            if (sourceMeshUUID.IsValid()) deps.push_back(sourceMeshUUID);
            if (m_ColorTextureUUID.IsValid()) deps.push_back(m_ColorTextureUUID);
            for (const auto& slot : m_MaterialSlots)
                if (slot.MaterialUUID.IsValid()) deps.push_back(slot.MaterialUUID);
            for (const auto& slot : m_TextureSlots)
                if (slot.TextureUUID.IsValid()) deps.push_back(slot.TextureUUID);

            if (importedUUID.IsValid() && !deps.empty())
                AssetManager::GetRegistry().UpdateDependencies(importedUUID, deps);

            CB_CORE_INFO("MeshImportPanel: Exported .vmesh to {0}", outputPath.string());

            // Auto-generate .vtex alongside .vmesh
            auto vtex = VoxelTextureAsset::GenerateFromVmesh(asset);
            if (vtex)
            {
                // Use the actual AssetManager UUID, not the in-memory object's UUID
                vtex->SourceVmeshUUID = importedUUID;
                std::filesystem::path vtexPath = sourceDir / (outputName + ".vtex");
                if (vtex->Save(vtexPath))
                {
                    std::filesystem::path relVtex = relative(
                        vtexPath, AssetManager::GetAssetDirectory());
                    UUID vtexUUID = AssetManager::ImportAsset(relVtex);

                    if (vtexUUID.IsValid() && importedUUID.IsValid())
                        AssetManager::GetRegistry().UpdateDependencies(vtexUUID, {importedUUID});

                    CB_CORE_INFO("MeshImportPanel: Auto-generated .vtex to {0}", vtexPath.string());
                }
            }
        }
    }

    void MeshImportPanel::GenerateProccessedMeshAsset(String outputName, std::filesystem::path sourceDir,
                                                      std::filesystem::path relativeSourcePath, UUID sourceMeshUUID)
    {
        String extension = ".mesh";
        std::filesystem::path outputPath = sourceDir / (outputName + extension);

        auto asset = CreateRef<ProcessedMeshAsset>();
        asset->SourceFilePath = relativeSourcePath.string();
        asset->SourceMeshUUID = sourceMeshUUID;
        asset->MaterialSlots = m_MaterialSlots;
        asset->TextureSlots = m_TextureSlots;

        // Embed vertex/index data so the FBX is not needed at runtime
        if (m_RawMesh && m_RawMesh->HasCPUData())
        {
            asset->Vertices = m_RawMesh->GetVertices();
            asset->Indices = m_RawMesh->GetIndices();
        }

        if (asset->Save(outputPath))
        {
            std::filesystem::path relOutput = relative(
                outputPath, AssetManager::GetAssetDirectory());
            UUID importedUUID = AssetManager::ImportAsset(relOutput);

            // Update dependencies
            std::vector<UUID> deps;
            if (sourceMeshUUID.IsValid()) deps.push_back(sourceMeshUUID);
            for (const auto& slot : m_MaterialSlots)
                if (slot.MaterialUUID.IsValid()) deps.push_back(slot.MaterialUUID);
            for (const auto& slot : m_TextureSlots)
                if (slot.TextureUUID.IsValid()) deps.push_back(slot.TextureUUID);

            if (importedUUID.IsValid() && !deps.empty())
                AssetManager::GetRegistry().UpdateDependencies(importedUUID, deps);

            CB_CORE_INFO("MeshImportPanel: Exported .mesh to {0}", outputPath.string());
        }
    }

    void MeshImportPanel::ConfirmImport()
    {
        CB_PROFILE_FUNCTION();

        String outputName = m_OutputNameBuffer;
        if (outputName.empty())
            outputName = m_SourceFilePath.stem().string();

        // Use the configured output directory for .mesh/.vmesh files
        std::filesystem::path sourceDir = m_OutputDirectory;

        // Get relative source path for serialization
        std::filesystem::path relativeSourcePath = relative(
            m_SourceFilePath, AssetManager::GetAssetDirectory());

        // Get source mesh UUID
        UUID sourceMeshUUID = AssetManager::GetRegistry().GetUUIDByPath(relativeSourcePath);

        if (m_GenerateVoxelMesh)
        {
            // Create VoxelMeshAsset
            GenerateVoxelMeshAsset(outputName, sourceDir, relativeSourcePath, sourceMeshUUID);
            GenerateProccessedMeshAsset(outputName, sourceDir, relativeSourcePath, sourceMeshUUID);
        }
        else
        {
            // Create ProcessedMeshAsset
            GenerateProccessedMeshAsset(outputName, sourceDir, relativeSourcePath, sourceMeshUUID);
        }

        Close();
    }

    void MeshImportPanel::Close()
    {
        m_IsOpen = false;
        m_Visible = false;

        // Cancel any pending voxelization
        if (m_VoxelTask.IsRunning())
            m_VoxelTask.Cancel();

        // Release CPU data from raw mesh
        if (m_RawMesh)
            m_RawMesh->ReleaseCPUData();

        m_RawMesh = nullptr;
        m_VoxelMesh = nullptr;
    }

    void MeshImportPanel::FitCameraToMesh()
    {
        if (!m_RawMesh || !m_RawMesh->HasCPUData())
        {
            m_RawCameraController.GetCamera().SetPosition({0.0f, 1.0f, 3.0f});
            m_RawCameraController.GetCamera().SetRotation(0.0f, -90.0f);
            return;
        }

        const auto& vertices = m_RawMesh->GetVertices();
        if (vertices.empty())
        {
            m_RawCameraController.GetCamera().SetPosition({0.0f, 1.0f, 3.0f});
            m_RawCameraController.GetCamera().SetRotation(0.0f, -90.0f);
            return;
        }

        // Compute AABB
        Vector3 boundsMin(std::numeric_limits<float>::max());
        Vector3 boundsMax(std::numeric_limits<float>::lowest());
        for (const auto& v : vertices)
        {
            boundsMin = min(boundsMin, v.Position);
            boundsMax = max(boundsMax, v.Position);
        }

        Vector3 center = (boundsMin + boundsMax) * 0.5f;
        Vector3 extents = boundsMax - boundsMin;
        float maxExtent = glm::max(extents.x, glm::max(extents.y, extents.z));

        if (maxExtent < 0.0001f)
            maxExtent = 1.0f;

        // Calculate distance to fit the model in view (FOV = 45 degrees)
        float fovRad = glm::radians(m_RawCameraController.GetFOV());
        float distance = (maxExtent * 0.5f) / std::tan(fovRad * 0.5f);
        distance *= 1.5f; // Padding factor

        // Position camera looking at the center from the front
        Vector3 cameraPos = center + Vector3(0.0f, 0.0f, distance);

        // Compute pitch to look at center
        float pitch = 0.0f;
        float yaw = -90.0f; // Looking toward -Z

        m_RawCameraController.GetCamera().SetPosition(cameraPos);
        m_RawCameraController.GetCamera().SetRotation(pitch, yaw);

        // Adjust clip planes based on model size
        float nearClip = distance * 0.01f;
        float farClip = distance * 10.0f;
        if (nearClip < 0.001f) nearClip = 0.001f;
        m_RawCameraController.SetNearClip(nearClip);
        m_RawCameraController.SetFarClip(farClip);
    }

    void MeshImportPanel::OnEvent(Event& e)
    {
        if (m_IsOpen && m_RawViewportHovered)
            m_RawCameraController.OnEvent(e);
    }
}
