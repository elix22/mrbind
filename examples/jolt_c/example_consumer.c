#include <common.h>
#include <jolt/Jolt/Math/Vec3.h>
#include <jolt/Jolt/Core/TempAllocator.h>
#include <jolt/Jolt/Core/JobSystemThreadPool.h>
#include <jolt/Jolt/Physics/PhysicsSystem.h>
#include <jolt/Jolt/Physics/EActivation.h>
#include <jolt/Jolt/Physics/Body/BodyInterface.h>
#include <jolt/Jolt/Physics/Body/BodyCreationSettings.h>
#include <jolt/Jolt/Physics/Collision/Shape/BoxShape.h>
#include <jolt/Jolt/Physics/Collision/Shape/SphereShape.h>
#include <jolt/Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <jolt/Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <jolt/Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <jolt/Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <jolt/jolt_init_wrapper.h>
#include <stdio.h>

/* Object and broad-phase layer indices used in this example. */
#define OBJ_LAYER_NON_MOVING 0u
#define OBJ_LAYER_MOVING     1u
#define BP_LAYER_NON_MOVING  ((unsigned char)0)
#define BP_LAYER_MOVING      ((unsigned char)1)

int main(void)
{
    /* --- Jolt runtime init --- */
    JoltHelpers_Init();

    /* --- Allocator and job system --- */
    JPH_TempAllocatorImpl *alloc =
        JPH_TempAllocatorImpl_Construct(64 * 1024 * 1024);
    int nthreads = -1; /* -1 = hardware_concurrency - 1 */
    JPH_JobSystemThreadPool *jobs =
        JPH_JobSystemThreadPool_Construct(2048, 8, &nthreads);

    /* --- Broad-phase layer mapping: NonMoving→BP0, Moving→BP1 --- */
    JPH_BroadPhaseLayerInterfaceTable *bpInterface =
        JPH_BroadPhaseLayerInterfaceTable_Construct(2u, 2u);
    {
        JPH_BroadPhaseLayer *bp0 = JPH_BroadPhaseLayer_Construct(BP_LAYER_NON_MOVING);
        JPH_BroadPhaseLayer *bp1 = JPH_BroadPhaseLayer_Construct(BP_LAYER_MOVING);
        JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(
            bpInterface, OBJ_LAYER_NON_MOVING, bp0);
        JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(
            bpInterface, OBJ_LAYER_MOVING, bp1);
        JPH_BroadPhaseLayer_Destroy(bp0);
        JPH_BroadPhaseLayer_Destroy(bp1);
    }

    /* --- Object layer pair filter: Moving collides with NonMoving and Moving --- */
    JPH_ObjectLayerPairFilterTable *pairFilter =
        JPH_ObjectLayerPairFilterTable_Construct(2u);
    JPH_ObjectLayerPairFilterTable_EnableCollision(
        pairFilter, OBJ_LAYER_MOVING, OBJ_LAYER_NON_MOVING);
    JPH_ObjectLayerPairFilterTable_EnableCollision(
        pairFilter, OBJ_LAYER_MOVING, OBJ_LAYER_MOVING);

    /* --- Object-vs-broadphase filter (derived from the two above) --- */
    JPH_ObjectVsBroadPhaseLayerFilterTable *objVsBP =
        JPH_ObjectVsBroadPhaseLayerFilterTable_Construct(
            JPH_BroadPhaseLayerInterfaceTable_UpcastTo_JPH_BroadPhaseLayerInterface(
                bpInterface),
            2u,
            JPH_ObjectLayerPairFilterTable_UpcastTo_JPH_ObjectLayerPairFilter(
                pairFilter),
            2u);

    /* --- Physics system --- */
    JPH_PhysicsSystem *sys = JPH_PhysicsSystem_DefaultConstruct();
    JPH_PhysicsSystem_Init(
        sys,
        1024u,  /* maxBodies */
        0u,     /* numBodyMutexes (0 = default) */
        1024u,  /* maxBodyPairs */
        1024u,  /* maxContactConstraints */
        JPH_BroadPhaseLayerInterfaceTable_UpcastTo_JPH_BroadPhaseLayerInterface(
            bpInterface),
        JPH_ObjectVsBroadPhaseLayerFilterTable_UpcastTo_JPH_ObjectVsBroadPhaseLayerFilter(
            objVsBP),
        JPH_ObjectLayerPairFilterTable_UpcastTo_JPH_ObjectLayerPairFilter(
            pairFilter));
    {
        JPH_Vec3 *gravity = JPH_Vec3_Construct_3(0.0f, -9.81f, 0.0f);
        JPH_PhysicsSystem_SetGravity(sys, gravity);
        JPH_Vec3_Destroy(gravity);
    }

    JPH_BodyInterface *bi = JPH_PhysicsSystem_GetBodyInterface_mut(sys);

    /* --- Static ground box: 100x2x100 centred at (0,-1,0) --- */
    JPH_Vec3 *floorHalfExtent = JPH_Vec3_Construct_3(50.0f, 1.0f, 50.0f);
    JPH_BoxShapeSettings *floorSS = JPH_BoxShapeSettings_Construct(floorHalfExtent, NULL, NULL);
    JPH_Vec3_Destroy(floorHalfExtent);
    JPH_BodyCreationSettings *floorCS = JPH_BodyCreationSettings_DefaultConstruct();
    JPH_BodyCreationSettings_SetShapeSettings(
        floorCS,
        JPH_BoxShapeSettings_MutableUpcastTo_JPH_ShapeSettings(floorSS));

    JPH_Vec3_Set(JPH_BodyCreationSettings_GetMutable_mPosition(floorCS), 0.0f, -1.0f, 0.0f);
    JPH_BodyCreationSettings_Set_mMotionType(floorCS, JPH_EMotionType_Static);
    JPH_BodyCreationSettings_Set_mObjectLayer(floorCS, OBJ_LAYER_NON_MOVING);
    JPH_BodyID floorId = JPH_BodyInterface_CreateAndAddBody(
        bi, floorCS, JPH_EActivation_DontActivate);
    /* floorCS owns floorSS via Ref<> refcounting — destroying floorCS also frees floorSS */
    JPH_BodyCreationSettings_Destroy(floorCS);

    /* --- Dynamic sphere: radius 0.5 starting at (0,20,0) --- */
    JPH_SphereShapeSettings *sphereSS = JPH_SphereShapeSettings_DefaultConstruct();
    JPH_SphereShapeSettings_Set_mRadius(sphereSS, 0.5f);
    JPH_BodyCreationSettings *sphereCS = JPH_BodyCreationSettings_DefaultConstruct();
    JPH_BodyCreationSettings_SetShapeSettings(
        sphereCS,
        JPH_SphereShapeSettings_MutableUpcastTo_JPH_ShapeSettings(sphereSS));
    JPH_Vec3_Set(JPH_BodyCreationSettings_GetMutable_mPosition(sphereCS), 0.0f, 20.0f, 0.0f);
    JPH_BodyCreationSettings_Set_mMotionType(sphereCS, JPH_EMotionType_Dynamic);
    JPH_BodyCreationSettings_Set_mObjectLayer(sphereCS, OBJ_LAYER_MOVING);
    JPH_BodyID sphereId = JPH_BodyInterface_CreateAndAddBody(
        bi, sphereCS, JPH_EActivation_Activate);
    /* sphereCS owns sphereSS via Ref<> — destroying sphereCS also frees sphereSS */
    JPH_BodyCreationSettings_Destroy(sphereCS);

    JPH_PhysicsSystem_OptimizeBroadPhase(sys);

    /* --- Simulate 120 steps (2 seconds at 60 Hz) --- */
    printf("Simulating sphere falling under gravity...\n");
    for (int i = 0; i < 120; i++)
    {
        JPH_PhysicsSystem_Update(
            sys, 1.0f / 60.0f, 1,
            JPH_TempAllocatorImpl_MutableUpcastTo_JPH_TempAllocator(alloc),
            JPH_JobSystemThreadPool_MutableUpcastTo_JPH_JobSystem(jobs));

        if (i % 20 == 0)
        {
            JPH_Vec3 *pos = JPH_BodyInterface_GetCenterOfMassPosition(bi, &sphereId);
            float y = JPH_Vec3_GetY(pos);
            JPH_Vec3_Destroy(pos);
            int active = JPH_BodyInterface_IsActive(bi, &sphereId);
            printf("  step %3d: y = %.4f  active=%d\n", i, (double)y, active);
        }
    }

    /* --- Print final body count --- */
    printf("Bodies: total=%u\n", JPH_PhysicsSystem_GetNumBodies(sys));

    /* --- Cleanup --- */
    JPH_BodyInterface_RemoveBody(bi, &sphereId);
    JPH_BodyInterface_DestroyBody(bi, &sphereId);
    JPH_BodyInterface_RemoveBody(bi, &floorId);
    JPH_BodyInterface_DestroyBody(bi, &floorId);

    JPH_PhysicsSystem_Destroy(sys);
    JPH_ObjectVsBroadPhaseLayerFilterTable_Destroy(objVsBP);
    JPH_ObjectLayerPairFilterTable_Destroy(pairFilter);
    JPH_BroadPhaseLayerInterfaceTable_Destroy(bpInterface);
    JPH_JobSystemThreadPool_Destroy(jobs);
    JPH_TempAllocatorImpl_Destroy(alloc);

    JoltHelpers_Shutdown();

    return 0;
}
