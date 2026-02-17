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

project "CBEngine"
    location "CBEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "cbpch.h"
	pchsource "CBEngine/src/cbpch.cpp"

	files
	{
		-- Engine source
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",

		-- stb_image
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",

		-- glm
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",

		-- voxelizer
		"%{prj.name}/vendor/voxelizer/**.h",
		"%{prj.name}/vendor/voxelizer/**.cpp",

		-- ImGuizmo
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp",

		-- Glad (embedded)
		"%{prj.name}/vendor/Glad/include/glad/glad.h",
		"%{prj.name}/vendor/Glad/include/KHR/khrplatform.h",
		"%{prj.name}/vendor/Glad/src/glad.c",

		-- ImGui (embedded)
		"%{prj.name}/vendor/imgui/imconfig.h",
		"%{prj.name}/vendor/imgui/imgui.h",
		"%{prj.name}/vendor/imgui/imgui.cpp",
		"%{prj.name}/vendor/imgui/imgui_draw.cpp",
		"%{prj.name}/vendor/imgui/imgui_internal.h",
		"%{prj.name}/vendor/imgui/imgui_tables.cpp",
		"%{prj.name}/vendor/imgui/imgui_widgets.cpp",
		"%{prj.name}/vendor/imgui/imstb_rectpack.h",
		"%{prj.name}/vendor/imgui/imstb_textedit.h",
		"%{prj.name}/vendor/imgui/imstb_truetype.h",
		"%{prj.name}/vendor/imgui/imgui_demo.cpp",

		-- yaml-cpp
		"%{prj.name}/vendor/yaml-cpp/src/**.cpp",
		"%{prj.name}/vendor/yaml-cpp/src/**.h",
		"%{prj.name}/vendor/yaml-cpp/include/**.h",

		-- GLFW (embedded - common files)
		"%{prj.name}/vendor/GLFW/include/GLFW/glfw3.h",
		"%{prj.name}/vendor/GLFW/include/GLFW/glfw3native.h",
		"%{prj.name}/vendor/GLFW/src/glfw_config.h",
		"%{prj.name}/vendor/GLFW/src/context.c",
		"%{prj.name}/vendor/GLFW/src/init.c",
		"%{prj.name}/vendor/GLFW/src/input.c",
		"%{prj.name}/vendor/GLFW/src/monitor.c",
		"%{prj.name}/vendor/GLFW/src/null_init.c",
		"%{prj.name}/vendor/GLFW/src/null_joystick.c",
		"%{prj.name}/vendor/GLFW/src/null_monitor.c",
		"%{prj.name}/vendor/GLFW/src/null_window.c",
		"%{prj.name}/vendor/GLFW/src/platform.c",
		"%{prj.name}/vendor/GLFW/src/vulkan.c",
		"%{prj.name}/vendor/GLFW/src/window.c",
	}

	-- Exclude vendor files from precompiled header
	filter "files:CBEngine/vendor/**"
		flags { "NoPCH" }
	filter "files:CBEngine/vendor/yaml-cpp/**"
		disablewarnings { "4267" }
	filter {}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"YAML_CPP_STATIC_DEFINE"
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
	}

	libdirs
	{
		"%{prj.name}/vendor/assimp/lib"
	}

	links
	{
		"opengl32.lib",
		"assimp-vc143-mt"
	}


    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8" }

		-- GLFW Windows-specific files
		files
		{
			"%{prj.name}/vendor/GLFW/src/win32_init.c",
			"%{prj.name}/vendor/GLFW/src/win32_joystick.c",
			"%{prj.name}/vendor/GLFW/src/win32_module.c",
			"%{prj.name}/vendor/GLFW/src/win32_monitor.c",
			"%{prj.name}/vendor/GLFW/src/win32_time.c",
			"%{prj.name}/vendor/GLFW/src/win32_thread.c",
			"%{prj.name}/vendor/GLFW/src/win32_window.c",
			"%{prj.name}/vendor/GLFW/src/wgl_context.c",
			"%{prj.name}/vendor/GLFW/src/egl_context.c",
			"%{prj.name}/vendor/GLFW/src/osmesa_context.c"
		}

	    defines
	    {
	    	"CB_PLATFORM_WINDOWS",
			"_GLFW_WIN32"
	    }

	filter "configurations:Debug"
		defines "CB_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "CB_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CB_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
    location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

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
		"%{IncludeDir.yaml_cpp}"
    }

    links { "CBEngine" }

    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8" }
	    defines
	    {
	    	"CB_PLATFORM_WINDOWS"
	    }

		-- Copy Assimp DLL to output folder after build
		postbuildcommands
		{
			"{COPYFILE} %{wks.location}CBEngine/vendor/assimp/bin/assimp-vc143-mt.dll %{cfg.targetdir}"
		}

	filter "configurations:Debug"
		defines "CB_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "CB_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CB_DIST"
		runtime "Release"
		optimize "on"

project "CBEngine-Editor"
    location "CBEngine-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

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
		"%{IncludeDir.yaml_cpp}"
    }

    links { "CBEngine" }

    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8" }
	    defines
	    {
	    	"CB_PLATFORM_WINDOWS"
	    }

		-- Copy Assimp DLL to output folder after build
		postbuildcommands
		{
			"{COPYFILE} %{wks.location}CBEngine/vendor/assimp/bin/assimp-vc143-mt.dll %{cfg.targetdir}"
		}

	filter "configurations:Debug"
		defines "CB_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "CB_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CB_DIST"
		runtime "Release"
		optimize "on"
