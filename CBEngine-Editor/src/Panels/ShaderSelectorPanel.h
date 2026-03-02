#pragma once

#include "Panel.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/Asset.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include <imgui.h>
#include <functional>
#include <filesystem>

namespace CB
{
	class ShaderSelectorPanel : public Panel
	{
	public:
		ShaderSelectorPanel()
			: Panel("Change Shader", false)
		{
		}

		std::function<void(UUID)> OnShaderSelected;

		void SetActiveShader(UUID uuid) { m_ActiveShaderUUID = uuid; }

		void OnImGuiRender() override
		{
			if (!m_Visible)
				return;

			ImGui::Begin(m_Name.c_str(), &m_Visible);

			ImGui::Text("Select a shader:");
			ImGui::Separator();

			const auto& allAssets = AssetManager::GetRegistry().GetAllAssets();
			for (const auto& [uuid, meta] : allAssets)
			{
				if (meta.Type != AssetType::Shader)
					continue;

				std::string filename = meta.FilePath.filename().string();
				bool isSelected = (uuid == m_ActiveShaderUUID);

				if (ImGui::Selectable(filename.c_str(), isSelected))
				{
					m_ActiveShaderUUID = uuid;
					if (OnShaderSelected)
						OnShaderSelected(uuid);
				}
			}

			ImGui::End();
		}

	private:
		UUID m_ActiveShaderUUID;
	};
}
