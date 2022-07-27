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
        "%{IncludeDir.stb}",
        "%{IncludeDir.imgui}"
    }

    libdirs
    {
        "%{LibDir.Vulkan}",
        "%{LibDir.assimp}"
    }

    links
    {
        "GLFW",
        "Optick",
        "%{Library.Vulkan}",
        "stb",
        "imgui"
    }
    
    defines
    {
        "VULKAN_HPP_NO_EXCEPTIONS"
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

    if (_OPTIONS["use-aftermath"]) then
        defines { "HYPER_USE_AFTERMATH" }
        includedirs
        {
            "%{IncludeDir.NvidiaAftermath}"
        }
        libdirs
        {
            "%{LibDir.NvidiaAftermath}"
        }
        links
        {
            "%{Library.NvidiaAftermath}"
        }
        postbuildcommands
        {
            "{COPY} %{LibDir.NvidiaAftermath}/GFSDK_Aftermath_Lib.x64.dll %{cfg.targetdir}",
            "{COPY} %{LibDir.NvidiaAftermath}/llvm_7_0_1.dll %{cfg.targetdir}",
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
            "%{Library.shaderc_shared_debug}",
            "%{Library.shaderc_utils_debug}",
            
            -- Spirv Cross for reflection
            "%{Library.spirv_cross_core_debug}",
            "%{Library.spirv_cross_reflect_debug}",

            -- Assimp debug libraries
            "%{Library.assimp_debug}",
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
            "%{Library.shaderc_shared_release}",
            "%{Library.shaderc_utils_release}",
            
            -- Spirv Cross for reflection
            "%{Library.spirv_cross_core_release}",
            "%{Library.spirv_cross_reflect_release}",

            -- Assimp release libraries
            "%{Library.assimp_release}",
        }

        postbuildcommands
        {
            "{COPY} %{BinDir.assimp}/assimp-vc143-mt.* %{cfg.targetdir}"
        }
