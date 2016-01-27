workspace "Hana"
    configurations { "Debug", "Release", "Production" }
    location "build"
    
    includedirs { "external" }
    
    defines { "_CRT_SECURE_NO_WARNINGS" }
    flags { "MultiProcessorCompile" }
    warnings "Extra"
    
    filter "configurations:Debug"
        defines { "DEBUG", "HANA_ASSERT_ON" }
        flags { "Symbols" }
        
    filter "configurations:Release"
        defines { "NDEBUG", "ASSERT_ON" }
        flags { "Symbols" }
        optimize "On"
    
    filter "configurations:Production"
        defines { "NDEBUG" }
        optimize "Full"
    
project "core"
    kind "StaticLib"
    architecture "x64"
    language "C++"
    location "build/core"
    targetdir "bin/%{cfg.buildcfg}"
    files { "src/core/**.h", "src/core/**.cpp" }

project "idx"
    kind "StaticLib"
    architecture "x64"
    language "C++"
    location "build/idx"
    targetdir "bin/%{cfg.buildcfg}"
    files { "src/idx/**.h", "src/idx/**.cpp" }
    
 project "test"
    kind "ConsoleApp"
    architecture "x64"
    language "C++"
    location "build/test"
    targetdir "bin/%{cfg.buildcfg}"    
    links { "core", "idx" }
    files { "src/test/**.h", "src/test/**.cpp" }