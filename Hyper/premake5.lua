project "Hyper"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"
    warnings "extra"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "HyperPCH.h"
    pchsource "src/HyperPCH.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
    {
        "src",

        "%{IncludeDir.GLFW}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.Vulkan}",
        "%{IncludeDir.magic_enum}",
        "%{IncludeDir.VkMemAlloc}",
        "%{IncludeDir.Optick}",
        "%{IncludeDir.assimp}"
    }

    libdirs
    {
        "%{LibDir.Vulkan}",
        "%{LibDir.assimp}",
    }

    links
    {
        "GLFW",
        "Optick",
        "%{Library.Vulkan}"
    }

    filter "system:windows"
        systemversion "latest"
        system "Windows"

        defines
        {
            "HYPER_WINDOWS"
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

        defines
        {
            "HYPER_DEBUG"
        }

        -- Shaderc debug libraries
        links
        {
            "%{LibDir.Vulkan}/shaderc_sharedd.lib",
            "%{LibDir.Vulkan}/shaderc_utild.lib",
            "%{LibDir.Vulkan}/spirv-cross-cored.lib",
            "%{LibDir.Vulkan}/spirv-cross-glsld.lib",
            "%{LibDir.Vulkan}/SPIRV-Toolsd.lib",

            -- Assimp debug
            "%{LibDir.assimp}/Debug/assimp-vc143-mtd.lib",
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

        defines
        {
            "HYPER_RELEASE"
        }

        -- Shaderc release libraries
        links
        {
            "%{LibDir.Vulkan}/shaderc_shared.lib",
            "%{LibDir.Vulkan}/shaderc_util.lib",
            "%{LibDir.Vulkan}/spirv-cross-core.lib",
            "%{LibDir.Vulkan}/spirv-cross-glsl.lib",
            "%{LibDir.Vulkan}/SPIRV-Tools.lib",

            -- Assimp release
            "%{LibDir.assimp}/Release/assimp-vc143-mt.lib",
        }

    filter "configurations:Distribute"
        runtime "Release"
        optimize "on"

        defines
        {
            "HYPER_DISTRIBUTE"
        }

        links
        {
            -- Shaderc release libraries
            "%{LibDir.Vulkan}/shaderc_shared.lib",
            "%{LibDir.Vulkan}/shaderc_util.lib",
            "%{LibDir.Vulkan}/spirv-cross-core.lib",
            "%{LibDir.Vulkan}/spirv-cross-glsl.lib",
            "%{LibDir.Vulkan}/SPIRV-Tools.lib",

            -- Assimp release
            "%{LibDir.assimp}/Release/assimp-vc143-mt.lib",
        }
