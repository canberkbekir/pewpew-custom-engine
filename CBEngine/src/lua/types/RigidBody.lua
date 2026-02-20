---@meta

--- Proxy object for a RigidBodyComponent. Obtained via entity:GetRigidBody().
--- Provides direct access to forces, impulses, velocity, and physics properties.
---@class RigidBody
local RigidBody = {}

-- =====================
-- Forces & Impulses
-- =====================

--- Apply a continuous force (in Newtons) for one physics step.
---@param force Vec3
function RigidBody:AddForce(force) end

--- Apply a rotational torque for one physics step.
---@param torque Vec3
function RigidBody:AddTorque(torque) end

--- Apply an instantaneous impulse (immediate velocity change).
---@param impulse Vec3
function RigidBody:AddImpulse(impulse) end

-- =====================
-- Velocity
-- =====================

--- Get the current linear velocity.
---@return Vec3
function RigidBody:GetLinearVelocity() end

--- Set the linear velocity directly.
---@param velocity Vec3
function RigidBody:SetLinearVelocity(velocity) end

--- Get the current angular velocity.
---@return Vec3
function RigidBody:GetAngularVelocity() end

--- Set the angular velocity directly.
---@param velocity Vec3
function RigidBody:SetAngularVelocity(velocity) end

-- =====================
-- Properties
-- =====================

--- Get the mass.
---@return number
function RigidBody:GetMass() end

--- Set the mass.
---@param mass number
function RigidBody:SetMass(mass) end

--- Get the friction coefficient.
---@return number
function RigidBody:GetFriction() end

--- Set the friction coefficient.
---@param friction number
function RigidBody:SetFriction(friction) end

--- Get the restitution (bounciness).
---@return number
function RigidBody:GetRestitution() end

--- Set the restitution (bounciness).
---@param restitution number
function RigidBody:SetRestitution(restitution) end

--- Get the linear damping.
---@return number
function RigidBody:GetLinearDamping() end

--- Set the linear damping.
---@param damping number
function RigidBody:SetLinearDamping(damping) end

--- Get the angular damping.
---@return number
function RigidBody:GetAngularDamping() end

--- Set the angular damping.
---@param damping number
function RigidBody:SetAngularDamping(damping) end

--- Check if gravity is enabled.
---@return boolean
function RigidBody:IsUsingGravity() end

--- Enable or disable gravity.
---@param useGravity boolean
function RigidBody:SetUseGravity(useGravity) end

--- Get the body type as a string: "static", "dynamic", "kinematic", or "none".
---@return string
function RigidBody:GetBodyType() end

--- Set the body type. Use "static", "dynamic", or "kinematic".
---@param bodyType string
function RigidBody:SetBodyType(bodyType) end

--- Check if the physics body is valid and ready for operations.
---@return boolean
function RigidBody:IsValid() end
