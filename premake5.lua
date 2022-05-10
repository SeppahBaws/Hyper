workspace "Hyper"
    architecture "x86_64"
    startproject "Hyper"

    configurations
    {
        "Debug",
        "Release"
    }

    flags
    {
        "MultiProcessorCompile",
        -- "FatalWarnings"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

newoption {
    trigger = "use-vld",
    description = "Enable the use of VLD to check for memory leaks."
}

if (_OPTIONS["use-vld"]) then
    print("VLD was enabled, memory leaks will get detected.")
end

include "Dependencies.lua"

group "Dependencies"
    include "Hyper/dependencies/glfw"
    include "Hyper/dependencies/Optick"
group ""

include "Hyper"
