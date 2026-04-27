#pragma once
/* jolt_init_wrapper.h — Parsed by mrbind to auto-generate C and C# bindings.
 * Only contains APIs that cannot be expressed through Jolt's own headers:
 *   - Global lifecycle (mrbind only binds named types, not free functions)
 *   - Vec3/RVec3 bridge methods (SIMD types mrbind cannot bind directly) */

#include <Jolt/Jolt.h>

/// Minimal helpers for Jolt global lifecycle.
/// These are the only hand-implemented methods; their C/C# bindings are machine-generated.
struct JoltHelpers
{
    /// Initialize Jolt: set Trace, RegisterDefaultAllocator, create Factory, RegisterTypes.
    static void Init();
    static void Shutdown();
};
