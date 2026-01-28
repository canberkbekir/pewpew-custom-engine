workspace "PewPew"
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
IncludeDir["GLFW"] = "PewPew/vendor/GLFW/include"
IncludeDir["Glad"] = "PewPew/vendor/Glad/include"
IncludeDir["ImGui"] = "PewPew/vendor/imgui"
IncludeDir["glm"] = "PewPew/vendor/glm"
IncludeDir["stb_image"] = "PewPew/vendor/stb_image"
IncludeDir["assimp"] = "PewPew/vendor/assimp/include"

include "PewPew/vendor/GLFW"
include "PewPew/vendor/Glad"
include "PewPew/vendor/imgui"

project "PewPew"
    location "PewPew"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pewpch.h"
	pchsource "PewPew/src/pewpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
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
	}

	libdirs
	{
		"%{prj.name}/vendor/assimp/lib"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib",
		"assimp-vc143-mt"
	}


    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8" }
	    defines
	    {
	    	"PEW_PLATFORM_WINDOWS",
	    	"PEW_BUILD_DLL",
	    	"GLFW_INCLUDE_NONE"
	    }

	filter "configurations:Debug"
		defines "PEW_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "PEW_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "PEW_DIST"
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
        "PewPew/vendor/spdlog/include",
        "PewPew/src",
		"PewPew/vendor",
		"%{IncludeDir.glm}"
    }

    links { "PewPew" }

    filter "system:windows"
	    systemversion "latest"
        buildoptions { "/utf-8" }
	    defines
	    {
	    	"PEW_PLATFORM_WINDOWS"
	    }

		-- Copy Assimp DLL to output folder after build
		postbuildcommands
		{
			"{COPYFILE} %{wks.location}PewPew/vendor/assimp/bin/assimp-vc143-mt.dll %{cfg.targetdir}"
		}

	filter "configurations:Debug"
		defines "PEW_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "PEW_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "PEW_DIST"
		runtime "Release"
		optimize "on"
