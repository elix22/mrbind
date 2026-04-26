#pragma once

// Simplified C++ wrapper around JoltPhysics for mrbind binding (C and C#).
// None of the Jolt headers are exposed here — only standard C++ types.
// Based on the joltc reference binding (references/joltc/).

// ---------------------------------------------------------------------------
// Primitive math types
// ---------------------------------------------------------------------------

/// Single-precision 3D vector (velocities, forces, normals).
struct JoltVec3f {
    float x, y, z;
    JoltVec3f();
    JoltVec3f(float x, float y, float z);
};

/// Double-precision 3D vector (world-space positions).
struct JoltVec3 {
    double x, y, z;
    JoltVec3();
    JoltVec3(double x, double y, double z);
};

/// Single-precision quaternion (rotation).
struct JoltQuat {
    float x, y, z, w;
    JoltQuat();
    JoltQuat(float x, float y, float z, float w);
    static JoltQuat Identity();
    bool IsNormalized(float tolerance) const;
    JoltVec3f RotateAxisX() const;
    JoltVec3f RotateAxisY() const;
    JoltVec3f RotateAxisZ() const;
};

// ---------------------------------------------------------------------------
// Motion / activation / layer constants (passed as int / unsigned int)
// ---------------------------------------------------------------------------

/// JoltMotionType values (passed as int to API functions).
static const int JoltMotionType_Static    = 0;
static const int JoltMotionType_Kinematic = 1;
static const int JoltMotionType_Dynamic   = 2;

/// JoltActivation values (passed as int to API functions).
static const int JoltActivation_Activate     = 0;
static const int JoltActivation_DontActivate = 1;

/// JoltObjectLayer values (passed as unsigned int to API functions).
static const unsigned int JoltObjectLayer_NonMoving = 0;
static const unsigned int JoltObjectLayer_Moving    = 1;

// ---------------------------------------------------------------------------
// Body ID
// ---------------------------------------------------------------------------

/// Opaque handle to a physics body.
struct JoltBodyID {
    unsigned int value;
    JoltBodyID();
    bool IsValid() const;
    bool IsInvalid() const;
};

/// Opaque handle to a constraint.
struct JoltConstraintID {
    unsigned int value;
    JoltConstraintID();
    bool IsValid() const;
};

// ---------------------------------------------------------------------------
// Shapes
// ---------------------------------------------------------------------------

/// Base class for all collision shapes.
/// Shapes are ref-counted; call Release() when you no longer need the handle.
struct JoltShape {
public:
    virtual ~JoltShape();
    void Release();
    bool IsValid() const;
    void* getHandle() const;
protected:
    void* mHandle;
    bool mOwning;
    JoltShape();
};

/// Axis-aligned box shape.
struct JoltBoxShape : public JoltShape {
public:
    JoltBoxShape(double halfX, double halfY, double halfZ,
                 float convexRadius);
    JoltBoxShape(double halfX, double halfY, double halfZ);
};

/// Sphere shape.
struct JoltSphereShape : public JoltShape {
public:
    JoltSphereShape(float radius);
};

/// Capsule shape (cylinder with hemispherical caps).  halfHeight is the
/// half-height of the cylinder part only (total body height = 2*(halfHeight+radius)).
struct JoltCapsuleShape : public JoltShape {
public:
    JoltCapsuleShape(float halfHeight, float radius);
};

/// Upright cylinder shape.
struct JoltCylinderShape : public JoltShape {
public:
    JoltCylinderShape(float halfHeight, float radius,
                      float convexRadius);
    JoltCylinderShape(float halfHeight, float radius);
};

/// A shape rotated and translated relative to a child shape.
struct JoltRotatedTranslatedShape : public JoltShape {
public:
    JoltRotatedTranslatedShape(JoltShape* inner,
                                double posX, double posY, double posZ,
                                float qx, float qy, float qz, float qw);
};

// ---------------------------------------------------------------------------
// Body creation settings
// ---------------------------------------------------------------------------

/// Parameters used when adding a body to the physics system.
struct JoltBodyCreationSettings {
public:
    /// Create settings for a body with the given shape.
    /// layer should be one of JoltObjectLayer values (0=NonMoving, 1=Moving).
    JoltBodyCreationSettings(JoltShape* shape,
                              double posX, double posY, double posZ,
                              float qx, float qy, float qz, float qw,
                              int motionType,
                              unsigned int objectLayer);

    /// Shorthand: identity rotation.
    JoltBodyCreationSettings(JoltShape* shape,
                              double posX, double posY, double posZ,
                              int motionType,
                              unsigned int objectLayer);

    ~JoltBodyCreationSettings();

    void SetPosition(double x, double y, double z);
    void SetRotation(float qx, float qy, float qz, float qw);
    void SetLinearVelocity(float vx, float vy, float vz);
    void SetAngularVelocity(float vx, float vy, float vz);
    void SetFriction(float f);
    void SetRestitution(float r);
    void SetGravityFactor(float f);
    void SetIsSensor(bool isSensor);
    void SetObjectLayer(unsigned int layer);

    void* mHandle;
};

// ---------------------------------------------------------------------------
// Physics system
// ---------------------------------------------------------------------------

/// The main Jolt physics simulation. Manages a job system, temp allocator,
/// and the full PhysicsSystem internally. The Jolt library itself is
/// initialised automatically on first construction and cleaned up on last
/// destruction.
struct JoltPhysicsSystem {
public:
    /// Construct with default settings (65536 bodies, 65536 pairs,
    /// 65536 contacts, 2-layer NON_MOVING/MOVING setup).
    JoltPhysicsSystem();

    /// Construct with explicit capacities.
    JoltPhysicsSystem(unsigned int maxBodies,
                      unsigned int maxBodyPairs,
                      unsigned int maxContactConstraints);

    ~JoltPhysicsSystem();

    // ---- Simulation ----------------------------------------------------------

    void SetGravity(double x, double y, double z);
    JoltVec3 GetGravity() const;

    /// Advance the simulation. deltaTime is typically 1/60.
    /// collisionSteps: increase for fast-moving objects (normally 1).
    void Update(float deltaTime, int collisionSteps);

    /// Optimise the broad phase. Call after loading a static level before
    /// starting to simulate.
    void OptimizeBroadPhase();

    // ---- Body management -----------------------------------------------------

    /// Create a body from settings and add it to the simulation.
    JoltBodyID CreateAndAddBody(JoltBodyCreationSettings* settings,
                                int activation);

    /// Remove a body from the simulation (body data preserved, can re-add).
    void RemoveBody(JoltBodyID id);

    /// Permanently destroy a body. ID is invalid after this call.
    void DestroyBody(JoltBodyID id);

    /// Remove and destroy in one call.
    void RemoveAndDestroyBody(JoltBodyID id);

    // ---- Body queries --------------------------------------------------------

    JoltVec3  GetBodyPosition(JoltBodyID id) const;
    JoltQuat  GetBodyRotation(JoltBodyID id) const;
    JoltVec3f GetBodyLinearVelocity(JoltBodyID id) const;
    JoltVec3f GetBodyAngularVelocity(JoltBodyID id) const;
    bool      IsBodyActive(JoltBodyID id) const;

    // ---- Body control --------------------------------------------------------

    void SetBodyPosition(JoltBodyID id, double x, double y, double z,
                         int activation);
    void SetBodyRotation(JoltBodyID id,
                         float qx, float qy, float qz, float qw,
                         int activation);
    void SetBodyPositionAndRotation(JoltBodyID id,
                                    double x, double y, double z,
                                    float qx, float qy, float qz, float qw,
                                    int activation);
    void SetBodyLinearVelocity(JoltBodyID id,
                               float vx, float vy, float vz);
    void SetBodyAngularVelocity(JoltBodyID id,
                                float vx, float vy, float vz);
    void SetBodyLinearAndAngularVelocity(JoltBodyID id,
                                         float lvx, float lvy, float lvz,
                                         float avx, float avy, float avz);
    void AddForce(JoltBodyID id, float fx, float fy, float fz);
    void AddForceAtPosition(JoltBodyID id,
                            float fx, float fy, float fz,
                            double px, double py, double pz);
    void AddTorque(JoltBodyID id, float tx, float ty, float tz);
    void AddImpulse(JoltBodyID id, float ix, float iy, float iz);
    void AddAngularImpulse(JoltBodyID id, float ix, float iy, float iz);
    void ActivateBody(JoltBodyID id);
    void DeactivateBody(JoltBodyID id);

    // ---- Body properties -----------------------------------------------------

    void  SetFriction(JoltBodyID id, float friction);
    float GetFriction(JoltBodyID id) const;
    void  SetRestitution(JoltBodyID id, float restitution);
    float GetRestitution(JoltBodyID id) const;
    void  SetGravityFactor(JoltBodyID id, float factor);
    float GetGravityFactor(JoltBodyID id) const;

    // ---- Constraint helpers --------------------------------------------------

    /// Connect two bodies at fixed relative positions/orientations.
    JoltConstraintID AddFixedConstraint(JoltBodyID body1, JoltBodyID body2);

    /// Distance constraint: keep two bodies between [minDist, maxDist].
    JoltConstraintID AddDistanceConstraint(JoltBodyID body1, JoltBodyID body2,
                                            float minDist, float maxDist);

    /// Point constraint: two bodies share a common world-space pivot.
    JoltConstraintID AddPointConstraint(JoltBodyID body1, JoltBodyID body2,
                                         double pivotX, double pivotY, double pivotZ);

    /// Hinge constraint: bodies rotate about a shared axis.
    JoltConstraintID AddHingeConstraint(JoltBodyID body1, JoltBodyID body2,
                                         double pivotX, double pivotY, double pivotZ,
                                         float axisX, float axisY, float axisZ);

    /// Remove and destroy a constraint.
    void DestroyConstraint(JoltConstraintID id);

    /// Enable or disable an existing constraint.
    void SetConstraintEnabled(JoltConstraintID id, bool enabled);

    // ---- Statistics ----------------------------------------------------------

    unsigned int GetNumBodies() const;
    unsigned int GetNumActiveBodies() const;

private:
    struct Impl;
    Impl* mImpl;
};

// ---------------------------------------------------------------------------
// Convenience: JoltWorld kept for backward compatibility
// ---------------------------------------------------------------------------

/// Simplified wrapper kept for backward compatibility. Prefer JoltPhysicsSystem
/// for new code.
struct JoltWorld {
public:
    JoltWorld();
    ~JoltWorld();

    void SetGravity(double x, double y, double z);

    JoltBodyID AddStaticBox(double halfX, double halfY, double halfZ,
                             double posX, double posY, double posZ);

    JoltBodyID AddDynamicSphere(double radius,
                                 double posX, double posY, double posZ);

    void Update(double deltaTime);
    void OptimizeBroadPhase();

    JoltVec3 GetBodyPosition(JoltBodyID id) const;
    bool     IsBodyActive(JoltBodyID id) const;

    void RemoveBody(JoltBodyID id);
    void DestroyBody(JoltBodyID id);

private:
    struct Impl;
    Impl* mImpl;
};
