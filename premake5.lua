workspace "rengage"    
	configurations { "Debug", "Release" }

project "rengage"
	kind "ConsoleApp"
	language "c++"
	targetdir "build/%{cfg.buildcfg}"
	debugdir "."
	flags { "C++14" }

	includedirs { 
		"src", 
		"externals/unicorn/include",
	}

	files { 
		"src/**.h", 
		"src/**.cpp",
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"

	filter "action:vs*"
		defines "_CRT_SECURE_NO_WARNINGS"

	filter "action:gmake"
		buildoptions { 
			"-Wall",
			"-Wextra",
		}