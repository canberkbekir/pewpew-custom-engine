#include "cbpch.h"
#include "SubstancePropertySchema.h"
#include "VoxelSubstance.h"

namespace CB
{
	static const char* s_FractureNames[] = {"None", "Chip", "Crack", "Shatter", "Crumble"};

	const std::vector<SubstancePropertyMeta>& GetSubstancePropertySchema()
	{
		static std::vector<SubstancePropertyMeta> schema = {
			// --- Physical ---
			CB_SUBSTANCE_PROP(MassPerVoxel, "mass_per_voxel", "Mass Per Voxel", "Physical",
				"kg per voxel. Fragment mass = VoxelCount * MassPerVoxel",
				Float, DragFloat, 0.001f, 10.0f, 0.001f),

			// --- Structural ---
			CB_SUBSTANCE_PROP(Health, "health", "Health", "Structural",
				"Base HP before destruction",
				Float, DragFloat, 1.0f, 10000.0f, 1.0f),
			CB_SUBSTANCE_PROP(Hardness, "hardness", "Hardness", "Structural",
				"0-100; reduces Impact damage",
				Float, SliderFloat, 0.0f, 100.0f, 0.0f),
			CB_SUBSTANCE_PROP(ExplosionResistance, "explosion_resistance", "Explosion Resist", "Structural",
				"0-1 (1 = immune to explosions)",
				Float, SliderFloat, 0.0f, 1.0f, 0.0f),
			CB_SUBSTANCE_PROP(SliceResistance, "slice_resistance", "Slice Resist", "Structural",
				"0-1 (0 = cut like butter)",
				Float, SliderFloat, 0.0f, 1.0f, 0.0f),
			CB_SUBSTANCE_PROP(TensileStrength, "tensile_strength", "Tensile Strength", "Structural",
				"Load capacity for structural integrity checks",
				Float, DragFloat, 0.0f, 100000.0f, 1.0f),
			CB_SUBSTANCE_PROP(ImpactThreshold, "impact_threshold", "Impact Threshold", "Structural",
				"Min force (N) before any impact damage registers",
				Float, DragFloat, 0.0f, 1000.0f, 0.1f),

			// --- Fracture ---
			CB_SUBSTANCE_PROP_ENUM(Fracture, "fracture_behavior", "Behavior", "Fracture",
				"Visual cracking pattern: None, Chip (stone), Crack (wood), Shatter (glass), Crumble (dirt)",
				s_FractureNames, 5),
			CB_SUBSTANCE_PROP(FractureThreshold, "fracture_threshold", "Fracture Threshold", "Fracture",
				"Health fraction (0-1) at which cracking visuals start",
				Float, SliderFloat, 0.0f, 1.0f, 0.0f),
			CB_SUBSTANCE_PROP(FragmentCount, "fragment_count", "Fragment Count", "Fracture",
				"Number of fragment pieces spawned on destruction",
				Int, SliderInt, 0.0f, 20.0f, 0.0f),
			CB_SUBSTANCE_PROP(FragmentsHavePhysics, "fragments_have_physics", "Fragments Have Physics", "Fracture",
				"Whether spawned fragments get RigidBody + Collider",
				Bool, Checkbox, 0.0f, 0.0f, 0.0f),

			// --- Environmental ---
			CB_SUBSTANCE_PROP(Flammable, "flammable", "Flammable", "Environmental",
				"Whether this substance can catch fire",
				Bool, Checkbox, 0.0f, 0.0f, 0.0f),
			CB_SUBSTANCE_PROP_COND(IgnitionTemperature, "ignition_temperature", "Ignition Temperature", "Environmental",
				"Heat required to ignite",
				Float, DragFloat, 0.0f, 1000.0f, 1.0f, "Flammable"),
			CB_SUBSTANCE_PROP_COND(BurnDuration, "burn_duration", "Burn Duration (s)", "Environmental",
				"Seconds until fire burns out (0 = never)",
				Float, DragFloat, 0.0f, 120.0f, 0.1f, "Flammable"),
			CB_SUBSTANCE_PROP(PropagatesDamage, "propagates_damage", "Propagates Damage", "Environmental",
				"Fire/explosion spreads to neighboring voxels",
				Bool, Checkbox, 0.0f, 0.0f, 0.0f),
			CB_SUBSTANCE_PROP_COND(DamageSpreadFactor, "damage_spread_factor", "Spread Factor", "Environmental",
				"Probability multiplier when spreading to neighbors (0-1)",
				Float, SliderFloat, 0.0f, 1.0f, 0.0f, "PropagatesDamage"),

			// --- Soot ---
			CB_SUBSTANCE_PROP(SootResistance, "soot_resistance", "Soot Resistance", "Environmental",
				"0 = absorbs soot easily, 1 = stays clean",
				Float, SliderFloat, 0.0f, 1.0f, 0.0f),
			CB_SUBSTANCE_PROP(SootColor, "soot_color", "Soot Color", "Environmental",
				"Dark tint color applied by neighboring fires",
				Color3, ColorEdit3, 0.0f, 0.0f, 0.0f),
			CB_SUBSTANCE_PROP_COND(CrossEntitySpreadRadius, "cross_entity_spread_radius", "Cross-Entity Spread Radius", "Environmental",
				"World units — how far fire can jump to another mesh",
				Float, DragFloat, 0.0f, 5.0f, 0.05f, "Flammable"),

			// --- Thermal (Heat Octree) ---
			CB_SUBSTANCE_PROP(HeatConductivity, "heat_conductivity", "Heat Conductivity", "Thermal",
				"0-1 — how well this substance conducts heat (metal=0.9, wood=0.4, stone=0.1)",
				Float, SliderFloat, 0.0f, 1.0f, 0.0f),
			CB_SUBSTANCE_PROP_COND(BurnHeatOutput, "burn_heat_output", "Burn Heat Output", "Thermal",
				"Heat per second emitted while burning",
				Float, DragFloat, 0.0f, 1000.0f, 1.0f, "Flammable"),
			CB_SUBSTANCE_PROP_COND(HeatDecayRate, "heat_decay_rate", "Heat Decay Rate", "Thermal",
				"Heat per second lost in empty space around this substance",
				Float, DragFloat, 0.0f, 100.0f, 0.1f, "Flammable"),
			CB_SUBSTANCE_PROP_COND(HeatAcceleration, "heat_acceleration", "Heat Acceleration", "Thermal",
				"Ramp-up multiplier (0.2=smolder, 2.0=flash fire)",
				Float, DragFloat, 0.0f, 5.0f, 0.05f, "Flammable"),
			CB_SUBSTANCE_PROP(InternalConductionRate, "internal_conduction_rate", "Internal Conduction Rate", "Thermal",
				"Per-voxel heat transfer rate within an entity (0=disabled, 0.5=wood, 0.9=metal)",
				Float, SliderFloat, 0.0f, 1.0f, 0.0f),

			// --- Burn Stages (Custom widget, enabled by Flammable) ---
			CB_SUBSTANCE_PROP_CUSTOM("burn_stages", "Burn Stage Tints", "Environmental",
				"Visual color progression when on fire: Normal > Burning > Charred > Collapse", "Flammable"),

			// --- Damage Tints (Custom widget, always visible) ---
			CB_SUBSTANCE_PROP_CUSTOM("damage_tints", "Damage Tints", "Damage Tints",
				"Per-damage-type visual tint configuration", nullptr),
		};

		return schema;
	}
}
