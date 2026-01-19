#pragma once

#include "aepch.h"

namespace Aether {

	enum class ImageFormat
	{
		None = 0,
		RGB8,
		RGBA8,
        RGBA16F,
		RGBA32F
	};

	struct TextureSpec
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;

        GLenum WrapMode = GL_REPEAT;
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual const TextureSpec& GetSpec() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual void SetData(const void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureSpec& spec);
		static Ref<Texture2D> Create(const std::string& path, bool wrapMode = false, bool flip = true);
	};

    class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(const std::string& path);
	};
}
