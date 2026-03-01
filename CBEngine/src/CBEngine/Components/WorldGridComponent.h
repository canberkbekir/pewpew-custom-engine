#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/YAMLHelpers.h"
#include "CBEngine/Voxel/World/WorldGrid.h"
#include "CBEngine/Voxel/World/WorldGenerator.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"

namespace YAML
{
	class Emitter;
	class Node;
}

namespace CB
{
	struct WorldGridComponent
	{
		static constexpr auto YAMLKey = "WorldGridComponent";

		// Grid
		float VoxelSize = 0.25f;

		// World size in chunks (from origin)
		int SizeX = 8;  // -SizeX..+SizeX chunks on X axis
		int SizeY = 4;  // -SizeY..+SizeY chunks on Y axis
		int SizeZ = 8;  // -SizeZ..+SizeZ chunks on Z axis

		// Simulation region
		float SimRadius = 128.0f;
		float RenderRadius = 256.0f;
		float PhysicsRadius = 160.0f;
		float LOD1Distance = 160.0f;
		float LOD2Distance = 224.0f;
		float StreamRadius = 320.0f;
		float UnloadRadius = 384.0f;

		// Generation
		int GeneratorType = 0; // 0=None, 1=Flat, 2=Noise
		uint32_t Seed = 42;
		int SeaLevel = 0;
		float NoiseScale = 0.02f;
		int NoiseOctaves = 4;
		float NoiseAmplitude = 32.0f;
		std::string SurfaceSubstance = "";  // Resolved from SubstanceRegistry at use time
		std::string FillSubstance = "";

		// Storage path for saving/loading .cbwg files
		std::string StoragePath = "";

		// Runtime (not serialized)
		bool Building = false;

		WorldGridComponent() = default;
		WorldGridComponent(const WorldGridComponent&) = default;

		// Total expected chunks based on size
		size_t GetExpectedChunkCount() const
		{
			return static_cast<size_t>(SizeX * 2 + 1) * (SizeY * 2 + 1) * (SizeZ * 2 + 1);
		}

		// World extent in world units (for display)
		Vector3 GetWorldExtent() const
		{
			float chunkWorldSize = CHUNK_SIZE * VoxelSize;
			return Vector3(
				(SizeX * 2 + 1) * chunkWorldSize,
				(SizeY * 2 + 1) * chunkWorldSize,
				(SizeZ * 2 + 1) * chunkWorldSize
			);
		}

		void ApplyToGrid(WorldGrid& grid) const
		{
			grid.VoxelSize = VoxelSize;

			// Set world bounds
			grid.HasBounds = true;
			grid.MinChunkCoord = glm::ivec3(-SizeX, -SizeY, -SizeZ);
			grid.MaxChunkCoord = glm::ivec3(SizeX, SizeY, SizeZ);

			grid.Region.SimRadius = SimRadius;
			grid.Region.RenderRadius = RenderRadius;
			grid.Region.PhysicsRadius = PhysicsRadius;
			grid.Region.LOD1Distance = LOD1Distance;
			grid.Region.LOD2Distance = LOD2Distance;
			grid.StreamRadius = StreamRadius;
			grid.UnloadRadius = UnloadRadius;

			grid.GenConfig.GeneratorType = static_cast<WorldGenConfig::Type>(GeneratorType);
			grid.GenConfig.Seed = Seed;
			grid.GenConfig.SeaLevel = SeaLevel;
			grid.GenConfig.NoiseScale = NoiseScale;
			grid.GenConfig.NoiseOctaves = NoiseOctaves;
			grid.GenConfig.NoiseAmplitude = NoiseAmplitude;
			grid.GenConfig.SurfaceSubstance = SubstanceRegistry::ResolveID(SurfaceSubstance);
			grid.GenConfig.FillSubstance = SubstanceRegistry::ResolveID(FillSubstance);

			// Set storage path for save/load
			if (!StoragePath.empty())
				grid.SetStoragePath(StoragePath);

			// Set generator callback if type != None
			if (GeneratorType != 0)
			{
				grid.Generator = [](WorldChunk& chunk, glm::ivec3 coord, WorldGrid& g)
				{
					WorldGenerator::Generate(chunk, coord, g.GenConfig, g);
				};
			}
			else
			{
				grid.Generator = nullptr;
			}
		}

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "VoxelSize" << YAML::Value << VoxelSize;
			out << YAML::Key << "SizeX" << YAML::Value << SizeX;
			out << YAML::Key << "SizeY" << YAML::Value << SizeY;
			out << YAML::Key << "SizeZ" << YAML::Value << SizeZ;
			out << YAML::Key << "SimRadius" << YAML::Value << SimRadius;
			out << YAML::Key << "RenderRadius" << YAML::Value << RenderRadius;
			out << YAML::Key << "PhysicsRadius" << YAML::Value << PhysicsRadius;
			out << YAML::Key << "LOD1Distance" << YAML::Value << LOD1Distance;
			out << YAML::Key << "LOD2Distance" << YAML::Value << LOD2Distance;
			out << YAML::Key << "StreamRadius" << YAML::Value << StreamRadius;
			out << YAML::Key << "UnloadRadius" << YAML::Value << UnloadRadius;
			out << YAML::Key << "GeneratorType" << YAML::Value << GeneratorType;
			out << YAML::Key << "Seed" << YAML::Value << Seed;
			out << YAML::Key << "SeaLevel" << YAML::Value << SeaLevel;
			out << YAML::Key << "NoiseScale" << YAML::Value << NoiseScale;
			out << YAML::Key << "NoiseOctaves" << YAML::Value << NoiseOctaves;
			out << YAML::Key << "NoiseAmplitude" << YAML::Value << NoiseAmplitude;
			out << YAML::Key << "SurfaceSubstance" << YAML::Value << SurfaceSubstance;
			out << YAML::Key << "FillSubstance" << YAML::Value << FillSubstance;
			out << YAML::Key << "StoragePath" << YAML::Value << StoragePath;
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["VoxelSize"]) VoxelSize = node["VoxelSize"].as<float>();
			if (node["SizeX"]) SizeX = node["SizeX"].as<int>();
			if (node["SizeY"]) SizeY = node["SizeY"].as<int>();
			if (node["SizeZ"]) SizeZ = node["SizeZ"].as<int>();
			if (node["SimRadius"]) SimRadius = node["SimRadius"].as<float>();
			if (node["RenderRadius"]) RenderRadius = node["RenderRadius"].as<float>();
			if (node["PhysicsRadius"]) PhysicsRadius = node["PhysicsRadius"].as<float>();
			if (node["LOD1Distance"]) LOD1Distance = node["LOD1Distance"].as<float>();
			if (node["LOD2Distance"]) LOD2Distance = node["LOD2Distance"].as<float>();
			if (node["StreamRadius"]) StreamRadius = node["StreamRadius"].as<float>();
			if (node["UnloadRadius"]) UnloadRadius = node["UnloadRadius"].as<float>();
			if (node["GeneratorType"]) GeneratorType = node["GeneratorType"].as<int>();
			if (node["Seed"]) Seed = node["Seed"].as<uint32_t>();
			if (node["SeaLevel"]) SeaLevel = node["SeaLevel"].as<int>();
			if (node["NoiseScale"]) NoiseScale = node["NoiseScale"].as<float>();
			if (node["NoiseOctaves"]) NoiseOctaves = node["NoiseOctaves"].as<int>();
			if (node["NoiseAmplitude"]) NoiseAmplitude = node["NoiseAmplitude"].as<float>();
			if (node["SurfaceSubstance"]) SurfaceSubstance = node["SurfaceSubstance"].as<std::string>();
			if (node["FillSubstance"]) FillSubstance = node["FillSubstance"].as<std::string>();
			if (node["StoragePath"]) StoragePath = node["StoragePath"].as<std::string>();
		}
	};
}
