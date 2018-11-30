

project "NewtonSDK"
    location "../../builds"
    kind "StaticLib"
    language "C++"
	characterset "MBCS"
	defines {"_NEWTON_STATIC_LIB", "_CUSTOM_JOINTS_STATIC_LIB", "_DVEHICLE_STATIC_LIB"}
	
	flags { "MultiProcessorCompile" }
	
	if _ACTION == "vs2017" then
		location "../../builds/VisualStudio2017"
	end
    
	targetdir ("../../builds/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../../builds/bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{ 
		"dgCore/**.h", 
		"dgCore/**.cpp", 
		"dMath/**.h", 
		"dMath/**.cpp", 
		"dContainers/**.h", 
		"dContainers/**.cpp", 
		"dgPhysics/**.h", 
		"dgPhysics/**.cpp", 
		"dgNewton/**.h", 
		"dgNewton/**.cpp", 
		"dCustomJoints/**.h", 
		"dCustomJoints/**.cpp", 
		"dAnimation/**.h", 
		"dAnimation/**.cpp", 
		"dgMeshUtil/**.h", 
		"dgMeshUtil/**.cpp", 
		"dVehicle/**.h", 
		"dVehicle/**.cpp", 
    }

    includedirs
	{
		"../",
		"dMath",
		"dContainers",
		"dgCore",
		"dgPhysics",
		"dgMeshUtil",
		"dgNewton",
		"dCustomJoints",
		"dAnimation"
    }
    
	filter "system:windows"
		cppdialect "C++14"
        staticruntime "On"
        systemversion "latest"
       
		defines 
		{ 
            "_WIN_64_VER",
			"_LIB",
			"_CRT_SECURE_NO_WARNINGS"
		}

    filter "configurations:Debug"
        symbols "On"
                
    filter "configurations:Release"
        optimize "On"

	filter { "system:windows", "configurations:Debug" }
        buildoptions "/MTd"
		
    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"




