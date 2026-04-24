#include <common.h>
#include <bullet/LinearMath/btVector3.h>
#include <bullet/LinearMath/btQuaternion.h>
#include <bullet/LinearMath/btTransform.h>
#include <bullet/LinearMath/btMotionState.h>
#include <bullet/LinearMath/btDefaultMotionState.h>
#include <bullet/BulletCollision/CollisionShapes/btCollisionShape.h>
#include <bullet/BulletCollision/CollisionShapes/btBoxShape.h>
#include <bullet/BulletCollision/CollisionShapes/btSphereShape.h>
#include <bullet/BulletCollision/CollisionDispatch/btCollisionConfiguration.h>
#include <bullet/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <bullet/BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <bullet/BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <bullet/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <bullet/BulletDynamics/Dynamics/btRigidBody.h>
#include <bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* --- World setup --- */
    btDefaultCollisionConfiguration *config =
        btDefaultCollisionConfiguration_ConstructFromAnother(Bullet_PassBy_DefaultConstruct, NULL);

    btCollisionDispatcher *dispatcher =
        btCollisionDispatcher_Construct(
            btDefaultCollisionConfiguration_MutableUpcastTo_btCollisionConfiguration(config));

    btDbvtBroadphase *broadphase =
        btDbvtBroadphase_Construct(NULL); /* NULL = use internal pair cache */

    btSequentialImpulseConstraintSolver *solver =
        btSequentialImpulseConstraintSolver_DefaultConstruct();

    btDiscreteDynamicsWorld *world =
        btDiscreteDynamicsWorld_Construct(
            btCollisionDispatcher_MutableUpcastTo_btDispatcher(dispatcher),
            btDbvtBroadphase_MutableUpcastTo_btBroadphaseInterface(broadphase),
            btSequentialImpulseConstraintSolver_MutableUpcastTo_btConstraintSolver(solver),
            btDefaultCollisionConfiguration_MutableUpcastTo_btCollisionConfiguration(config));

    /* gravity */
    btVector3 *gravity = btVector3_Construct(&(double){0.0}, &(double){-9.81}, &(double){0.0});
    btDiscreteDynamicsWorld_setGravity(world, gravity);
    btVector3_Destroy(gravity);

    /* --- Ground plane (static rigid body, mass = 0) --- */
    btVector3 *groundHalfExtents = btVector3_Construct(&(double){50.0}, &(double){1.0}, &(double){50.0});
    btBoxShape *groundBoxShape = btBoxShape_Construct(groundHalfExtents);
    btCollisionShape *groundShape = btBoxShape_MutableUpcastTo_btCollisionShape(groundBoxShape);
    btVector3_Destroy(groundHalfExtents);

    btVector3    *groundOrigin = btVector3_Construct(&(double){0.0}, &(double){-1.0}, &(double){0.0});
    btQuaternion *groundRot    = btQuaternion_Construct_4(&(double){0.0}, &(double){0.0}, &(double){0.0}, &(double){1.0});
    btTransform  *groundTrans  = btTransform_Construct_btQuaternion(groundRot, groundOrigin);
    btVector3_Destroy(groundOrigin);
    btQuaternion_Destroy(groundRot);

    btDefaultMotionState *groundMotion =
        btDefaultMotionState_Construct(groundTrans, NULL);
    btTransform_Destroy(groundTrans);

    btVector3 *zeroInertia = btVector3_Construct(&(double){0.0}, &(double){0.0}, &(double){0.0});
    btRigidBody_btRigidBodyConstructionInfo *groundInfo =
        btRigidBody_btRigidBodyConstructionInfo_Construct(
            0.0,
            btDefaultMotionState_MutableUpcastTo_btMotionState(groundMotion),
            groundShape,
            zeroInertia);

    btRigidBody *groundBody = btRigidBody_Construct_1(groundInfo);
    btRigidBody_btRigidBodyConstructionInfo_Destroy(groundInfo);

    btDiscreteDynamicsWorld_addRigidBody_1(world, groundBody);

    /* --- Falling sphere (dynamic, mass = 1) --- */
    btSphereShape *sphereBoxShape = btSphereShape_Construct(1.0);
    btCollisionShape *sphereShape = btSphereShape_MutableUpcastTo_btCollisionShape(sphereBoxShape);

    btVector3    *sphereOrigin = btVector3_Construct(&(double){0.0}, &(double){20.0}, &(double){0.0});
    btQuaternion *sphereRot    = btQuaternion_Construct_4(&(double){0.0}, &(double){0.0}, &(double){0.0}, &(double){1.0});
    btTransform  *sphereTrans  = btTransform_Construct_btQuaternion(sphereRot, sphereOrigin);
    btVector3_Destroy(sphereOrigin);
    btQuaternion_Destroy(sphereRot);

    btDefaultMotionState *sphereMotion =
        btDefaultMotionState_Construct(sphereTrans, NULL);
    btTransform_Destroy(sphereTrans);

    btVector3 *sphereInertia = btVector3_Construct(&(double){0.0}, &(double){0.0}, &(double){0.0});
    btCollisionShape_calculateLocalInertia(sphereShape, 1.0, sphereInertia);

    btRigidBody_btRigidBodyConstructionInfo *sphereInfo =
        btRigidBody_btRigidBodyConstructionInfo_Construct(
            1.0,
            btDefaultMotionState_MutableUpcastTo_btMotionState(sphereMotion),
            sphereShape,
            sphereInertia);
    btVector3_Destroy(sphereInertia);

    btRigidBody *sphereBody = btRigidBody_Construct_1(sphereInfo);
    btRigidBody_btRigidBodyConstructionInfo_Destroy(sphereInfo);

    btDiscreteDynamicsWorld_addRigidBody_1(world, sphereBody);

    /* --- Simulate 120 steps (2 seconds at 60 Hz) --- */
    printf("Simulating sphere falling under gravity...\n");
    for (int i = 0; i < 120; i++)
    {
        btDiscreteDynamicsWorld_stepSimulation(world, 1.0 / 60.0, &(int){10}, &(double){1.0 / 60.0});

        if (i % 20 == 0)
        {
            btTransform *trans = btTransform_DefaultConstruct();
            const btMotionState *ms = btRigidBody_getMotionState(sphereBody);
            btMotionState_getWorldTransform(ms, trans);
            const btVector3 *pos = btTransform_getOrigin(trans);
            printf("  step %3d: y = %.4f\n", i, *btVector3_y(pos));
            btTransform_Destroy(trans);
        }
    }

    /* --- Cleanup --- */
    btDiscreteDynamicsWorld_removeRigidBody(world, sphereBody);
    btDiscreteDynamicsWorld_removeRigidBody(world, groundBody);
    btRigidBody_Destroy(sphereBody);
    btRigidBody_Destroy(groundBody);
    btDefaultMotionState_Destroy(sphereMotion);
    btDefaultMotionState_Destroy(groundMotion);
    btCollisionShape_Destroy(sphereShape);
    btCollisionShape_Destroy(groundShape);
    btVector3_Destroy(zeroInertia);
    btDiscreteDynamicsWorld_Destroy(world);
    btSequentialImpulseConstraintSolver_Destroy(solver);
    btDbvtBroadphase_Destroy(broadphase);
    btCollisionDispatcher_Destroy(dispatcher);
    btDefaultCollisionConfiguration_Destroy(config);

    return 0;
}
