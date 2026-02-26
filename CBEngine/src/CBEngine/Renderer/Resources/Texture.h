#pragma once
#include "CBEngine/Core/String.h"
#include "CBEngine/Asset/Asset.h"

namespace CB
{
	class Texture : public Asset
	{
	public:
		~Texture() override = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(String path);
		static Ref<Texture2D> Create(uint32_t width,uint32_t height,uint32_t color);
		static Ref<Texture2D> CreateFromData(uint32_t width,uint32_t height,const void* data,bool nearest = false);

		static AssetType GetStaticType() { return AssetType::Texture2D; }
	};
}