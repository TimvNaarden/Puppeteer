workspace "Master"
    architecture "x86_64"
    startproject "Master"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    flags
    {
        "MultiProcessorCompile"
    }

-- The output directory based on the configurations
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/libs/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/libs/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/libs/ImGui"
IncludeDir["glm"] = "%{wks.location}/libs/glm"

filter {}

-- Create a solution folder inside visual studio
group "Dependencies"
    include "libs/GLFW"
    include "libs/Glad"
    include "libs/ImGui"
group "" -- End the solution folder here

project "Master"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "src/pch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    defines
    {
        "GLFW_INCLUDE_NONE",
    }
 
    includedirs
    {
        "src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}",
    }

    links
    {
        "GLFW",
        "Glad",
        "ImGui",
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "PLATFORM_WINDOWS"
        }

    filter "system:linux"
        systemversion "latest"

        defines
        {
            "PLATFORM_LINUX"
        }

    filter "configurations:Debug"
        defines "DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "DIST"
        runtime "Release"
        optimize "on"