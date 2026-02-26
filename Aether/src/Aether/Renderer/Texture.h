#pragma once

#include "aepch.h"
#include "Aether/Core/UUID.h"

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

	class Texture
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

	class AETHER_API Texture2DLibrary
    {
    public:
        static void Init();
        static void Shutdown();

		static void Add(Ref<Texture2D> obj, UUID id);
        static Ref<Texture2D> Get(UUID id);
        
        static bool Exists(UUID id);
    private:
        static std::unordered_map<UUID, Ref<Texture2D>>& GetTextures();
    };
}
