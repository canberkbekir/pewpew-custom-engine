workspace "CBEngine"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "CBEngine/vendor/GLFW/include"
IncludeDir["Glad"] = "CBEngine/vendor/Glad/include"
IncludeDir["ImGui"] = "CBEngine/vendor/imgui"
IncludeDir["glm"] = "CBEngine/vendor/glm"
IncludeDir["stb_image"] = "CBEngine/vendor/stb_image"
IncludeDir["assimp"] = "CBEngine/vendor/assimp/include"
IncludeDir["voxelizer"] = "CBEngine/vendor/voxelizer"
IncludeDir["ImGuizmo"] = "CBEngine/vendor/ImGuizmo"
IncludeDir["entt"] = "CBEngine/vendor/entt/include"
IncludeDir["yaml_cpp"] = "CBEngine/vendor/yaml-cpp/include"
IncludeDir["JoltPhysics"] = "CBEngine/vendor/JoltPhysics"
IncludeDir["lua"] = "CBEngine/vendor/lua/src"
IncludeDir["sol2"] = "CBEngine/vendor/sol2"
IncludeDir["OpenALSoft"] = "CBEngine/vendor/OpenAL-Soft/include"
IncludeDir["dr_libs"] = "CBEngine/vendor/dr_libs"

-- ═══════════════════════════════════════════════════════════════════════════
-- Dependencies group — these compile once and stay cached until vendor
-- source files themselves change. Keeps incremental engine builds fast.
-- ═══════════════════════════════════════════════════════════════════════════
group "Dependencies"

-- ── GLFW ─────────────────────────────────────────────────────────────────
project "GLFW"
	location "CBEngine/vendor/GLFW"
	kind "StaticLib"
	language "C"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"CBEngine/vendor/GLFW/include/GLFW/glfw3.h",
		"CBEngine/vendor/GLFW/include/GLFW/glfw3native.h",
		"CBEngine/vendor/GLFW/src/glfw_config.h",
		"CBEngine/vendor/GLFW/src/context.c",
		"CBEngine/vendor/GLFW/src/init.c",
		"CBEngine/vendor/GLFW/src/input.c",
		"CBEngine/vendor/GLFW/src/monitor.c",
		"CBEngine/vendor/GLFW/src/null_init.c",
		"CBEngine/vendor/GLFW/src/null_joystick.c",
		"CBEngine/vendor/GLFW/src/null_monitor.c",
		"CBEngine/vendor/GLFW/src/null_window.c",
		"CBEngine/vendor/GLFW/src/platform.c",
		"CBEngine/vendor/GLFW/src/vulkan.c",
		"CBEngine/vendor/GLFW/src/window.c",
	}

	defines { "_CRT_SECURE_NO_WARNINGS" }

	filter "system:windows"
		systemversion "latest"
		defines { "_GLFW_WIN32" }
		files
		{
			"CBEngine/vendor/GLFW/src/win32_init.c",
			"CBEngine/vendor/GLFW/src/win32_joystick.c",
			"CBEngine/vendor/GLFW/src/win32_module.c",
			"CBEngine/vendor/GLFW/src/win32_monitor.c",
			"CBEngine/vendor/GLFW/src/win32_time.c",
			"CBEngine/vendor/GLFW/src/win32_thread.c",
			"CBEngine/vendor/GLFW/src/win32_window.c",
			"CBEngine/vendor/GLFW/src/wgl_context.c",
			"CBEngine/vendor/GLFW/src/egl_context.c",
			"CBEngine/vendor/GLFW/src/osmesa_context.c",
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

-- ── Glad ─────────────────────────────────────────────────────────────────
project "Glad"
	location "CBEngine/vendor/Glad"
	kind "StaticLib"
	language "C"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"CBEngine/vendor/Glad/include/glad/glad.h",
		"CBEngine/vendor/Glad/include/KHR/khrplatform.h",
		"CBEngine/vendor/Glad/src/glad.c",
	}

	includedirs { "CBEngine/vendor/Glad/include" }

	filter "system:windows"
		systemversion "latest"
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

-- ── ImGui ────────────────────────────────────────────────────────────────
project "ImGui"
	location "CBEngine/vendor/imgui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"CBEngine/vendor/imgui/imconfig.h",
		"CBEngine/vendor/imgui/imgui.h",
		"CBEngine/vendor/imgui/imgui.cpp",
		"CBEngine/vendor/imgui/imgui_draw.cpp",
		"CBEngine/vendor/imgui/imgui_internal.h",
		"CBEngine/vendor/imgui/imgui_tables.cpp",
		"CBEngine/vendor/imgui/imgui_widgets.cpp",
		"CBEngine/vendor/imgui/imstb_rectpack.h",
		"CBEngine/vendor/imgui/imstb_textedit.h",
		"CBEngine/vendor/imgui/imstb_truetype.h",
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		files { "CBEngine/vendor/imgui/imgui_demo.cpp" }
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

-- ── yaml-cpp ─────────────────────────────────────────────────────────────
project "yaml-cpp"
	location "CBEngine/vendor/yaml-cpp"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"CBEngine/vendor/yaml-cpp/src/**.cpp",
		"CBEngine/vendor/yaml-cpp/src/**.h",
		"CBEngine/vendor/yaml-cpp/include/**.h",
	}

	includedirs { "CBEngine/vendor/yaml-cpp/include" }
	defines { "YAML_CPP_STATIC_DEFINE" }

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }
		disablewarnings { "4267" }
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

-- ── Lua ──────────────────────────────────────────────────────────────────
project "Lua"
	location "CBEngine/vendor/lua"
	kind "StaticLib"
	language "C"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"CBEngine/vendor/lua/src/*.c",
		"CBEngine/vendor/lua/src/*.h",
	}

	removefiles
	{
		"CBEngine/vendor/lua/src/lua.c",
		"CBEngine/vendor/lua/src/luac.c",
	}

	filter "system:windows"
		systemversion "latest"
		disablewarnings { "4244", "4702" }
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

-- ── JoltPhysics ──────────────────────────────────────────────────────────
project "JoltPhysics"
	location "CBEngine/vendor/JoltPhysics"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"CBEngine/vendor/JoltPhysics/Jolt/**.h",
		"CBEngine/vendor/JoltPhysics/Jolt/**.inl",
		"CBEngine/vendor/JoltPhysics/Jolt/**.cpp",
	}

	includedirs { "CBEngine/vendor/JoltPhysics" }

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"JPH_DISABLE_CUSTOM_ALLOCATOR",
		"JPH_CROSS_PLATFORM_DETERMINISTIC",
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8", "/bigobj" }
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
	filter "configurations:Release"
		runtime "Release"
		optimize "on"
	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

-- ═══════════════════════════════════════════════════════════════════════════
-- End Dependencies group
-- ═══════════════════════════════════════════════════════════════════════════
group ""

-- ═══════════════════════════════════════════════════════════════════════════
-- CBEngine (static library — engine core)
-- ═══════════════════════════════════════════════════════════════════════════
project "CBEngine"
    location "CBEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "cbpch.h"
	pchsource "CBEngine/src/cbpch.cpp"

	files
	{
		-- Engine source
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",

		-- Tiny vendors (header-only or 1-2 files) stay embedded
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/voxelizer/**.h",
		"%{prj.name}/vendor/voxelizer/**.cpp",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp",
	}

	-- Exclude vendor files from precompiled header
	filter "files:CBEngine/vendor/**"
		flags { "NoPCH" }
	filter {}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"YAML_CPP_STATIC_DEFINE",
		"JPH_DISABLE_CUSTOM_ALLOCATOR",
		"JPH_CROSS_PLATFORM_DETERMINISTIC",
		"CB_AUDIO_ENABLED"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.voxelizer}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.JoltPhysics}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.OpenALSoft}",
		"%{IncludeDir.dr_libs}",
	}

	libdirs
	{
		"%{prj.name}/vendor/assimp/lib",
		"%{prj.name}/vendor/OpenAL-Soft/libs/Win64"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"yaml-cpp",
		"Lua",
		"JoltPhysics",
		"opengl32.lib",
		"assimp-vc143-mt",
		"OpenAL32"
	}

    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8", "/bigobj" }

	    defines
	    {
	    	"CB_PLATFORM_WINDOWS",
			"_GLFW_WIN32"
	    }

	filter "configurations:Debug"
		defines { "CB_DEBUG", "CB_ENABLE_IMGUI" }
		runtime "Debug"
		symbols "FastLink"

	filter "configurations:Release"
		defines { "CB_RELEASE", "CB_ENABLE_IMGUI" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CB_DIST"
		runtime "Release"
		optimize "on"

-- ═══════════════════════════════════════════════════════════════════════════
-- Sandbox (game executable)
-- ═══════════════════════════════════════════════════════════════════════════
project "Sandbox"
    location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

    targetdir ("bin/"..outputdir.."/%{prj.name}")
    objdir ("bin-int/"..outputdir.."/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/resources/icon.rc",
        -- Asset files (visible in solution but not compiled)
        "%{prj.name}/assets/shaders/**.glsl",
        "%{prj.name}/assets/shaders/**.hlsl",
        "%{prj.name}/assets/shaders/**.vert",
        "%{prj.name}/assets/shaders/**.frag",
        "%{prj.name}/assets/textures/**.png",
        "%{prj.name}/assets/textures/**.jpg",
        "%{prj.name}/assets/textures/**.tga",
        "%{prj.name}/assets/models/**.fbx",
        "%{prj.name}/assets/models/**.obj",
        "%{prj.name}/assets/models/**.gltf"
    }

    -- Organize assets in virtual folders in Solution Explorer
    vpaths
    {
        ["Source/*"] = { "%{prj.name}/src/**.h", "%{prj.name}/src/**.cpp" },
        ["Assets/Shaders/*"] = { "%{prj.name}/assets/shaders/**" },
        ["Assets/Textures/*"] = { "%{prj.name}/assets/textures/**" },
        ["Assets/Models/*"] = { "%{prj.name}/assets/models/**" }
    }

    includedirs
    {
        "CBEngine/vendor/spdlog/include",
        "CBEngine/src",
		"CBEngine/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.voxelizer}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.JoltPhysics}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.OpenALSoft}"
    }

    links { "CBEngine" }

    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8" }
	    defines
	    {
	    	"CB_PLATFORM_WINDOWS"
	    }

		-- Copy DLLs and Lua library to output folder after build
		postbuildcommands
		{
			"{COPYFILE} %{wks.location}CBEngine/vendor/assimp/bin/assimp-vc143-mt.dll %{cfg.targetdir}",
			"{COPYFILE} %{wks.location}CBEngine/vendor/OpenAL-Soft/bin/Win64/OpenAL32.dll %{cfg.targetdir}",
			"{MKDIR} %{cfg.targetdir}/lua",
			"{COPYDIR} %{wks.location}CBEngine/src/lua %{cfg.targetdir}/lua"
		}

	filter "configurations:Debug"
		defines { "CB_DEBUG", "CB_ENABLE_IMGUI" }
		runtime "Debug"
		symbols "FastLink"

	filter "configurations:Release"
		defines { "CB_RELEASE", "CB_ENABLE_IMGUI" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CB_DIST"
		runtime "Release"
		optimize "on"

-- ═══════════════════════════════════════════════════════════════════════════
-- CBEngine-Editor (editor executable)
-- ═══════════════════════════════════════════════════════════════════════════
project "CBEngine-Editor"
    location "CBEngine-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	flags { "MultiProcessorCompile" }

    targetdir ("bin/"..outputdir.."/%{prj.name}")
    objdir ("bin-int/"..outputdir.."/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/resources/icon.rc",
        -- Asset files (visible in solution but not compiled)
        "%{prj.name}/assets/shaders/**.glsl",
        "%{prj.name}/assets/shaders/**.hlsl",
        "%{prj.name}/assets/shaders/**.vert",
        "%{prj.name}/assets/shaders/**.frag",
        "%{prj.name}/assets/textures/**.png",
        "%{prj.name}/assets/textures/**.jpg",
        "%{prj.name}/assets/textures/**.tga",
        "%{prj.name}/assets/models/**.fbx",
        "%{prj.name}/assets/models/**.obj",
        "%{prj.name}/assets/models/**.gltf"
    }

    -- Organize assets in virtual folders in Solution Explorer
    vpaths
    {
        ["Source/*"] = { "%{prj.name}/src/**.h", "%{prj.name}/src/**.cpp" },
        ["Assets/Shaders/*"] = { "%{prj.name}/assets/shaders/**" },
        ["Assets/Textures/*"] = { "%{prj.name}/assets/textures/**" },
        ["Assets/Models/*"] = { "%{prj.name}/assets/models/**" }
    }

    includedirs
    {
        "CBEngine/vendor/spdlog/include",
        "CBEngine/src",
		"CBEngine/vendor",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.voxelizer}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.JoltPhysics}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.OpenALSoft}"
    }

    links { "CBEngine" }

    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8" }
	    defines
	    {
	    	"CB_PLATFORM_WINDOWS"
	    }

		-- Copy DLLs and Lua library to output folder after build
		postbuildcommands
		{
			"{COPYFILE} %{wks.location}CBEngine/vendor/assimp/bin/assimp-vc143-mt.dll %{cfg.targetdir}",
			"{COPYFILE} %{wks.location}CBEngine/vendor/OpenAL-Soft/bin/Win64/OpenAL32.dll %{cfg.targetdir}",
			"{MKDIR} %{cfg.targetdir}/lua",
			"{COPYDIR} %{wks.location}CBEngine/src/lua %{cfg.targetdir}/lua"
		}

	filter "configurations:Debug"
		defines { "CB_DEBUG", "CB_ENABLE_IMGUI" }
		runtime "Debug"
		symbols "FastLink"

	filter "configurations:Release"
		defines { "CB_RELEASE", "CB_ENABLE_IMGUI" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CB_DIST"
		runtime "Release"
		optimize "on"
