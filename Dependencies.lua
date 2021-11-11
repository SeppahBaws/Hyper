VULKAN_SDK = os.getenv("VULKAN_SDK")
VLDLocation = os.getenv("VLD_DIR")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Hyper/dependencies/GLFW/include"
IncludeDir["spdlog"] = "%{wks.location}/Hyper/dependencies/spdlog/include"
IncludeDir["Vulkan"] = "%{VULKAN_SDK}/Include"

LibDir = {}
LibDir["Vulkan"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "%{LibDir.Vulkan}/vulkan-1.lib"
