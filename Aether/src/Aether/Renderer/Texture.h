#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Renderer/Resource.h"

namespace Aether {

	class ResourceManager;

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

	class Texture : public Resource
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
		template<typename... Args>
        static Ref<Texture2D> Create(Args&&... args)
        {
            Scope<Texture2D> scope = CreateImpl(std::forward<Args>(args)...);
            return Ref<Texture2D>(std::move(scope));
        }

	private:
		static Scope<Texture2D> CreateImpl(const TextureSpec& spec);
		static Scope<Texture2D> CreateImpl(void* data, size_t size);
		static Scope<Texture2D> CreateImpl(const std::string& path, WrapMode mode = WrapMode::REPEAT, bool flip = true);

		friend class ResourceManager;
		friend class FrameBuffer;
	};

    class AETHER_API TextureCube : public Texture
	{
	public:
		template<typename... Args>
        static Ref<Texture2D> Create(Args&&... args)
        {
            Scope<Texture2D> scope = CreateImpl(std::forward<Args>(args)...);
            return Ref<Texture2D>(std::move(scope));
        }
	private:
		static Scope<TextureCube> CreateImpl(const std::string& path);
		friend class ResourceManager;
	};
}
