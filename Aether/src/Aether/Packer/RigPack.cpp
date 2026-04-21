#include "aepch.h"
#include "Aether/Packer/RigPack.h"
#include "Aether/Importer/AnimationParser.h"

#include <fstream>
#include <cstring>
#include <type_traits>

namespace Aether
{
    struct BW
    {
        std::vector<char> buf;

        template<typename T>
        void pod(const T& v)
        {
            static_assert(std::is_trivially_copyable_v<T>);
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
        uint32_t n;
        r.pod(n);
        s.resize(n);
        if (n) r.bytes(s.data(), n);
    }

    template<typename T>
    static void W(BW& w, const std::vector<T>& v)
    {
        uint32_t n = (uint32_t)v.size();
        w.pod(n);

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            if (n) w.bytes(v.data(), n * sizeof(T));
        }
        else
        {
            for (const auto& e : v)
                W(w, e);
        }
    }

    template<typename T>
    static void R(BR& r, std::vector<T>& v)
    {
        uint32_t n;
        r.pod(n);

        v.resize(n);

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            if (n) r.bytes(v.data(), n * sizeof(T));
        }
        else
        {
            for (auto& e : v)
                R(r, e);
        }
    }

    static void W(BW& w, const glm::vec3& v)
    {
        w.pod(v.x); w.pod(v.y); w.pod(v.z);
    }

    static void R(BR& r, glm::vec3& v)
    {
        r.pod(v.x); r.pod(v.y); r.pod(v.z);
    }

    static void W(BW& w, const glm::quat& q)
    {
        w.pod(q.x); w.pod(q.y); w.pod(q.z); w.pod(q.w);
    }

    static void R(BR& r, glm::quat& q)
    {
        r.pod(q.x); r.pod(q.y); r.pod(q.z); r.pod(q.w);
    }

    static void W(BW& w, const glm::mat4& m)
    {
        w.bytes(&m[0][0], sizeof(float) * 16);
    }

    static void R(BR& r, glm::mat4& m)
    {
        r.bytes(&m[0][0], sizeof(float) * 16);
    }

    static void W(BW& w, const SkeletonSpec::Joint& j)
    {
        W(w, j.Name);
        w.pod(j.ParentIndex);
        W(w, j.Translation);
        W(w, j.Rotation);
        W(w, j.Scale);
    }

    static void R(BR& r, SkeletonSpec::Joint& j)
    {
        R(r, j.Name);
        r.pod(j.ParentIndex);
        R(r, j.Translation);
        R(r, j.Rotation);
        R(r, j.Scale);
    }

    static void W(BW& w, const SkeletonSpec& s)
    {
        W(w, s.Joints);
        W(w, s.IBM);
    }

    static void R(BR& r, SkeletonSpec& s)
    {
        R(r, s.Joints);
        R(r, s.IBM);
    }

    static void W(BW& w, const RigCreateInfo& rci)
    {
        w.pod(rci.AssetID);
        W(w, rci.DebugName);
        W(w, rci.spec);
    }

    static void R(BR& r, RigCreateInfo& rci)
    {
        r.pod(rci.AssetID);
        R(r, rci.DebugName);
        R(r, rci.spec);
    }

    static void W(BW& w, const ClipSpec::Track& t)
    {
        w.pod(t.JointIndex);
        W(w, t.TranslationTimes);
        W(w, t.TranslationValues);
        W(w, t.RotationTimes);
        W(w, t.RotationValues);
        W(w, t.ScaleTimes);
        W(w, t.ScaleValues);
    }

    static void R(BR& r, ClipSpec::Track& t)
    {
        r.pod(t.JointIndex);
        R(r, t.TranslationTimes);
        R(r, t.TranslationValues);
        R(r, t.RotationTimes);
        R(r, t.RotationValues);
        R(r, t.ScaleTimes);
        R(r, t.ScaleValues);
    }

    static void W(BW& w, const ClipSpec& c)
    {
        w.pod(c.Duration);
        w.pod(c.SampleRate);
        W(w, c.Tracks);
    }

    static void R(BR& r, ClipSpec& c)
    {
        r.pod(c.Duration);
        r.pod(c.SampleRate);
        R(r, c.Tracks);
    }

    static void W(BW& w, const ClipCreateInfo& c)
    {
        w.pod(c.AssetID);
        W(w, c.DebugName);
        w.pod(c.rigIdx);
        W(w, c.spec);
    }

    static void R(BR& r, ClipCreateInfo& c)
    {
        r.pod(c.AssetID);
        R(r, c.DebugName);
        r.pod(c.rigIdx);
        R(r, c.spec);
    }

    bool WriteRigFile(const std::string& path, const std::vector<RigCreateInfo>& rigs, const std::vector<ClipCreateInfo>& clips)
    {
        BW w;

        RigHeader h{};
        size_t hp = w.tell();
        w.pod(h);

        h.RigOffset = w.tell();
        h.RigCount = (uint32_t)rigs.size();
        W(w, rigs);

        h.ClipOffset = w.tell();
        h.ClipCount = (uint32_t)clips.size();
        W(w, clips);

        w.patch(hp, &h, sizeof(h));
        return w.save(path);
    }

    bool ReadRigFile(const std::string& path, std::vector<RigCreateInfo>& rigs, std::vector<ClipCreateInfo>& clips)
    {
        BR r;
        if (!r.load(path)) return false;

        RigHeader h{};
        r.pod(h);

        if (h.Magic != 'RIGF') return false;
        if (h.Version != 1) return false;
        if (h.RigOffset >= r.buf.size()) return false;
        if (h.ClipOffset >= r.buf.size()) return false;

        r.seek(h.RigOffset);
        R(r, rigs);

        r.seek(h.ClipOffset);
        R(r, clips);

        // validate
        for (auto& c : clips)
        {
            if (c.rigIdx >= (int)rigs.size() && c.rigIdx != -1)
                return false;
        }

        return true;
    }

}