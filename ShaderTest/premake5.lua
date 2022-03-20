project "ShaderTest"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
    {
        "%{IncludeDir.Vulkan}",
    }

    libdirs
    {
        "%{LibDir.Vulkan}"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

        links
        {
            "%{LibDir.Vulkan}/shaderc_sharedd.lib",
            "%{LibDir.Vulkan}/shaderc_utild.lib",
            "%{LibDir.Vulkan}/spirv-cross-cored.lib",
            "%{LibDir.Vulkan}/spirv-cross-glsld.lib",
            "%{LibDir.Vulkan}/SPIRV-Toolsd.lib",
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

        links
        {
            "%{LibDir.Vulkan}/shaderc_shared.lib",
            "%{LibDir.Vulkan}/shaderc_util.lib",
            "%{LibDir.Vulkan}/spirv-cross-core.lib",
            "%{LibDir.Vulkan}/spirv-cross-glsl.lib",
            "%{LibDir.Vulkan}/SPIRV-Tools.lib",
        }
