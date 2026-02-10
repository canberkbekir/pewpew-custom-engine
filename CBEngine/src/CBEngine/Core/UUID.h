#pragma once

#include <cstdint>
#include <random>
#include <functional>

namespace CB
{
    class UUID
    {
    public:
        UUID()
            : m_UUID(GenerateUUID())
        {
        }

        UUID(uint64_t uuid)
            : m_UUID(uuid)
        {
        }

        UUID(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }

        static UUID Invalid() { return UUID(0); }
        bool IsValid() const { return m_UUID != 0; }

    private:
        static uint64_t GenerateUUID()
        {
            static std::random_device s_RandomDevice;
            static std::mt19937_64 s_Engine(s_RandomDevice());
            static std::uniform_int_distribution<uint64_t> s_Distribution;

            return s_Distribution(s_Engine);
        }

        uint64_t m_UUID;
    };
}

namespace std
{
    template <>
    struct hash<CB::UUID>
    {
        size_t operator()(const CB::UUID& uuid) const
        {
            return hash<uint64_t>{}(uuid);
        }
    };
}
