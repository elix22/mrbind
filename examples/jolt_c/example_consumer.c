#include <jolt/jolt_helper.h>
#include <stdio.h>

int main(void)
{
    /* --- Create physics system (also initialises Jolt) --- */
    JoltPhysicsSystem *sys = JoltPhysicsSystem_DefaultConstruct();
    JoltPhysicsSystem_SetGravity(sys, 0.0, -9.81, 0.0);

    /* --- Static ground box: 100x2x100 centred at (0,-1,0) --- */
    JoltBoxShape *floorShape = JoltBoxShape_Construct_3(50.0, 1.0, 50.0);
    JoltBodyCreationSettings *floorSettings = JoltBodyCreationSettings_Construct_6(
        JoltBoxShape_MutableUpcastTo_JoltShape(floorShape),
        0.0, -1.0, 0.0,
        0,   /* JoltMotionType_Static */
        0u); /* JoltObjectLayer_NonMoving */
    JoltBodyID *floorId = JoltPhysicsSystem_CreateAndAddBody(
        sys, floorSettings, 1); /* JoltActivation_DontActivate */
    JoltBodyCreationSettings_Destroy(floorSettings);

    /* --- Dynamic sphere: radius 0.5 starting at (0,20,0) --- */
    JoltSphereShape *sphereShape = JoltSphereShape_Construct(0.5f);
    JoltBodyCreationSettings *sphereSettings = JoltBodyCreationSettings_Construct_6(
        JoltSphereShape_MutableUpcastTo_JoltShape(sphereShape),
        0.0, 20.0, 0.0,
        2,   /* JoltMotionType_Dynamic */
        1u); /* JoltObjectLayer_Moving */
    JoltBodyID *sphereId = JoltPhysicsSystem_CreateAndAddBody(
        sys, sphereSettings, 0); /* JoltActivation_Activate */
    JoltBodyCreationSettings_Destroy(sphereSettings);

    JoltPhysicsSystem_OptimizeBroadPhase(sys);

    /* --- Simulate 120 steps (2 seconds at 60 Hz) --- */
    printf("Simulating sphere falling under gravity...\n");
    for (int i = 0; i < 120; i++)
    {
        JoltPhysicsSystem_Update(sys, 1.0f / 60.0f, 1);

        if (i % 20 == 0)
        {
            JoltVec3 *pos = JoltPhysicsSystem_GetBodyPosition(sys, sphereId);
            printf("  step %3d: y = %.4f  active=%d\n",
                i,
                *JoltVec3_Get_y(pos),
                JoltPhysicsSystem_IsBodyActive(sys, sphereId));
            JoltVec3_Destroy(pos);
        }
    }

    /* --- Print final body counts --- */
    printf("Bodies: total=%u  active=%u\n",
        JoltPhysicsSystem_GetNumBodies(sys),
        JoltPhysicsSystem_GetNumActiveBodies(sys));

    /* --- Cleanup --- */
    JoltPhysicsSystem_RemoveAndDestroyBody(sys, sphereId);
    JoltBodyID_Destroy(sphereId);

    JoltPhysicsSystem_RemoveAndDestroyBody(sys, floorId);
    JoltBodyID_Destroy(floorId);

    /* Shapes are ref-counted; destroying the C++ wrapper releases the ref */
    JoltSphereShape_Destroy(sphereShape);
    JoltBoxShape_Destroy(floorShape);

    JoltPhysicsSystem_Destroy(sys);

    return 0;
}
