// Jolt Physics C# binding example.
// Demonstrates JoltPhysicsSystem with explicit shape and body creation.

class Program
{
    static void Main()
    {
        // Create physics system (also initialises Jolt internally)
        using var sys = new Jolt.JoltPhysicsSystem();
        sys.SetGravity(0.0, -9.81, 0.0);

        // Static ground box: 100x2x100 centred at (0,-1,0)
        using var floorShape = new Jolt.JoltBoxShape(50.0, 1.0, 50.0);
        using var floorSettings = new Jolt.JoltBodyCreationSettings(
            floorShape,
            0.0, -1.0, 0.0,
            0,       /* JoltMotionType_Static */
            0u);     /* JoltObjectLayer_NonMoving */
        using var floorId = sys.CreateAndAddBody(floorSettings, 1); /* JoltActivation_DontActivate */

        // Dynamic sphere: radius 0.5 starting at (0,20,0)
        using var sphereShape = new Jolt.JoltSphereShape(0.5f);
        using var sphereSettings = new Jolt.JoltBodyCreationSettings(
            sphereShape,
            0.0, 20.0, 0.0,
            2,       /* JoltMotionType_Dynamic */
            1u);     /* JoltObjectLayer_Moving */
        using var sphereId = sys.CreateAndAddBody(sphereSettings, 0); /* JoltActivation_Activate */

        sys.OptimizeBroadPhase();

        // Simulate 120 steps (2 seconds at 60 Hz)
        Console.WriteLine("Simulating sphere falling under gravity...");
        for (int i = 0; i < 120; i++)
        {
            sys.Update(1.0f / 60.0f, 1);

            if (i % 20 == 0)
            {
                using var pos = sys.GetBodyPosition(sphereId);
                double y = pos.y;
                bool active = sys.IsBodyActive(sphereId);
                Console.WriteLine($"  step {i,3}: y = {y:F4}  active={active}");
            }
        }

        // Print body counts
        Console.WriteLine($"Bodies: total={sys.GetNumBodies()}  active={sys.GetNumActiveBodies()}");

        // Cleanup (using blocks handle Dispose; body IDs are freed last)
        sys.RemoveAndDestroyBody(sphereId);
        sys.RemoveAndDestroyBody(floorId);
    }
}
