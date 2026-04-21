#include "aepch.h"
#include "Aether/Packer/MeshPack.h"
#include "Aether/Importer/MeshParser.h"


namespace Aether
{
    struct BW
    {
        std::vector<char> buf;

        template<typename T>
        void pod(const T& v)
        {
            size_t o = buf.size();
            buf.resize(o + sizeof(T));
            memcpy(buf.data() + o, &v, sizeof(T));
        }

        void bytes(const void* d, size_t s)
        {
            if (!s) return;
            size_t o = buf.size();
            buf.resize(o + s);
            memcpy(buf.data() + o, d, s);
        }

        size_t tell() const { return buf.size(); }

        void patch(size_t off, const void* d, size_t s)
        {
            memcpy(buf.data() + off, d, s);
        }

        bool save(const std::string& p)
        {
            std::ofstream o(p, std::ios::binary);
            if (!o) return false;
            o.write(buf.data(), buf.size());
            return true;
        }
    };

    struct BR
    {
        std::vector<char> buf;
        size_t off = 0;

        bool load(const std::string& p)
        {
            std::ifstream i(p, std::ios::binary | std::ios::ate);
            if (!i) return false;
            size_t s = i.tellg();
            buf.resize(s);
            i.seekg(0);
            i.read(buf.data(), s);
            off = 0;
            return true;
        }

        template<typename T>
        void pod(T& v)
        {
            memcpy(&v, buf.data() + off, sizeof(T));
            off += sizeof(T);
        }

        void bytes(void* d, size_t s)
        {
            if (!s) return;
            memcpy(d, buf.data() + off, s);
            off += s;
        }

        void seek(size_t p) { off = p; }
    };

    static void W(BW& w, const std::string& s)
    {
        uint32_t n = (uint32_t)s.size();
        w.pod(n);
        if (n) w.bytes(s.data(), n);
    }

    static void R(BR& r, std::string& s)
    {
        uint32_t n; r.pod(n);
        s.resize(n);
        if (n) r.bytes(s.data(), n);
    }

    template<typename T>
    static void W(BW& w, const std::vector<T>& v)
    {
        uint32_t n = (uint32_t)v.size();
        w.pod(n);

        if constexpr (std::is_trivially_copyable_v<T>)
            w.bytes(v.data(), n * sizeof(T));
        else
            for (auto& e : v) W(w, e);
    }

    template<typename T>
    static void R(BR& r, std::vector<T>& v)
    {
        uint32_t n; r.pod(n);
        v.resize(n);

        if constexpr (std::is_trivially_copyable_v<T>)
            r.bytes(v.data(), n * sizeof(T));
        else
            for (auto& e : v) R(r, e);
    }

    static void W(BW& w, const SubMeshCreateInfo& s)
    {
        W(w, s.NodeName);
        w.pod(s.VertexCount);
        w.pod(s.IndexCount);
        w.pod(s.BaseVertex);
        w.pod(s.BaseIndex);
        w.pod(s.BoundsMin);
        w.pod(s.BoundsMax);
        w.pod(s.MaterialIdx);
    }

    static void R(BR& r, SubMeshCreateInfo& s)
    {
        R(r, s.NodeName);
        r.pod(s.VertexCount);
        r.pod(s.IndexCount);
        r.pod(s.BaseVertex);
        r.pod(s.BaseIndex);
        r.pod(s.BoundsMin);
        r.pod(s.BoundsMax);
        r.pod(s.MaterialIdx);
    }

    static void W(BW& w, const MeshCreateInfo& m)
    {
        w.pod(m.AssetID);
        W(w, m.DebugName);

        W(w, m.Positions);
        W(w, m.Normals);
        W(w, m.Tangents);
        W(w, m.TexCoords);
        W(w, m.Indices);

        W(w, m.Weights);
        W(w, m.Joints);

        W(w, m.SubMeshes);

        w.pod(m.totalVertices);
        w.pod(m.totalIndices);
    }

    static void R(BR& r, MeshCreateInfo& m)
    {
        r.pod(m.AssetID);
        R(r, m.DebugName);

        R(r, m.Positions);
        R(r, m.Normals);
        R(r, m.Tangents);
        R(r, m.TexCoords);
        R(r, m.Indices);

        R(r, m.Weights);
        R(r, m.Joints);

        R(r, m.SubMeshes);

        r.pod(m.totalVertices);
        r.pod(m.totalIndices);
    }

    bool WriteMeshFile(const std::string& path, const std::vector<MeshCreateInfo>& meshes)
    {
        BW w;
        MeshHeader h{};
        size_t hp = w.tell();
        w.pod(h);

        h.MeshOffset = w.tell();
        h.MeshCount = (uint32_t)meshes.size();
        W(w, meshes);

        w.patch(hp, &h, sizeof(h));
        return w.save(path);
    }

    bool ReadMeshFile(const std::string& path, std::vector<MeshCreateInfo>& out)
    {
        BR r;
        if (!r.load(path)) return false;

        MeshHeader h{};
        r.pod(h);

        if (h.Magic != 'MESF') return false;

        r.seek(h.MeshOffset);
        R(r, out);

        return true;
    }
}