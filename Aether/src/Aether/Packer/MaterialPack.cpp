#include "aepch.h"
#include "Aether/Packer/MaterialPack.h"
#include "Aether/Importer/ImporterAPI.h"

namespace Aether
{
    struct BinaryWriter
    {
        std::vector<char> buffer;

        template<typename T>
        void WritePod(const T& v)
        {
            static_assert(std::is_trivially_copyable_v<T>, "Not POD");
            size_t offset = buffer.size();
            buffer.resize(offset + sizeof(T));
            std::memcpy(buffer.data() + offset, &v, sizeof(T));
        }

        void WriteBytes(const void* data, size_t size)
        {
            if (size == 0) return;
            size_t offset = buffer.size();
            buffer.resize(offset + size);
            std::memcpy(buffer.data() + offset, data, size);
        }

        size_t Tell() const { return buffer.size(); }

        void Patch(size_t offset, const void* data, size_t size)
        {
            std::memcpy(buffer.data() + offset, data, size);
        }

        bool WriteToFile(const std::string& path)
        {
            std::ofstream out(path, std::ios::binary);
            if (!out.is_open())
                return false;

            out.write(buffer.data(), buffer.size());
            return true;
        }
    };

    struct BinaryReader
    {
        std::vector<char> buffer;
        size_t offset = 0;

        bool LoadFromFile(const std::string& path)
        {
            std::ifstream in(path, std::ios::binary | std::ios::ate);
            if (!in.is_open())
                return false;

            size_t size = in.tellg();
            buffer.resize(size);

            in.seekg(0);
            in.read(buffer.data(), size);

            offset = 0;
            return true;
        }

        template<typename T>
        void ReadPod(T& v)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            std::memcpy(&v, buffer.data() + offset, sizeof(T));
            offset += sizeof(T);
        }

        void ReadBytes(void* dst, size_t size)
        {
            if (size == 0) return;
            std::memcpy(dst, buffer.data() + offset, size);
            offset += size;
        }

        void Seek(size_t pos)
        {
            offset = pos;
        }
    };

    static void Write(BinaryWriter& w, const std::string& s)
    {
        uint32_t size = (uint32_t)s.size();
        w.WritePod(size);
        if (size)
            w.WriteBytes(s.data(), size);
    }

    template<typename E>
    static std::enable_if_t<std::is_enum_v<E>>
    Write(BinaryWriter& w, E e)
    {
        using U = std::underlying_type_t<E>;
        w.WritePod((U)e);
    }

    static void Write(BinaryWriter& w, const std::vector<uint8_t>& v)
    {
        uint32_t size = (uint32_t)v.size();
        w.WritePod(size);
        if (size)
            w.WriteBytes(v.data(), size);
    }

    template<typename T>
    static void Write(BinaryWriter& w, const std::vector<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        w.WritePod(size);

        for (const auto& e : v)
            Write(w, e);
    }

    static void Write(BinaryWriter& w, const glm::vec4& v)
    {
        w.WritePod(v);
    }

    static void Read(BinaryReader& r, std::string& s)
    {
        uint32_t size;
        r.ReadPod(size);

        s.resize(size);
        if (size)
            r.ReadBytes(s.data(), size);
    }

    template<typename E>
    static std::enable_if_t<std::is_enum_v<E>>
    Read(BinaryReader& r, E& e)
    {
        using U = std::underlying_type_t<E>;
        U value;
        r.ReadPod(value);
        e = (E)value;
    }

    static void Read(BinaryReader& r, std::vector<uint8_t>& v)
    {
        uint32_t size;
        r.ReadPod(size);

        v.resize(size);
        if (size)
            r.ReadBytes(v.data(), size);
    }

    template<typename T>
    static void Read(BinaryReader& r, std::vector<T>& v)
    {
        uint32_t size;
        r.ReadPod(size);

        v.resize(size);
        for (auto& e : v)
            Read(r, e);
    }

    static void Read(BinaryReader& r, glm::vec4& v)
    {
        r.ReadPod(v);
    }

    static void Write(BinaryWriter& w, const TextureSpec& s)
    {
        w.WritePod(s.Width);
        w.WritePod(s.Height);
        w.WritePod(s.Samples);

        Write(w, s.Format);

        uint8_t mips = s.GenerateMips ? 1 : 0;
        w.WritePod(mips);

        Write(w, s.Mode);
    }

    static void Write(BinaryWriter& w, const TextureCreateInfo& t)
    {
        Write(w, t.DebugName);
        Write(w, t.Spec);
        Write(w, t.RawData);
    }

    static void Write(BinaryWriter& w, const MaterialCreateInfo& m)
    {
        w.WritePod(m.AssetID);
        Write(w, m.DebugName);
        Write(w, m.AlbedoColor);

        w.WritePod(m.Metallic);
        w.WritePod(m.Roughness);

        w.WritePod(m.AlbedoMapIdx);
        w.WritePod(m.NormalMapIdx);
        w.WritePod(m.MetallicRoughnessMapIdx);
    }

    static void Read(BinaryReader& r, TextureSpec& s)
    {
        r.ReadPod(s.Width);
        r.ReadPod(s.Height);
        r.ReadPod(s.Samples);

        Read(r, s.Format);

        uint8_t mips;
        r.ReadPod(mips);
        s.GenerateMips = (mips != 0);

        Read(r, s.Mode);
    }

    static void Read(BinaryReader& r, TextureCreateInfo& t)
    {
        Read(r, t.DebugName);
        Read(r, t.Spec);
        Read(r, t.RawData);
    }

    static void Read(BinaryReader& r, MaterialCreateInfo& m)
    {
        r.ReadPod(m.AssetID);
        Read(r, m.DebugName);
        Read(r, m.AlbedoColor);

        r.ReadPod(m.Metallic);
        r.ReadPod(m.Roughness);

        r.ReadPod(m.AlbedoMapIdx);
        r.ReadPod(m.NormalMapIdx);
        r.ReadPod(m.MetallicRoughnessMapIdx);
    }

    bool WriteMatFile(
        const std::string& path,
        const std::vector<TextureCreateInfo>& textures,
        const std::vector<MaterialCreateInfo>& materials)
    {
        BinaryWriter w;

        MatHeader header{};
        size_t headerPos = w.Tell();
        w.WritePod(header); // placeholder

        // Textures
        header.TextureOffset = w.Tell();
        header.TextureCount = (uint32_t)textures.size();
        Write(w, textures);

        // Materials
        header.MaterialOffset = w.Tell();
        header.MaterialCount = (uint32_t)materials.size();
        Write(w, materials);

        // Patch header
        w.Patch(headerPos, &header, sizeof(header));

        return w.WriteToFile(path);
    }

    bool ReadMatFile(
        const std::string& path,
        std::vector<TextureCreateInfo>& outTextures,
        std::vector<MaterialCreateInfo>& outMaterials)
    {
        BinaryReader r;
        if (!r.LoadFromFile(path))
            return false;

        MatHeader header{};
        r.ReadPod(header);

        if (header.Magic != 'MATF') return false;

        r.Seek((size_t)header.TextureOffset);
        outTextures.clear();
        Read(r, outTextures);

        r.Seek((size_t)header.MaterialOffset);
        outMaterials.clear();
        Read(r, outMaterials);

        return true;
    }

} 