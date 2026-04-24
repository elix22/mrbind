// Bullet Physics C# binding example.
// Simulates a sphere falling under gravity onto a ground plane.

class Program
{
    static void Main()
    {
        // World setup
        using var config     = new Bullet.BtDefaultCollisionConfiguration(new Bullet._ByValue_BtDefaultCollisionConfiguration());
        using var dispatcher = new Bullet.BtCollisionDispatcher((Bullet.BtCollisionConfiguration)config);
        using var broadphase = new Bullet.BtDbvtBroadphase();
        using var solver     = new Bullet.BtSequentialImpulseConstraintSolver();
        using var world      = new Bullet.BtDiscreteDynamicsWorld(
                                    (Bullet.BtDispatcher)dispatcher,
                                    (Bullet.BtBroadphaseInterface)broadphase,
                                    (Bullet.BtConstraintSolver)solver,
                                    (Bullet.BtCollisionConfiguration)config);

        world.SetGravity(new Bullet.BtVector3(0.0, -9.81, 0.0));

        // Ground plane (static, mass = 0)
        using var groundShape  = new Bullet.BtBoxShape(new Bullet.BtVector3(50.0, 1.0, 50.0));
        using var groundTrans  = new Bullet.BtTransform(
                                     new Bullet.BtQuaternion(0, 0, 0, 1),
                                     new Bullet.BtVector3(0, -1, 0));
        using var groundMotion = new Bullet.BtDefaultMotionState(groundTrans);
        using var groundInfo   = new Bullet.BtRigidBody.BtRigidBodyConstructionInfo(
                                     0.0,
                                     (Bullet.BtMotionState)groundMotion,
                                     (Bullet.BtCollisionShape)groundShape,
                                     new Bullet.BtVector3(0, 0, 0));
        using var groundBody   = new Bullet.BtRigidBody(groundInfo);
        world.AddRigidBody(groundBody);

        // Falling sphere (dynamic, mass = 1)
        using var sphereShape  = new Bullet.BtSphereShape(1.0);
        using var sphereTrans  = new Bullet.BtTransform(
                                     new Bullet.BtQuaternion(0, 0, 0, 1),
                                     new Bullet.BtVector3(0, 20, 0));
        using var sphereMotion = new Bullet.BtDefaultMotionState(sphereTrans);
        using var inertia      = new Bullet.BtVector3(0, 0, 0);
        sphereShape.CalculateLocalInertia(1.0, inertia);
        using var sphereInfo   = new Bullet.BtRigidBody.BtRigidBodyConstructionInfo(
                                     1.0,
                                     (Bullet.BtMotionState)sphereMotion,
                                     (Bullet.BtCollisionShape)sphereShape,
                                     inertia);
        using var sphereBody   = new Bullet.BtRigidBody(sphereInfo);
        world.AddRigidBody(sphereBody);

        // Simulate 120 steps (2 seconds at 60 Hz)
        Console.WriteLine("Simulating sphere falling under gravity...");
        for (int i = 0; i < 120; i++)
        {
            world.StepSimulation(1.0 / 60.0, 10, 1.0 / 60.0);

            if (i % 20 == 0)
            {
                using var trans = new Bullet.BtTransform();
                sphereBody.GetMotionState()!.GetWorldTransform(trans);
                double y = trans.GetOrigin().Y();
                Console.WriteLine($"  step {i,3}: y = {y:F4}");
            }
        }

        // Cleanup (using blocks handle Dispose automatically)
        world.RemoveRigidBody(sphereBody);
        world.RemoveRigidBody(groundBody);
    }
}
