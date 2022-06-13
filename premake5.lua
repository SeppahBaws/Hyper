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
newoption {
    trigger = "use-aftermath",
    description = "Enable Nsight Aftermath to generate GPU dumps when a TDR happens."
}

if (_OPTIONS["use-vld"]) then
    print("VLD was enabled, memory leaks will get detected.")
end

if (_OPTIONS["use-aftermath"]) then
    print("Nsight Aftermath was enabled, GPU crash dumps will get generated.")
end

include "Dependencies.lua"

group "Dependencies"
    include "Hyper/dependencies/glfw"
    include "Hyper/dependencies/Optick"
    include "Hyper/dependencies/stb"
group ""

include "Hyper"
