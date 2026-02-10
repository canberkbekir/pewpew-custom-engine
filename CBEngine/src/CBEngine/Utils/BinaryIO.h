#pragma once

#include <cstdio>
#include <cstdint>
#include <type_traits>
#include <limits>

#include "CBEngine/Core/String.h"

namespace CB
{
    // ------------------------------------------------------------
    // BinaryStream
    // ------------------------------------------------------------
    class BinaryStream
    {
    public:
        virtual ~BinaryStream() = default;

        // Return bytes actually written/read (0 on failure).
        virtual size_t Write(const void* data, size_t size) = 0;
        virtual size_t Read(void* data, size_t size) = 0;

        virtual uint64_t Tell() const = 0;
        virtual bool Seek(uint64_t pos) = 0;

        virtual bool IsOpen() const = 0;

        virtual void Flush()
        {
        }
    };

    // ------------------------------------------------------------
    // FileStream (Win64)
    // ------------------------------------------------------------
    class FileStream final : public BinaryStream
    {
    public:
        FileStream() = default;

        FileStream(const char* path, const char* mode)
        {
            Open(path, mode);
        }

        ~FileStream() override
        {
            Close();
        }

        FileStream(const FileStream&) = delete;
        FileStream& operator=(const FileStream&) = delete;

        FileStream(FileStream&& other) noexcept
            : m_File(other.m_File)
        {
            other.m_File = nullptr;
        }

        FileStream& operator=(FileStream&& other) noexcept
        {
            if (this == &other) return *this;
            Close();
            m_File = other.m_File;
            other.m_File = nullptr;
            return *this;
        }

        bool Open(const char* path, const char* mode)
        {
            Close();
            m_File = std::fopen(path, mode);
            if (!m_File)
            {
                CB_CORE_ERROR("FileStream: failed to open file '{}' with mode '{}'",
                              path ? path : "(null)", mode ? mode : "(null)");
                return false;
            }
            return true;
        }

        void Close()
        {
            if (m_File)
            {
                std::fclose(m_File);
                m_File = nullptr;
            }
        }

        bool IsOpen() const override { return m_File != nullptr; }

        void Flush() override
        {
            if (m_File) std::fflush(m_File);
        }

        size_t Write(const void* data, size_t size) override
        {
            if (!m_File || !data || size == 0)
                return 0;

            const size_t written = std::fwrite(data, 1, size, m_File);
            if (written != size)
            {
                CB_CORE_ERROR("FileStream: fwrite wrote {} / {} bytes", static_cast<uint64_t>(written), size);
            }
            return written;
        }

        size_t Read(void* data, size_t size) override
        {
            if (!m_File || !data || size == 0)
                return 0;

            const size_t read = std::fread(data, 1, size, m_File);
            if (read != size)
            {
                if (std::ferror(m_File))
                    CB_CORE_ERROR("FileStream: fread read {} / {} bytes (ferror set)", static_cast<uint64_t>(read),
                              size);
            }
            return read;
        }

        uint64_t Tell() const override
        {
            if (!m_File) return 0;
            const auto p = _ftelli64(m_File);
            return (p < 0) ? 0ull : static_cast<uint64_t>(p);
        }

        bool Seek(uint64_t pos) override
        {
            if (!m_File) return false;
            const bool ok = _fseeki64(m_File, static_cast<long long>(pos), SEEK_SET) == 0;
            if (!ok)
                CB_CORE_ERROR("FileStream: seek failed (pos={})", pos);
            return ok;
        }

        std::FILE* GetHandle() const { return m_File; }

    private:
        std::FILE* m_File = nullptr;
    };

    // ------------------------------------------------------------
    // BinaryWriter / BinaryReader
    // ------------------------------------------------------------
    class BinaryWriter
    {
    public:
        explicit BinaryWriter(BinaryStream& s) : m_Stream(s)
        {
        }

        bool IsValid() const { return m_Stream.IsOpen(); }

        uint64_t Tell() const { return m_Stream.Tell(); }
        bool Seek(uint64_t pos) { return m_Stream.Seek(pos); }

        void WriteBytes(const void* data, size_t size)
        {
            if (size == 0) return;
            const size_t written = m_Stream.Write(data, size);
            if (written != size)
                CB_CORE_ERROR("BinaryWriter: failed to write {} bytes", size);
        }

        template <typename T>
        void Write(const T& v)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                WriteBytes(&v, sizeof(T));
            }
            else
            {
                // Non-trivial types must provide:
                //   void Serialize(BinaryWriter&) const;
                v.Serialize(*this);
            }
        }

    private:
        BinaryStream& m_Stream;
    };

    class BinaryReader
    {
    public:
        explicit BinaryReader(BinaryStream& s) : m_Stream(s)
        {
        }

        bool IsValid() const { return m_Stream.IsOpen(); }

        uint64_t Tell() const { return m_Stream.Tell(); }
        bool Seek(uint64_t pos) { return m_Stream.Seek(pos); }

        void ReadBytes(void* data, size_t size)
        {
            if (size == 0) return;
            const size_t read = m_Stream.Read(data, size);
            if (read != size)
                CB_CORE_ERROR("BinaryReader: failed to read {} bytes", size);
        }

        template <typename T>
        void Read(T& v)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                ReadBytes(&v, sizeof(T));
            }
            else
            {
                // Non-trivial types must provide:
                //   void Deserialize(BinaryReader&);
                v.Deserialize(*this);
            }
        }

    private:
        BinaryStream& m_Stream;
    };

    // ------------------------------------------------------------
    // Operators (generic)
    // ------------------------------------------------------------
    template <typename T>
    BinaryWriter& operator<<(BinaryWriter& ar, const T& v)
    {
        ar.Write(v);
        return ar;
    }

    template <typename T>
    BinaryReader& operator>>(BinaryReader& ar, T& v)
    {
        ar.Read(v);
        return ar;
    }

    // ------------------------------------------------------------
    // std::vector serialization (count + elements)
    // ------------------------------------------------------------
    template <typename T>
    BinaryWriter& operator<<(BinaryWriter& ar, const std::vector<T>& arr)
    {
        const uint64_t count64 = static_cast<uint64_t>(arr.size());
        if (count64 > std::numeric_limits<uint32_t>::max())
        {
            CB_CORE_ERROR("BinaryWriter: vector too large to serialize (count={})", count64);
            uint32_t zero = 0;
            ar << zero;
            return ar;
        }

        const uint32_t count = static_cast<uint32_t>(count64);
        ar << count;

        if (count == 0) return ar;

        // Note: std::vector<bool> is a special case (no real contiguous bool*).
        if constexpr (std::is_trivially_copyable_v<T> && !std::is_same_v<T, bool>)
        {
            ar.WriteBytes(arr.data(), static_cast<size_t>(count) * sizeof(T));
        }
        else
        {
            for (uint32_t i = 0; i < count; ++i)
                ar << arr[i];
        }

        return ar;
    }

    template <typename T>
    BinaryReader& operator>>(BinaryReader& ar, std::vector<T>& arr)
    {
        uint32_t count = 0;
        ar >> count;

        arr.clear();
        arr.resize(static_cast<size_t>(count));

        if (count == 0) return ar;

        if constexpr (std::is_trivially_copyable_v<T> && !std::is_same_v<T, bool>)
        {
            ar.ReadBytes(arr.data(), static_cast<size_t>(count) * sizeof(T));
        }
        else
        {
            for (uint32_t i = 0; i < count; ++i)
                ar >> arr[static_cast<size_t>(i)];
        }

        return ar;
    }

    // ------------------------------------------------------------
    // String serialization (len + bytes, no null terminator)
    // ------------------------------------------------------------
    inline BinaryWriter& operator<<(BinaryWriter& ar, const String& s)
    {
        const uint64_t len64 = s.length();
        if (len64 > std::numeric_limits<uint32_t>::max())
        {
            CB_CORE_ERROR("BinaryWriter: String too large to serialize (len={})", len64);
            uint32_t zero = 0;
            ar << zero;
            return ar;
        }

        const uint32_t len = static_cast<uint32_t>(len64);
        ar << len;

        if (len > 0)
            ar.WriteBytes(s.data(), len);

        return ar;
    }

    inline BinaryReader& operator>>(BinaryReader& ar, String& s)
    {
        uint32_t len = 0;
        ar >> len;

        s.clear();
        if (len == 0) return ar;

        s.resize(len);
        ar.ReadBytes(s.data(), len);
        return ar;
    }
} // namespace CB
