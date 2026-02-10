#include "cbpch.h"
#include "VoxelPalette.h"
#include "CBEngine/Utils/BinaryIO.h"

#include <algorithm>
#include <cmath>

namespace CB
{
    VoxelPalette::VoxelPalette()
    {
        // Entry 0 = default white
        m_Entries[0] = {Vector3(1.0f), 0.0f, 0.5f, 0.0f};
    }

    uint8_t VoxelPalette::FindClosestEntry(const Vector3& color) const
    {
        uint8_t bestIdx = 0;
        float bestDist = std::numeric_limits<float>::max();

        for (uint32_t i = 0; i < m_UsedCount; i++)
        {
            Vector3 diff = m_Entries[i].Color - color;
            float dist = dot(diff, diff);
            if (dist < bestDist)
            {
                bestDist = dist;
                bestIdx = static_cast<uint8_t>(i);
            }
        }

        return bestIdx;
    }

    uint8_t VoxelPalette::AddEntry(const VoxelPaletteEntry& entry)
    {
        if (m_UsedCount >= MaxEntries)
            return FindClosestEntry(entry.Color);

        uint8_t idx = static_cast<uint8_t>(m_UsedCount);
        m_Entries[idx] = entry;
        m_UsedCount++;
        return idx;
    }

    void VoxelPalette::SetEntry(uint8_t index, const VoxelPaletteEntry& entry)
    {
        m_Entries[index] = entry;
        if (static_cast<uint32_t>(index) >= m_UsedCount)
            m_UsedCount = static_cast<uint32_t>(index) + 1;
    }

    void VoxelPalette::GenerateColorTextureData(std::vector<uint8_t>& outRGBA) const
    {
        outRGBA.resize(MaxEntries * 4);
        for (uint32_t i = 0; i < MaxEntries; i++)
        {
            const auto& e = m_Entries[i];
            outRGBA[i * 4 + 0] = static_cast<uint8_t>(std::clamp(e.Color.r, 0.0f, 1.0f) * 255.0f + 0.5f);
            outRGBA[i * 4 + 1] = static_cast<uint8_t>(std::clamp(e.Color.g, 0.0f, 1.0f) * 255.0f + 0.5f);
            outRGBA[i * 4 + 2] = static_cast<uint8_t>(std::clamp(e.Color.b, 0.0f, 1.0f) * 255.0f + 0.5f);
            outRGBA[i * 4 + 3] = 255;
        }
    }

    void VoxelPalette::GenerateMaterialTextureData(std::vector<uint8_t>& outRGBA) const
    {
        outRGBA.resize(MaxEntries * 4);
        for (uint32_t i = 0; i < MaxEntries; i++)
        {
            const auto& e = m_Entries[i];
            outRGBA[i * 4 + 0] = static_cast<uint8_t>(std::clamp(e.Metallic, 0.0f, 1.0f) * 255.0f + 0.5f);
            outRGBA[i * 4 + 1] = static_cast<uint8_t>(std::clamp(e.Roughness, 0.0f, 1.0f) * 255.0f + 0.5f);
            outRGBA[i * 4 + 2] = static_cast<uint8_t>(std::clamp(e.Emission, 0.0f, 1.0f) * 255.0f + 0.5f);
            outRGBA[i * 4 + 3] = 255;
        }
    }

    void VoxelPalette::Serialize(BinaryWriter& writer) const
    {
        writer << m_UsedCount;
        for (uint32_t i = 0; i < m_UsedCount; i++)
        {
            const auto& e = m_Entries[i];
            writer << e.Color.r;
            writer << e.Color.g;
            writer << e.Color.b;
            writer << e.Metallic;
            writer << e.Roughness;
            writer << e.Emission;
        }
    }

    void VoxelPalette::Deserialize(BinaryReader& reader)
    {
        reader >> m_UsedCount;
        if (m_UsedCount > MaxEntries)
            m_UsedCount = MaxEntries;

        for (uint32_t i = 0; i < m_UsedCount; i++)
        {
            auto& e = m_Entries[i];
            reader >> e.Color.r;
            reader >> e.Color.g;
            reader >> e.Color.b;
            reader >> e.Metallic;
            reader >> e.Roughness;
            reader >> e.Emission;
        }
    }
}
