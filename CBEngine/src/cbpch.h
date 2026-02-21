#pragma once

// --- Standard Library ---
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <random>
#include <queue>
#include <mutex>

// --- GLM (stable vendor, heavy to parse - precompile once) ---
#define GLM_ENABLE_EXPERIMENTAL  // required for glm/gtx/* headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

// --- EnTT (massive single-header ECS, pulled in by Scene.h/Entity.h in ~20 files) ---
#include <entt.hpp>

// --- Windows (lean to avoid dragging in rarely-used APIs) ---
#ifdef CB_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <Windows.h>
#endif

// --- Engine logging macros (stable, used engine-wide) ---
#include <CBEngine/Core/Log.h>
