-- Premake5 configuration for Radar project

workspace "radar"
    architecture "x64"
    startproject "radar"
    
    configurations {
        "Release"
    }

    cppdialect "C++20"
    characterset "MBCS"
    symbols "Off"
    flags "MultiProcessorCompile"

    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "_SCL_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_DEPRECATE",
        "NOMINMAX",
        "NDEBUG",
        "RELEASE"
    }

    -- Windows specific settings
    filter "system:windows"
        toolset "v145"  -- Visual Studio 2026 toolset
        preferredtoolarchitecture "x86_64"
        staticruntime "On"
        systemversion "latest"
        defines {
            "WIN32",
            "_WIN32",
            "PLATFORM_WINDOWS"
        }
        buildoptions { "/Zc:__cplusplus" }

    -- Release configuration
    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"

project "radar"
    location "Build"
    kind "WindowedApp"
    language "C++"
    entrypoint "wWinMainCRTStartup"
    
    targetdir "Bin"
    objdir "Build/Intermediate"
    
    files {
        "src/**.h",
        "src/**.hpp",
        "src/**.cpp",
        "src/**.c"
    }
    
    includedirs {
        "src",
        "vendor"
    }

    links {
        "d3d11",
        "d3dcompiler",
        "dxgi",
        "d2d1",
        "dwrite",
        "dwmapi",
        "user32",
        "gdi32",
        "ole32",
        "winmm"
    }
    
    -- miniaudio requires these defines
    defines {
        "MA_NO_FLAC",
        "MA_NO_MP3",
        "MA_NO_ENCODING"
    }
