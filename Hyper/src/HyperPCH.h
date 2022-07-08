#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <memory>
#include <ranges>
#include <filesystem>

#include "Hyper/Defines.h"
#include "Hyper/Core/UUID.h"

#include "Hyper/Core/Logger.h"

// For using operator""s
using namespace std::string_literals;

// Temp workaround to speed up builds
#include <vulkan/vulkan.hpp>
