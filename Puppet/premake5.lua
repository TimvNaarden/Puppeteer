workspace "Puppet"
    architecture "x86_64"
    startproject "Puppet"

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
IncludeDir["Networking"] = "%{wks.location}/vendor/Networking/include"
IncludeDir["Database"] = "%{wks.location}/vendor/Database"

filter {}
-- Create a solution folder inside visual studio
group "Dependencies"
    include "vendor/Networking"
    include "vendor/Database"
group "" -- End the solution folder here


project "Puppet"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")


    files
    {
        "src/**.h",
        "src/**.cpp",
    }

 
    includedirs
    {
        "src",
        "%{IncludeDir.Networking}",
        "%{IncludeDir.Database}",
    }

    libdirs
    {
        "libs",
    }
    links
    {
        "Networking",
        "Database",
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
