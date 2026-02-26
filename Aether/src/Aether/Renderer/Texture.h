#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Assets/Asset.h"
#include <unordered_map>

namespace Aether {

	enum class ImageFormat
	{
		None = 0,
		RGB8,
		RGBA8,
        RGBA16F,
		RGBA32F, 
		RED_INTEGER,
		DEPTH24STENCIL8,
		Depth = DEPTH24STENCIL8
	};

	enum class WrapMode
	{
		None = 0, 
		REPEAT, CLAMP_TO_EDGE
	};

	struct TextureSpec
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		int Samples = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = false;
        WrapMode Mode = WrapMode::REPEAT;

		TextureSpec() = default;
		TextureSpec(ImageFormat format)
        	: Format(format) {}
	};

	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual void SetData(const void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;

		static const AssetType GetType() { return AssetType::Texture; }
        virtual const AssetType GetAssetType() const override { return AssetType::Texture; }
	};

	class AETHER_API Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureSpec& spec);
		static Ref<Texture2D> Create(void* data, size_t size);
		static Ref<Texture2D> Create(const std::string& path, WrapMode mode = WrapMode::REPEAT, bool flip = true);
	};

    class AETHER_API TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(const std::string& path);
	};
}
