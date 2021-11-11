VULKAN_SDK = os.getenv("VULKAN_SDK")
VLDLocation = os.getenv("VLD_DIR")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Hyper/dependencies/GLFW/include"
IncludeDir["spdlog"] = "%{wks.location}/Hyper/dependencies/spdlog/include"

LibDir = {}

Library = {}
