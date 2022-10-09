#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <memory>
#include <ranges>
#include <filesystem>

#ifdef HYPER_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Get rid of Windows' stupid min and max defines (seriously wtf windows)
#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif
#endif

#include "Hyper/Defines.h"
#include "Hyper/Core/UUID.h"

#include "Hyper/Core/Logger.h"

// For using operator""s
using namespace std::string_literals;

// Temp workaround to speed up builds
#include <vulkan/vulkan.hpp>
