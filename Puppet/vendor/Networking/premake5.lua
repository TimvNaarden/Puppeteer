project "Networking"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")


    files
    {
        "include/**.h",
        "include/**.cpp",		
    }
	
	includedirs {
		"include"
	}

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "PLATFORM_WINDOWS",
        }
    
    filter "system:linux"
        systemversion "latest"

        defines
        {   
            "PLATFORM_LINUX"
        }
        links {
            "ssl",
            "crypto"
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
