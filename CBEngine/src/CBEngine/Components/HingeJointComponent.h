#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/YAMLHelpers.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>

namespace YAML { class Emitter; class Node; }

namespace CB
{
    struct HingeJointComponent
    {
        static constexpr auto YAMLKey = "HingeJointComponent";

        // Connected body (UUID(0) = attach to world)
        UUID ConnectedBodyUUID = UUID(0);

        // Anchor & Axis in local space of this entity
        Vector3 Anchor          = Vector3(0.0f);
        Vector3 ConnectedAnchor = Vector3(0.0f); // auto-computed when AutoConfigureConnectedAnchor=true
        Vector3 Axis            = Vector3(0.0f, 1.0f, 0.0f); // rotation axis
        bool    AutoConfigureConnectedAnchor = true;

        // Limits
        bool  UseLimits       = false;
        float LimitsMin       = -180.0f; // degrees
        float LimitsMax       =  180.0f; // degrees
        float Bounciness      =  0.0f;   // [0..1]
        float ContactDistance =  5.0f;   // degrees buffer before hitting limit

        // Spring (soft limits)
        bool  UseSpring       = false;
        float SpringFrequency = 2.0f;  // Hz
        float SpringDamping   = 1.0f;
        float TargetPosition  = 0.0f;  // degrees

        // Motor
        bool  UseMotor        = false;
        float TargetVelocity  = 0.0f;    // degrees/second
        float MotorForce      = 1000.0f; // max torque (Nm)
        bool  FreeSpin        = false;   // accelerate only, never brake

        // Break
        float BreakForce  = 0.0f; // 0 = unbreakable
        float BreakTorque = 0.0f; // 0 = unbreakable

        // Options
        bool  EnableCollision   = false;
        float MaxFrictionTorque = 0.0f;

        // Runtime state (NOT serialized)
        JPH::Constraint* RuntimeConstraint = nullptr;
        bool             ConstraintCreated = false;
        bool             IsBroken          = false;

        HingeJointComponent() = default;
        HingeJointComponent(const HingeJointComponent&) = default;

        void Serialize(YAML::Emitter& out) const
        {
            out << YAML::Key << "ConnectedBodyUUID" << YAML::Value << static_cast<uint64_t>(ConnectedBodyUUID);
            out << YAML::Key << "Anchor"            << YAML::Value << Anchor;
            out << YAML::Key << "ConnectedAnchor"   << YAML::Value << ConnectedAnchor;
            out << YAML::Key << "Axis"              << YAML::Value << Axis;
            out << YAML::Key << "AutoConfigureConnectedAnchor" << YAML::Value << AutoConfigureConnectedAnchor;

            out << YAML::Key << "UseLimits"       << YAML::Value << UseLimits;
            out << YAML::Key << "LimitsMin"       << YAML::Value << LimitsMin;
            out << YAML::Key << "LimitsMax"       << YAML::Value << LimitsMax;
            out << YAML::Key << "Bounciness"      << YAML::Value << Bounciness;
            out << YAML::Key << "ContactDistance" << YAML::Value << ContactDistance;

            out << YAML::Key << "UseSpring"       << YAML::Value << UseSpring;
            out << YAML::Key << "SpringFrequency" << YAML::Value << SpringFrequency;
            out << YAML::Key << "SpringDamping"   << YAML::Value << SpringDamping;
            out << YAML::Key << "TargetPosition"  << YAML::Value << TargetPosition;

            out << YAML::Key << "UseMotor"       << YAML::Value << UseMotor;
            out << YAML::Key << "TargetVelocity" << YAML::Value << TargetVelocity;
            out << YAML::Key << "MotorForce"     << YAML::Value << MotorForce;
            out << YAML::Key << "FreeSpin"       << YAML::Value << FreeSpin;

            out << YAML::Key << "BreakForce"  << YAML::Value << BreakForce;
            out << YAML::Key << "BreakTorque" << YAML::Value << BreakTorque;

            out << YAML::Key << "EnableCollision"   << YAML::Value << EnableCollision;
            out << YAML::Key << "MaxFrictionTorque" << YAML::Value << MaxFrictionTorque;
        }

        void Deserialize(const YAML::Node& node)
        {
            if (node["ConnectedBodyUUID"])
                ConnectedBodyUUID = UUID(node["ConnectedBodyUUID"].as<uint64_t>());
            if (node["Anchor"])          Anchor          = node["Anchor"].as<Vector3>();
            if (node["ConnectedAnchor"]) ConnectedAnchor = node["ConnectedAnchor"].as<Vector3>();
            if (node["Axis"])            Axis            = node["Axis"].as<Vector3>();
            if (node["AutoConfigureConnectedAnchor"])
                AutoConfigureConnectedAnchor = node["AutoConfigureConnectedAnchor"].as<bool>();

            if (node["UseLimits"])       UseLimits       = node["UseLimits"].as<bool>();
            if (node["LimitsMin"])       LimitsMin       = node["LimitsMin"].as<float>();
            if (node["LimitsMax"])       LimitsMax       = node["LimitsMax"].as<float>();
            if (node["Bounciness"])      Bounciness      = node["Bounciness"].as<float>();
            if (node["ContactDistance"]) ContactDistance = node["ContactDistance"].as<float>();

            if (node["UseSpring"])       UseSpring       = node["UseSpring"].as<bool>();
            if (node["SpringFrequency"]) SpringFrequency = node["SpringFrequency"].as<float>();
            if (node["SpringDamping"])   SpringDamping   = node["SpringDamping"].as<float>();
            if (node["TargetPosition"])  TargetPosition  = node["TargetPosition"].as<float>();

            if (node["UseMotor"])       UseMotor       = node["UseMotor"].as<bool>();
            if (node["TargetVelocity"]) TargetVelocity = node["TargetVelocity"].as<float>();
            if (node["MotorForce"])     MotorForce     = node["MotorForce"].as<float>();
            if (node["FreeSpin"])       FreeSpin       = node["FreeSpin"].as<bool>();

            if (node["BreakForce"])  BreakForce  = node["BreakForce"].as<float>();
            if (node["BreakTorque"]) BreakTorque = node["BreakTorque"].as<float>();

            if (node["EnableCollision"])   EnableCollision   = node["EnableCollision"].as<bool>();
            if (node["MaxFrictionTorque"]) MaxFrictionTorque = node["MaxFrictionTorque"].as<float>();
        }
    };
}
