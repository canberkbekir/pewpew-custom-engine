#pragma once
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Asset/Asset.h"
#include "Texture.h"
#include "Shader.h"

namespace CB
{
	class Material : public Asset
	{
	public:
		Material();
		~Material() override = default;

		// Asset interface
		bool Reload() override;
		static AssetType GetStaticType() { return AssetType::Material; }

		// Factory methods
		static Ref<Material> Create();
		static Ref<Material> Load(const String& filePath);

		// Serialization
		bool Save(const String& filePath) const;

		// Texture setters (direct)
		void SetAlbedoMap(const Ref<Texture2D>& texture) { m_AlbedoMap = texture; }
		void SetNormalMap(const Ref<Texture2D>& texture) { m_NormalMap = texture; }
		void SetMetallicMap(const Ref<Texture2D>& texture) { m_MetallicMap = texture; }
		void SetRoughnessMap(const Ref<Texture2D>& texture) { m_RoughnessMap = texture; }

		// Texture setters with path (for Save() support)
		void SetAlbedoMap(const Ref<Texture2D>& texture,const String& path)
		{
			m_AlbedoMap = texture;
			m_AlbedoMapPath = path;
		}

		void SetNormalMap(const Ref<Texture2D>& texture,const String& path)
		{
			m_NormalMap = texture;
			m_NormalMapPath = path;
		}

		void SetMetallicMap(const Ref<Texture2D>& texture,const String& path)
		{
			m_MetallicMap = texture;
			m_MetallicMapPath = path;
		}

		void SetRoughnessMap(const Ref<Texture2D>& texture,const String& path)
		{
			m_RoughnessMap = texture;
			m_RoughnessMapPath = path;
		}

		// Scalar property setters
		void SetAlbedo(const Vector3& color) { m_Albedo = color; }
		void SetMetallic(float value) { m_Metallic = value; }
		void SetRoughness(float value) { m_Roughness = value; }
		void SetSmoothShading(float value) { m_SmoothShading = value; }
		void SetOpacity(float value) { m_Opacity = value; }

		// Getters
		const Ref<Texture2D>& GetAlbedoMap() const { return m_AlbedoMap; }
		const Ref<Texture2D>& GetNormalMap() const { return m_NormalMap; }
		const Ref<Texture2D>& GetMetallicMap() const { return m_MetallicMap; }
		const Ref<Texture2D>& GetRoughnessMap() const { return m_RoughnessMap; }

		const Vector3& GetAlbedo() const { return m_Albedo; }
		float GetMetallic() const { return m_Metallic; }
		float GetRoughness() const { return m_Roughness; }
		float GetSmoothShading() const { return m_SmoothShading; }
		float GetOpacity() const { return m_Opacity; }

		bool HasAlbedoMap() const { return m_AlbedoMap != nullptr; }
		bool HasNormalMap() const { return m_NormalMap != nullptr; }
		bool HasMetallicMap() const { return m_MetallicMap != nullptr; }
		bool HasRoughnessMap() const { return m_RoughnessMap != nullptr; }

		// Bind all textures and upload uniforms to shader
		void Bind(const Ref<Shader>& shader) const;
	private:
		bool LoadFromFile(const String& filePath);

		// Textures
		Ref<Texture2D> m_AlbedoMap;
		Ref<Texture2D> m_NormalMap;
		Ref<Texture2D> m_MetallicMap;
		Ref<Texture2D> m_RoughnessMap;

		// Texture paths (for serialization and reload)
		String m_AlbedoMapPath;
		String m_NormalMapPath;
		String m_MetallicMapPath;
		String m_RoughnessMapPath;

		// Scalar fallbacks
		Vector3 m_Albedo = {1.0f, 1.0f, 1.0f};
		float m_Metallic = 0.0f;
		float m_Roughness = 0.5f;
		float m_SmoothShading = 1.0f;
		float m_Opacity = 1.0f;

		// For reload
		String m_FilePath;
		bool m_IsFromFile = false;
	};
}