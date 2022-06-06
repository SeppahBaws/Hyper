project "Hyper"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    -- staticruntime "on"
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
        "%{IncludeDir.assimp}",
        "%{IncludeDir.stb}"
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
        "%{Library.Vulkan}",
        "stb"
    }

    if (_OPTIONS["use-vld"]) then
        defines { "HYPER_USE_VLD" }
        includedirs
        {
            (VLDLocation .. "/include")
        }
        libdirs
        {
            (VLDLocation .. "/lib/Win64")
        }
        links
        {
            "vld"
        }
    end

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

        links
        {
            -- Shaderc debug libraries
            "%{LibDir.Vulkan}/shaderc_sharedd.lib",
            "%{LibDir.Vulkan}/shaderc_utild.lib",
            
            -- Spirv Cross for reflection
            "%{LibDir.Vulkan}/spirv-cross-cored.lib",
            "%{LibDir.Vulkan}/spirv-cross-reflectd.lib",

            -- Assimp debug libraries
            "%{LibDir.assimp}/assimp-vc143-mtd.lib",
        }

        postbuildcommands
        {
            "{COPY} %{BinDir.assimp}/assimp-vc143-mtd.* %{cfg.targetdir}"
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

        defines
        {
            "HYPER_RELEASE"
        }

        links
        {
            -- Shaderc release libraries
            "%{LibDir.Vulkan}/shaderc_shared.lib",
            "%{LibDir.Vulkan}/shaderc_util.lib",
            
            -- Spirv Cross for reflection
            "%{LibDir.Vulkan}/spirv-cross-core.lib",
            "%{LibDir.Vulkan}/spirv-cross-reflect.lib",

            -- Assimp release libraries
            "%{LibDir.assimp}/assimp-vc143-mt.lib",
        }

        postbuildcommands
        {
            "{COPY} %{BinDir.assimp}/assimp-vc143-mt.* %{cfg.targetdir}"
        }
