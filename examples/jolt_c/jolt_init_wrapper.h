#pragma once
/* jolt_init_wrapper.h — Parsed by mrbind to auto-generate C and C# bindings.
 * Only contains APIs that cannot be expressed through Jolt's own headers:
 *   - Global lifecycle (mrbind only binds named types, not free functions)
 *   - Vec3/RVec3 bridge methods (SIMD types mrbind cannot bind directly) */

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyID.h>

/// Minimal helpers for Jolt global lifecycle and Vec3/RVec3 bridge operations.
/// These are the only hand-implemented methods; their C/C# bindings are machine-generated.
struct JoltHelpers
{
    /// Initialize Jolt: set Trace, RegisterDefaultAllocator, create Factory, RegisterTypes.
    static void Init();
    static void Shutdown();

    /// Create a BoxShapeSettings with the given half-extents (Vec3 bridge).
    /// Ownership passes to the caller; typically transferred to BodyCreationSettings via SetShapeSettings.
    static JPH::BoxShapeSettings *BoxShapeSettings_WithHalfExtent(float hx, float hy, float hz);

    /// Set the position of a BodyCreationSettings (RVec3 bridge; default position is origin).
    static void BodyCreationSettings_SetPosition(JPH::BodyCreationSettings *s,
                                                  float x, float y, float z);

    /// Get the Y component of a body's center-of-mass position (RVec3 bridge).
    static float BodyInterface_GetPositionY(JPH::BodyInterface *bi,
                                             const JPH::BodyID *id);
};
