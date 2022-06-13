//*********************************************************
//
// Copyright (c) 2019-2022, NVIDIA CORPORATION. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//
//*********************************************************

#pragma once

#ifdef HYPER_USE_AFTERMATH

#include <iomanip>
#include <string>
#include <sstream>

#include <vulkan/vulkan.hpp>
#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"

//*********************************************************
// Some std::to_string overloads for some Nsight Aftermath
// API types.
//

namespace std
{
    template<typename T>
    inline std::string to_hex_string(T n)
    {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2 * sizeof(T)) << std::hex << n;
        return stream.str();
    }

    inline std::string to_string(GFSDK_Aftermath_Result result)
    {
        return std::string("0x") + to_hex_string(static_cast<uint32_t>(result));
    }

    inline std::string to_string(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier)
    {
        return to_hex_string(identifier.id[0]) + "-" + to_hex_string(identifier.id[1]);
    }

    inline std::string to_string(const GFSDK_Aftermath_ShaderBinaryHash& hash)
    {
        return to_hex_string(hash.hash);
    }
} // namespace std

//*********************************************************
// Helper for comparing shader hashes and debug info identifier.
//

// Helper for comparing GFSDK_Aftermath_ShaderDebugInfoIdentifier.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& lhs, const GFSDK_Aftermath_ShaderDebugInfoIdentifier& rhs)
{
    if (lhs.id[0] == rhs.id[0])
    {
        return lhs.id[1] < rhs.id[1];
    }
    return lhs.id[0] < rhs.id[0];
}

// Helper for comparing GFSDK_Aftermath_ShaderBinaryHash.
inline bool operator<(const GFSDK_Aftermath_ShaderBinaryHash& lhs, const GFSDK_Aftermath_ShaderBinaryHash& rhs)
{
    return lhs.hash < rhs.hash;
}

// Helper for comparing GFSDK_Aftermath_ShaderDebugName.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugName& lhs, const GFSDK_Aftermath_ShaderDebugName& rhs)
{
    return strncmp(lhs.name, rhs.name, sizeof(lhs.name)) < 0;
}

//*********************************************************
// Helper for checking Nsight Aftermath failures.
//

inline std::string  AftermathErrorMessage(GFSDK_Aftermath_Result result)
{
    const auto resultToString = [](GFSDK_Aftermath_Result result)
    {
	    switch (result)
	    {
	    case GFSDK_Aftermath_Result_Success: return "GFSDK_Aftermath_Result_Success";
	    case GFSDK_Aftermath_Result_NotAvailable: return "GFSDK_Aftermath_Result_NotAvailable";
	    case GFSDK_Aftermath_Result_Fail: return "GFSDK_Aftermath_Result_Fail";
	    case GFSDK_Aftermath_Result_FAIL_VersionMismatch: return "GFSDK_Aftermath_Result_FAIL_VersionMismatch";
	    case GFSDK_Aftermath_Result_FAIL_NotInitialized: return "GFSDK_Aftermath_Result_FAIL_NotInitialized";
	    case GFSDK_Aftermath_Result_FAIL_InvalidAdapter: return "GFSDK_Aftermath_Result_FAIL_InvalidAdapter";
	    case GFSDK_Aftermath_Result_FAIL_InvalidParameter: return "GFSDK_Aftermath_Result_FAIL_InvalidParameter";
	    case GFSDK_Aftermath_Result_FAIL_Unknown: return "GFSDK_Aftermath_Result_FAIL_Unknown";
	    case GFSDK_Aftermath_Result_FAIL_ApiError: return "GFSDK_Aftermath_Result_FAIL_ApiError";
	    case GFSDK_Aftermath_Result_FAIL_NvApiIncompatible: return "GFSDK_Aftermath_Result_FAIL_NvApiIncompatible";
	    case GFSDK_Aftermath_Result_FAIL_GettingContextDataWithNewCommandList: return "GFSDK_Aftermath_Result_FAIL_GettingContextDataWithNewCommandList";
	    case GFSDK_Aftermath_Result_FAIL_AlreadyInitialized: return "GFSDK_Aftermath_Result_FAIL_AlreadyInitialized";
	    case GFSDK_Aftermath_Result_FAIL_D3DDebugLayerNotCompatible: return "GFSDK_Aftermath_Result_FAIL_D3DDebugLayerNotCompatible";
	    case GFSDK_Aftermath_Result_FAIL_DriverInitFailed: return "GFSDK_Aftermath_Result_FAIL_DriverInitFailed";
	    case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported: return "GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported";
	    case GFSDK_Aftermath_Result_FAIL_OutOfMemory: return "GFSDK_Aftermath_Result_FAIL_OutOfMemory";
	    case GFSDK_Aftermath_Result_FAIL_GetDataOnBundle: return "GFSDK_Aftermath_Result_FAIL_GetDataOnBundle";
	    case GFSDK_Aftermath_Result_FAIL_GetDataOnDeferredContext: return "GFSDK_Aftermath_Result_FAIL_GetDataOnDeferredContext";
	    case GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled: return "GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled";
	    case GFSDK_Aftermath_Result_FAIL_NoResourcesRegistered: return "GFSDK_Aftermath_Result_FAIL_NoResourcesRegistered";
	    case GFSDK_Aftermath_Result_FAIL_ThisResourceNeverRegistered: return "GFSDK_Aftermath_Result_FAIL_ThisResourceNeverRegistered";
	    case GFSDK_Aftermath_Result_FAIL_NotSupportedInUWP: return "GFSDK_Aftermath_Result_FAIL_NotSupportedInUWP";
	    case GFSDK_Aftermath_Result_FAIL_D3dDllNotSupported: return "GFSDK_Aftermath_Result_FAIL_D3dDllNotSupported";
	    case GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported: return "GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported";
	    case GFSDK_Aftermath_Result_FAIL_Disabled: return "GFSDK_Aftermath_Result_FAIL_Disabled";
	    default: return "unrecognized result value";
	    }
    };
    switch (result)
    {
    case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
        return "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
    default:
        return "Aftermath Error 0x" + std::to_hex_string(result) + " (" + resultToString(result) + ")";
    }
}

// Helper macro for checking Nsight Aftermath results and throwing exception
// in case of a failure.
#ifdef _WIN32
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
[&]() {                                                                                                 \
    GFSDK_Aftermath_Result _result = FC;                                                                \
    if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
    {                                                                                                   \
        MessageBoxA(0, AftermathErrorMessage(_result).c_str(), "Aftermath Error", MB_OK);               \
        exit(1);                                                                                        \
    }                                                                                                   \
}()
#else
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
[&]() {                                                                                                 \
    GFSDK_Aftermath_Result _result = FC;                                                                \
    if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
    {                                                                                                   \
        printf("%s\n", AftermathErrorMessage(_result).c_str());                                         \
        fflush(stdout);                                                                                 \
        exit(1);                                                                                        \
    }                                                                                                   \
}()
#endif

#endif
