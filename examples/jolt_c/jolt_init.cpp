/* jolt_init.cpp — implementation of JoltHelpers.
   Compiled with -include Jolt/Jolt.h (force-included by run.sh). */

#define JOLT_BUILD_LIBRARY

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <cstdio>

#include "jolt_init_wrapper.h"

using namespace JPH;

static void sTrace(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    putchar('\n');
}

JPH_IF_ENABLE_ASSERTS(
static bool sAssertFailed(const char *inExpr, const char *inMsg,
                          const char *inFile, uint inLine)
{
    sTrace("%s:%u: (%s) %s", inFile, inLine, inExpr, inMsg ? inMsg : "");
    return true;
}
)

void JoltHelpers::Init()
{
    Trace = sTrace;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = sAssertFailed;)
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();
}

void JoltHelpers::Shutdown()
{
    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
}

