#pragma once

#include <cstdint>
#include <cstddef>

namespace CB
{
	enum class SubstancePropType : uint8_t { Float, Int, Bool, Enum, Color3, DamageTintMap, Custom };
	enum class SubstancePropWidget : uint8_t { DragFloat, SliderFloat, SliderInt, DragInt, Checkbox, Combo, ColorEdit3, Custom };

	// Forward-declared (VoxelSubstanceProperties defined in VoxelSubstance.h)
	struct VoxelSubstanceProperties;

	struct SubstancePropertyMeta
	{
		const char* YAMLKey;        // e.g. "health"
		const char* DisplayName;    // e.g. "Health"
		const char* Category;       // e.g. "Structural"
		const char* Tooltip;        // hover help text
		SubstancePropType Type;
		SubstancePropWidget Widget;
		float Min, Max, Speed;
		size_t Offset;              // offsetof into VoxelSubstanceProperties
		const char* const* EnumNames; // for Combo widgets
		int EnumCount;
		const char* EnabledBy;      // field name of a bool that must be true (nullptr = always visible)
	};

// Macro to define a scalar property metadata entry.
// Field must already exist in VoxelSubstanceProperties.
#define CB_SUBSTANCE_PROP(Field, YamlKey, Display, Cat, Tip, PropType, WidgetType, MinVal, MaxVal, SpeedVal) \
	{ YamlKey, Display, Cat, Tip, SubstancePropType::PropType, SubstancePropWidget::WidgetType, \
	  MinVal, MaxVal, SpeedVal, offsetof(VoxelSubstanceProperties, Field), nullptr, 0, nullptr }

#define CB_SUBSTANCE_PROP_ENUM(Field, YamlKey, Display, Cat, Tip, Names, Count) \
	{ YamlKey, Display, Cat, Tip, SubstancePropType::Enum, SubstancePropWidget::Combo, \
	  0, 0, 0, offsetof(VoxelSubstanceProperties, Field), Names, Count, nullptr }

#define CB_SUBSTANCE_PROP_COND(Field, YamlKey, Display, Cat, Tip, PropType, WidgetType, MinVal, MaxVal, SpeedVal, CondField) \
	{ YamlKey, Display, Cat, Tip, SubstancePropType::PropType, SubstancePropWidget::WidgetType, \
	  MinVal, MaxVal, SpeedVal, offsetof(VoxelSubstanceProperties, Field), nullptr, 0, CondField }

#define CB_SUBSTANCE_PROP_CUSTOM(YamlKey, Display, Cat, Tip, CondField) \
	{ YamlKey, Display, Cat, Tip, SubstancePropType::Custom, SubstancePropWidget::Custom, \
	  0, 0, 0, 0, nullptr, 0, CondField }

}
