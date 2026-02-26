#pragma once
#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Asset/Asset.h"

namespace CB
{
	class Shader : public Asset
	{
	public:
		~Shader() override = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const String& GetName() const = 0;

		// Uniform setters
		virtual void SetInt(const String& name,int value) = 0;
		virtual void SetFloat(const String& name,float value) = 0;
		virtual void SetFloat2(const String& name,const Vector2& value) = 0;
		virtual void SetFloat3(const String& name,const Vector3& value) = 0;
		virtual void SetFloat4(const String& name,const Vector4& value) = 0;
		virtual void SetMat3(const String& name,const Mat3& matrix) = 0;
		virtual void SetMat4(const String& name,const Mat4& matrix) = 0;

		static Ref<Shader> Create(const String& filePath);
		static Ref<Shader> Create(const String& name,const String& vertexSrc,const String& fragmentSrc);

		static AssetType GetStaticType() { return AssetType::Shader; }
	};

	class ShaderLibrary
	{
	public:
		void Add(const String& name,const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> Load(const String& filepath);
		Ref<Shader> Load(const String& name,const String& filepath);

		Ref<Shader> Get(const String& name);

		bool Exists(const String& name) const;
	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
	};
}