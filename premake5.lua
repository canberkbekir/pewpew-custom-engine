workspace "PewPew"
    architecture "x64"

    configurations {"Debug","Release","Dist"}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "PewPew/vendor/GLFW/include"

include "PewPew/vendor/GLFW"

project "PewPew"
    location "PewPew"
    kind "SharedLib"
    language "C++"

    targetdir ("bin/"..outputdir.."/%{prj.name}")
    objdir ("bin-int/"..outputdir.."/%{prj.name}")

    pchheader "pewpch.h"
    pchsource "PewPew/src/pewpch.cpp"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"    
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/vendor/sdplog/include",
        "%{IncludeDir.GLFW}"
    }

    links
    {
        "GLFW",
	    "opengl32.lib", 
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"
        buildoptions { "/utf-8" }

    defines 
    {
        "PEW_PLATFORM_WINDOWS",
        "PEW_BUILD_DLL", 
    } 

    postbuildcommands
    {
	    ("{MKDIR} ../bin/" .. outputdir .. "/Sandbox/"),
	    ("{COPYFILE} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox/")
    }

    filter "configurations:Debug"
        defines "PEW_DEBUG"
        buildoptions "/MDd"
        symbols "On"

    filter "configurations:Release"
        defines "PEW_RELEASE"
        buildoptions "/MD"
        optimize "On"

    filter "configurations:Dist"
        defines "PEW_DIST"
        buildoptions "/MD"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"

    language "C++"

    targetdir ("bin/"..outputdir.."/%{prj.name}")
    objdir ("bin-int/"..outputdir.."/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"    
    }

    includedirs
    {
        "PewPew/vendor/sdplog/include",
        "PewPew/src"
    }

    links { "PewPew"}

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"
        buildoptions { "/utf-8" }

    defines 
    {
        "PEW_PLATFORM_WINDOWS", 
    }  

    filter "configurations:Debug"
        defines "PEW_DEBUG"
        buildoptions "/MDd"
        symbols "On"

    filter "configurations:Release"
        defines "PEW_RELEASE"
        buildoptions "/MD"
        optimize "On"

    filter "configurations:Dist"
        defines "PEW_DIST"
        buildoptions "/MD"
        optimize "On"