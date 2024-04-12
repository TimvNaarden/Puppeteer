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
IncludeDir["GLFW"] = "%{wks.location}/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/vendor/ImGui"
IncludeDir["glm"] = "%{wks.location}/vendor/glm"
IncludeDir["Networking"] = "%{wks.location}/vendor/Networking/include"
IncludeDir["Database"] = "%{wks.location}/vendor/Database"
IncludeDir["ZLib"] = "%{wks.location}/vendor/zlib"
IncludeDir["NativeFileDialogExtended"] = "%{wks.location}/vendor/NativeFileDialogExtended/include"
IncludeDir["OpenXLSX"] = "%{wks.location}/vendor/OpenXLSX"

filter {}
-- Create a solution folder inside visual studio
group "Dependencies"
    include "vendor/GLFW"
    include "vendor/Glad"
    include "vendor/ImGui"
    include "vendor/Networking"
    include "vendor/Database"
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
        "%{IncludeDir.Networking}",
        "%{IncludeDir.Database}",
		"%{IncludeDir.ZLib}",
		"%{IncludeDir.NativeFileDialogExtended}",
		"%{IncludeDir.OpenXLSX}",
    }

    libdirs
    {
        "libs",
    }
    links
    {
        "GLFW",
        "Glad",
        "ImGui",
        "Networking",
        "Database",
		"nfd.lib",
		"OpenXLSX.lib"

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
