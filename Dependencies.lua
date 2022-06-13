VULKAN_SDK = os.getenv("VULKAN_SDK")
VLDLocation = os.getenv("VLD_DIR")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Hyper/dependencies/glfw/include"
IncludeDir["glm"] = "%{wks.location}/Hyper/dependencies/glm"
IncludeDir["spdlog"] = "%{wks.location}/Hyper/dependencies/spdlog/include"
IncludeDir["Vulkan"] = "%{VULKAN_SDK}/Include"
IncludeDir["magic_enum"] = "%{wks.location}/Hyper/dependencies/magic_enum/include"
IncludeDir["VkMemAlloc"] = "%{wks.location}/Hyper/dependencies/VulkanMemoryAllocator/include"
IncludeDir["Optick"] = "%{wks.location}/Hyper/dependencies/Optick/src"
IncludeDir["assimp"] = "%{wks.location}/Hyper/dependencies/assimp/include"
IncludeDir["stb"] = "%{wks.location}/Hyper/dependencies/stb/include"
IncludeDir["NvidiaAftermath"] = "%{wks.location}/Hyper/dependencies/NvidiaAftermath/include"

LibDir = {}
LibDir["Vulkan"] = "%{VULKAN_SDK}/Lib"
LibDir["assimp"] = "%{wks.location}/Hyper/dependencies/assimp/lib"
LibDir["NvidiaAftermath"] = "%{wks.location}/Hyper/dependencies/NvidiaAftermath/lib"

Library = {}
Library["Vulkan"] = "%{LibDir.Vulkan}/vulkan-1.lib"
Library["NvidiaAftermath"] = "%{LibDir.NvidiaAftermath}/GFSDK_Aftermath_Lib.x64.lib"

BinDir = {}
BinDir["assimp"] = "%{wks.location}/Hyper/dependencies/assimp/bin"
