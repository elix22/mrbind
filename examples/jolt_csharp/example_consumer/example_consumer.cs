// Jolt Physics C# example.
// Uses only machine-generated bindings from mrbind/mrbind_gen_c/mrbind_gen_csharp.
// Zero manual [DllImport] declarations.

class Program
{
    const ushort ObjLayerNonMoving = 0;
    const ushort ObjLayerMoving    = 1;

    static void Main()
    {
        /* --- Jolt runtime init --- */
        JPH.Const_JoltHelpers.Init();

        /* --- Allocator and job system --- */
        using var alloc = new JPH.TempAllocatorImpl(64 * 1024 * 1024);
        using var jobs  = new JPH.JobSystemThreadPool(2048, 8, -1);

        /* --- Broad-phase layer mapping: NonMoving->BP0, Moving->BP1 --- */
        using var bpInterface = new JPH.BroadPhaseLayerInterfaceTable(2, 2);
        using var bp0 = new JPH.BroadPhaseLayer(0);
        using var bp1 = new JPH.BroadPhaseLayer(1);
        bpInterface.MapObjectToBroadPhaseLayer(ObjLayerNonMoving, bp0);
        bpInterface.MapObjectToBroadPhaseLayer(ObjLayerMoving,    bp1);

        /* --- Object layer pair filter: Moving collides with NonMoving and Moving --- */
        using var pairFilter = new JPH.ObjectLayerPairFilterTable(2);
        pairFilter.EnableCollision(ObjLayerMoving, ObjLayerNonMoving);
        pairFilter.EnableCollision(ObjLayerMoving, ObjLayerMoving);

        /* --- Object-vs-broadphase filter --- */
        /* Explicit casts resolve ambiguous user-defined conversions (mutable vs const). */
        using var objVsBP = new JPH.ObjectVsBroadPhaseLayerFilterTable(
            (JPH.Const_BroadPhaseLayerInterfaceTable)bpInterface, 2,
            (JPH.Const_ObjectLayerPairFilterTable)pairFilter, 2);

        /* --- Physics system --- */
        using var sys = new JPH.PhysicsSystem();
        sys.Init(1024, 0, 1024, 1024,
            (JPH.Const_BroadPhaseLayerInterfaceTable)bpInterface,
            (JPH.Const_ObjectVsBroadPhaseLayerFilterTable)objVsBP,
            (JPH.Const_ObjectLayerPairFilterTable)pairFilter);
        { using var gravity = new JPH.Vec3(0f, -9.81f, 0f); sys.SetGravity(gravity); }

        var bi = sys.GetBodyInterface();

        /* --- Static ground box: 100x2x100 centred at (0,-1,0) --- */
        using var floorHalfExtent = new JPH.Vec3(50f, 1f, 50f);
        var floorSS = new JPH.BoxShapeSettings(floorHalfExtent);
        using var floorCS = new JPH.BodyCreationSettings();
        floorCS.SetShapeSettings((JPH.Const_BoxShapeSettings)floorSS);
        GC.SuppressFinalize(floorSS); /* floorCS now owns floorSS via Ref<> */
        floorCS.mPosition.Set(0f, -1f, 0f);
        floorCS.mMotionType  = JPH.EMotionType.Static;
        floorCS.mObjectLayer = ObjLayerNonMoving;
        var floorId = bi.CreateAndAddBody(floorCS, JPH.EActivation.DontActivate);

        /* --- Dynamic sphere: radius 0.5 starting at (0,20,0) --- */
        /* No 'using': sphereCS takes Ref<> ownership via SetShapeSettings; C# must not double-free. */
        var sphereSS = new JPH.SphereShapeSettings();
        sphereSS.mRadius = 0.5f;
        using var sphereCS = new JPH.BodyCreationSettings();
        sphereCS.SetShapeSettings((JPH.Const_SphereShapeSettings)sphereSS);
        GC.SuppressFinalize(sphereSS); /* sphereCS now owns sphereSS via Ref<> */
        sphereCS.mPosition.Set(0f, 20f, 0f);
        sphereCS.mMotionType  = JPH.EMotionType.Dynamic;
        sphereCS.mObjectLayer = ObjLayerMoving;
        var sphereId = bi.CreateAndAddBody(sphereCS, JPH.EActivation.Activate);

        sys.OptimizeBroadPhase();

        /* --- Simulate 120 steps (2 seconds at 60 Hz) --- */
        Console.WriteLine("Simulating sphere falling under gravity...");
        for (int i = 0; i < 120; i++)
        {
            sys.Update(1f / 60f, 1, alloc, jobs);

            if (i % 20 == 0)
            {
                using var pos = bi.GetCenterOfMassPosition(sphereId);
                float y      = pos.GetY();
                bool  active = bi.IsActive(sphereId);
                Console.WriteLine($"  step {i,3}: y = {y:F4}  active={active}");
            }
        }

        /* --- Print final body count --- */
        Console.WriteLine($"Bodies: total={sys.GetNumBodies()}");

        /* --- Cleanup --- */
        bi.RemoveBody(sphereId);
        bi.DestroyBody(sphereId);
        bi.RemoveBody(floorId);
        bi.DestroyBody(floorId);

        /* sys, objVsBP, pairFilter, bpInterface, jobs, alloc disposed by using blocks */
        JPH.Const_JoltHelpers.Shutdown();
    }
}
