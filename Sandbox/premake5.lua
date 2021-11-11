project "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"
    warnings "extra"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "%{wks.location}/Hyper/src"
    }

    links
    {
        "Hyper"
    }

    if (_OPTIONS["use-vld"]) then
        defines { "SANDBOX_USE_VLD" }
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

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
