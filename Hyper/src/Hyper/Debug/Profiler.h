#pragma once
#include <optick.h>

#ifdef HYPER_DEBUG
// #define USE_OPTICK 1
#define HPR_PROFILE_FRAME(name, ...)            OPTICK_FRAME(name, __VA_ARGS__)
#define HPR_PROFILE_SCOPE(...)                  OPTICK_EVENT(__VA_ARGS__)
#define HPR_PROFILE_SCOPE_DYNAMIC(name)         OPTICK_EVENT_DYNAMIC(name)
#define HPR_PROFILE_CATEGORY(name, category)    OPTICK_CATEGORY(name, category)
#define HPR_PROFILE_THREAD(threadName)          OPTICK_THREAD(threadName)
#define HPR_PROFILE_TAG(name, ...)              OPTICK_TAG(name, __VA_ARGS__)
#define HPR_PROFILE_GPU_CONTEXT(...)            OPTICK_GPU_CONTEXT(__VA_ARGS__)
#define HPR_PROFILE_GPU_EVENT(name)             OPTICK_GPU_EVENT(name)
#define HPR_PROFILE_SHUTDOWN()                  OPTICK_SHUTDOWN()
#else
// #define USE_OPTICK 0
#define HPR_PROFILE_FRAME(name, ...)
#define HPR_PROFILE_SCOPE(...)
#define HPR_PROFILE_CATEGORY(name, category)
#define HPR_PROFILE_THREAD(threadName)
#define HPR_PROFILE_TAG(name, ...)
#define HPR_PROFILE_GPU_CONTEXT(...)
#define HPR_PROFILE_GPU_EVENT(name)
#define HPR_PROFILE_SHUTDOWN()
#endif
