// Implementation of jolt_helper.h.  All Jolt headers are confined to this TU.

#include "jolt_helper.h"

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wall"
#  pragma GCC diagnostic ignored "-Wextra"
#endif

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>

#ifdef __clang__
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#include <thread>
#include <unordered_map>
#include <vector>
#include <cstdarg>
#include <cstdio>

using namespace JPH;

// ---------------------------------------------------------------------------
// Layer constants (two-layer default setup: 0=NonMoving, 1=Moving)
// ---------------------------------------------------------------------------
static constexpr ObjectLayer LAYER_NON_MOVING = 0;
static constexpr ObjectLayer LAYER_MOVING     = 1;
static constexpr BroadPhaseLayer BP_NON_MOVING(0);
static constexpr BroadPhaseLayer BP_MOVING(1);

class DefaultBPLayer final : public BroadPhaseLayerInterface {
    BroadPhaseLayer mMap[2];
public:
    DefaultBPLayer() { mMap[0] = BP_NON_MOVING; mMap[1] = BP_MOVING; }
    uint GetNumBroadPhaseLayers() const override { return 2; }
    BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer l) const override { return mMap[l]; }
};
class DefaultObjVsBP final : public ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(ObjectLayer l1, BroadPhaseLayer l2) const override {
        return l1 == LAYER_NON_MOVING ? (l2 == BP_MOVING) : true;
    }
};
class DefaultObjVsObj final : public ObjectLayerPairFilter {
public:
    bool ShouldCollide(ObjectLayer l1, ObjectLayer l2) const override {
        return l1 == LAYER_NON_MOVING ? (l2 == LAYER_MOVING) : true;
    }
};

// ---------------------------------------------------------------------------
// Library init/shutdown reference counting
// ---------------------------------------------------------------------------
static int g_jolt_refs = 0;
static void jolt_trace(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    putchar('\n');
}
#ifdef JPH_ENABLE_ASSERTS
static bool jolt_assert_failed(const char* expr, const char* msg,
                                const char* file, unsigned int line) {
    fprintf(stderr, "JPH Assert failed: %s (%s) at %s:%u\n",
            expr, msg ? msg : "", file, line);
    return true;
}
#endif

static void jolt_lib_acquire() {
    if (g_jolt_refs++ == 0) {
        Trace = jolt_trace;
        JPH_IF_ENABLE_ASSERTS(AssertFailed = jolt_assert_failed;)
        RegisterDefaultAllocator();
        Factory::sInstance = new Factory();
        RegisterTypes();
    }
}
static void jolt_lib_release() {
    if (--g_jolt_refs == 0) {
        UnregisterTypes();
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Math helpers
// ---------------------------------------------------------------------------
static inline Vec3  toJph(const JoltVec3f& v)  { return Vec3(v.x, v.y, v.z); }
static inline Vec3  toJph3(const JoltVec3& v)   { return Vec3((float)v.x,(float)v.y,(float)v.z); }
static inline RVec3 toJphR(const JoltVec3& v)   { return RVec3(v.x, v.y, v.z); }
static inline Quat  toJphQ(const JoltQuat& q)   { return Quat(q.x, q.y, q.z, q.w); }
static inline JoltVec3f fromVec3(Vec3 v)  { return JoltVec3f(v.GetX(), v.GetY(), v.GetZ()); }
static inline JoltVec3  fromRVec3(RVec3 v){ return JoltVec3((double)v.GetX(),(double)v.GetY(),(double)v.GetZ()); }
static inline JoltQuat  fromQuat(Quat q)  { return JoltQuat(q.GetX(), q.GetY(), q.GetZ(), q.GetW()); }


// ---------------------------------------------------------------------------
// JoltVec3f
// ---------------------------------------------------------------------------
JoltVec3f::JoltVec3f() : x(0), y(0), z(0) {}
JoltVec3f::JoltVec3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

// ---------------------------------------------------------------------------
// JoltVec3
// ---------------------------------------------------------------------------
JoltVec3::JoltVec3() : x(0), y(0), z(0) {}
JoltVec3::JoltVec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

// ---------------------------------------------------------------------------
// JoltQuat
// ---------------------------------------------------------------------------
JoltQuat::JoltQuat() : x(0), y(0), z(0), w(1) {}
JoltQuat::JoltQuat(float x_, float y_, float z_, float w_)
    : x(x_), y(y_), z(z_), w(w_) {}
JoltQuat JoltQuat::Identity() { return JoltQuat(0, 0, 0, 1); }
bool JoltQuat::IsNormalized(float tolerance) const {
    return toJphQ(*this).IsNormalized(tolerance);
}
JoltVec3f JoltQuat::RotateAxisX() const { return fromVec3(toJphQ(*this).RotateAxisX()); }
JoltVec3f JoltQuat::RotateAxisY() const { return fromVec3(toJphQ(*this).RotateAxisY()); }
JoltVec3f JoltQuat::RotateAxisZ() const { return fromVec3(toJphQ(*this).RotateAxisZ()); }

// ---------------------------------------------------------------------------
// JoltBodyID
// ---------------------------------------------------------------------------
JoltBodyID::JoltBodyID() : value(BodyID().GetIndexAndSequenceNumber()) {}
bool JoltBodyID::IsValid()   const { return !BodyID(value).IsInvalid(); }
bool JoltBodyID::IsInvalid() const { return  BodyID(value).IsInvalid(); }

// ---------------------------------------------------------------------------
// JoltConstraintID
// ---------------------------------------------------------------------------
JoltConstraintID::JoltConstraintID() : value(0xFFFFFFFF) {}
bool JoltConstraintID::IsValid() const { return value != 0xFFFFFFFF; }

// ---------------------------------------------------------------------------
// JoltShape base
// ---------------------------------------------------------------------------
JoltShape::JoltShape()       : mHandle(nullptr), mOwning(false) {}
JoltShape::~JoltShape()      { Release(); }
bool JoltShape::IsValid() const { return mHandle != nullptr; }
void* JoltShape::getHandle() const { return mHandle; }
void JoltShape::Release() {
    if (mOwning && mHandle) {
        static_cast<Shape*>(mHandle)->Release();
        mHandle  = nullptr;
        mOwning  = false;
    }
}

// Derived-class helper: store an already-AddRef'd shape (shape->AddRef already called by caller).
#define SHAPE_INIT(shape_ref_c) \
    do { (shape_ref_c)->AddRef(); \
         mHandle = (void*)(shape_ref_c).GetPtr(); \
         mOwning = true; } while(0)

// ---------------------------------------------------------------------------
// JoltBoxShape
// ---------------------------------------------------------------------------
JoltBoxShape::JoltBoxShape(double hx, double hy, double hz, float cr) {
    BoxShapeSettings ss(Vec3((float)hx, (float)hy, (float)hz), cr);
    ss.SetEmbedded();
    ShapeRefC s = ss.Create().Get(); SHAPE_INIT(s);
}
JoltBoxShape::JoltBoxShape(double hx, double hy, double hz)
    : JoltBoxShape(hx, hy, hz, 0.05f) {}

// ---------------------------------------------------------------------------
// JoltSphereShape
// ---------------------------------------------------------------------------
JoltSphereShape::JoltSphereShape(float radius) {
    ShapeRefC s = new SphereShape(radius); SHAPE_INIT(s);
}

// ---------------------------------------------------------------------------
// JoltCapsuleShape
// ---------------------------------------------------------------------------
JoltCapsuleShape::JoltCapsuleShape(float halfHeight, float radius) {
    CapsuleShapeSettings ss(halfHeight, radius);
    ss.SetEmbedded();
    ShapeRefC s = ss.Create().Get(); SHAPE_INIT(s);
}

// ---------------------------------------------------------------------------
// JoltCylinderShape
// ---------------------------------------------------------------------------
JoltCylinderShape::JoltCylinderShape(float halfHeight, float radius, float cr) {
    CylinderShapeSettings ss(halfHeight, radius, cr);
    ss.SetEmbedded();
    ShapeRefC s = ss.Create().Get(); SHAPE_INIT(s);
}
JoltCylinderShape::JoltCylinderShape(float halfHeight, float radius)
    : JoltCylinderShape(halfHeight, radius, 0.05f) {}

// ---------------------------------------------------------------------------
// JoltRotatedTranslatedShape
// ---------------------------------------------------------------------------
JoltRotatedTranslatedShape::JoltRotatedTranslatedShape(
    JoltShape* inner,
    double posX, double posY, double posZ,
    float qx, float qy, float qz, float qw)
{
    const Shape* innerPtr = static_cast<const Shape*>(inner->getHandle());
    RotatedTranslatedShapeSettings ss(
        Vec3((float)posX, (float)posY, (float)posZ),
        Quat(qx, qy, qz, qw),
        innerPtr);
    ss.SetEmbedded();
    ShapeRefC s = ss.Create().Get(); SHAPE_INIT(s);
}

#undef SHAPE_INIT

// ---------------------------------------------------------------------------
// JoltBodyCreationSettings
// ---------------------------------------------------------------------------
struct BCSHandle {
    BodyCreationSettings settings;
};

JoltBodyCreationSettings::JoltBodyCreationSettings(
    JoltShape* shape,
    double posX, double posY, double posZ,
    float qx, float qy, float qz, float qw,
    int motionType,
    unsigned int objectLayer)
{
    auto* h = new BCSHandle();
    EMotionType mt = static_cast<EMotionType>((int)motionType);
    h->settings = BodyCreationSettings(
        static_cast<Shape*>(shape->getHandle()),
        RVec3(posX, posY, posZ),
        Quat(qx, qy, qz, qw),
        mt,
        (ObjectLayer)objectLayer);
    mHandle = h;
}

JoltBodyCreationSettings::JoltBodyCreationSettings(
    JoltShape* shape,
    double posX, double posY, double posZ,
    int motionType,
    unsigned int objectLayer)
    : JoltBodyCreationSettings(shape, posX, posY, posZ,
                                0, 0, 0, 1,
                                motionType, objectLayer) {}

JoltBodyCreationSettings::~JoltBodyCreationSettings() {
    delete static_cast<BCSHandle*>(mHandle);
}

static BodyCreationSettings& bcs(JoltBodyCreationSettings* s) {
    return static_cast<BCSHandle*>(s->mHandle)->settings;
}

void JoltBodyCreationSettings::SetPosition(double x, double y, double z) {
    bcs(this).mPosition = RVec3(x, y, z);
}
void JoltBodyCreationSettings::SetRotation(float qx, float qy, float qz, float qw) {
    bcs(this).mRotation = Quat(qx, qy, qz, qw);
}
void JoltBodyCreationSettings::SetLinearVelocity(float vx, float vy, float vz) {
    bcs(this).mLinearVelocity = Vec3(vx, vy, vz);
}
void JoltBodyCreationSettings::SetAngularVelocity(float vx, float vy, float vz) {
    bcs(this).mAngularVelocity = Vec3(vx, vy, vz);
}
void JoltBodyCreationSettings::SetFriction(float f) {
    bcs(this).mFriction = f;
}
void JoltBodyCreationSettings::SetRestitution(float r) {
    bcs(this).mRestitution = r;
}
void JoltBodyCreationSettings::SetGravityFactor(float f) {
    bcs(this).mGravityFactor = f;
}
void JoltBodyCreationSettings::SetIsSensor(bool s) {
    bcs(this).mIsSensor = s;
}
void JoltBodyCreationSettings::SetObjectLayer(unsigned int layer) {
    bcs(this).mObjectLayer = (ObjectLayer)layer;
}

// ---------------------------------------------------------------------------
// JoltPhysicsSystem::Impl
// ---------------------------------------------------------------------------
struct JoltPhysicsSystem::Impl {
    DefaultBPLayer     bpLayer;
    DefaultObjVsBP     objVsBP;
    DefaultObjVsObj    objVsObj;
    TempAllocatorImpl  tempAlloc;
    JobSystemThreadPool jobSystem;
    PhysicsSystem      physics;

    // Constraint registry (pointer -> sequential ID)
    std::unordered_map<uint32_t, Ref<Constraint>> constraints;
    uint32_t nextConstraintID = 1;

    Impl(uint32_t maxBodies, uint32_t maxBodyPairs, uint32_t maxContacts)
        : tempAlloc(64 * 1024 * 1024)
        , jobSystem(cMaxPhysicsJobs, cMaxPhysicsBarriers,
                    std::max(1, (int)std::thread::hardware_concurrency() - 1))
    {
        physics.Init(maxBodies, 0, maxBodyPairs, maxContacts,
                     bpLayer, objVsBP, objVsObj);
    }

    BodyInterface& bi()       { return physics.GetBodyInterface(); }
    const BodyInterface& bi() const { return physics.GetBodyInterface(); }

    JoltBodyID toJoltID(BodyID id) {
        JoltBodyID r; r.value = id.GetIndexAndSequenceNumber(); return r;
    }
    BodyID fromJoltID(JoltBodyID id) const { return BodyID(id.value); }

    JoltConstraintID addConstraint(Constraint* c) {
        uint32_t id = nextConstraintID++;
        physics.AddConstraint(c);
        constraints[id] = c;
        JoltConstraintID r; r.value = id; return r;
    }
    Constraint* getConstraint(JoltConstraintID id) {
        auto it = constraints.find(id.value);
        return it != constraints.end() ? it->second.GetPtr() : nullptr;
    }
    void removeConstraint(JoltConstraintID id) {
        auto it = constraints.find(id.value);
        if (it != constraints.end()) {
            physics.RemoveConstraint(it->second);
            constraints.erase(it);
        }
    }
};

// ---------------------------------------------------------------------------
// JoltPhysicsSystem
// ---------------------------------------------------------------------------
JoltPhysicsSystem::JoltPhysicsSystem()
    : JoltPhysicsSystem(65536, 65536, 65536) {}

JoltPhysicsSystem::JoltPhysicsSystem(
    unsigned int maxBodies,
    unsigned int maxBodyPairs,
    unsigned int maxContactConstraints)
{
    jolt_lib_acquire();
    mImpl = new Impl(maxBodies, maxBodyPairs, maxContactConstraints);
}

JoltPhysicsSystem::~JoltPhysicsSystem() {
    delete mImpl;
    jolt_lib_release();
}

void JoltPhysicsSystem::SetGravity(double x, double y, double z) {
    mImpl->physics.SetGravity(Vec3((float)x, (float)y, (float)z));
}
JoltVec3 JoltPhysicsSystem::GetGravity() const {
    return fromRVec3(RVec3(mImpl->physics.GetGravity()));
}
void JoltPhysicsSystem::Update(float deltaTime, int collisionSteps) {
    mImpl->physics.Update(deltaTime, collisionSteps,
                          &mImpl->tempAlloc, &mImpl->jobSystem);
}
void JoltPhysicsSystem::OptimizeBroadPhase() {
    mImpl->physics.OptimizeBroadPhase();
}

JoltBodyID JoltPhysicsSystem::CreateAndAddBody(
    JoltBodyCreationSettings* settings, int activation)
{
    EActivation a = (activation == JoltActivation_Activate)
                  ? EActivation::Activate : EActivation::DontActivate;
    BodyID id = mImpl->bi().CreateAndAddBody(bcs(settings), a);
    return mImpl->toJoltID(id);
}

void JoltPhysicsSystem::RemoveBody(JoltBodyID id) {
    mImpl->bi().RemoveBody(mImpl->fromJoltID(id));
}
void JoltPhysicsSystem::DestroyBody(JoltBodyID id) {
    mImpl->bi().DestroyBody(mImpl->fromJoltID(id));
}
void JoltPhysicsSystem::RemoveAndDestroyBody(JoltBodyID id) {
    mImpl->bi().RemoveBody(mImpl->fromJoltID(id));
    mImpl->bi().DestroyBody(mImpl->fromJoltID(id));
}

JoltVec3 JoltPhysicsSystem::GetBodyPosition(JoltBodyID id) const {
    return fromRVec3(mImpl->bi().GetCenterOfMassPosition(mImpl->fromJoltID(id)));
}
JoltQuat JoltPhysicsSystem::GetBodyRotation(JoltBodyID id) const {
    return fromQuat(mImpl->bi().GetRotation(mImpl->fromJoltID(id)));
}
JoltVec3f JoltPhysicsSystem::GetBodyLinearVelocity(JoltBodyID id) const {
    return fromVec3(mImpl->bi().GetLinearVelocity(mImpl->fromJoltID(id)));
}
JoltVec3f JoltPhysicsSystem::GetBodyAngularVelocity(JoltBodyID id) const {
    return fromVec3(mImpl->bi().GetAngularVelocity(mImpl->fromJoltID(id)));
}
bool JoltPhysicsSystem::IsBodyActive(JoltBodyID id) const {
    return mImpl->bi().IsActive(mImpl->fromJoltID(id));
}

void JoltPhysicsSystem::SetBodyPosition(
    JoltBodyID id, double x, double y, double z, int activation)
{
    EActivation a = (activation == JoltActivation_Activate)
                  ? EActivation::Activate : EActivation::DontActivate;
    mImpl->bi().SetPosition(mImpl->fromJoltID(id), RVec3(x,y,z), a);
}
void JoltPhysicsSystem::SetBodyRotation(
    JoltBodyID id, float qx, float qy, float qz, float qw, int activation)
{
    EActivation a = (activation == JoltActivation_Activate)
                  ? EActivation::Activate : EActivation::DontActivate;
    mImpl->bi().SetRotation(mImpl->fromJoltID(id), Quat(qx,qy,qz,qw), a);
}
void JoltPhysicsSystem::SetBodyPositionAndRotation(
    JoltBodyID id,
    double x, double y, double z,
    float qx, float qy, float qz, float qw,
    int activation)
{
    EActivation a = (activation == JoltActivation_Activate)
                  ? EActivation::Activate : EActivation::DontActivate;
    mImpl->bi().SetPositionAndRotation(
        mImpl->fromJoltID(id), RVec3(x,y,z), Quat(qx,qy,qz,qw), a);
}
void JoltPhysicsSystem::SetBodyLinearVelocity(JoltBodyID id, float vx, float vy, float vz) {
    mImpl->bi().SetLinearVelocity(mImpl->fromJoltID(id), Vec3(vx,vy,vz));
}
void JoltPhysicsSystem::SetBodyAngularVelocity(JoltBodyID id, float vx, float vy, float vz) {
    mImpl->bi().SetAngularVelocity(mImpl->fromJoltID(id), Vec3(vx,vy,vz));
}
void JoltPhysicsSystem::SetBodyLinearAndAngularVelocity(
    JoltBodyID id,
    float lvx, float lvy, float lvz,
    float avx, float avy, float avz)
{
    mImpl->bi().SetLinearAndAngularVelocity(
        mImpl->fromJoltID(id), Vec3(lvx,lvy,lvz), Vec3(avx,avy,avz));
}
void JoltPhysicsSystem::AddForce(JoltBodyID id, float fx, float fy, float fz) {
    mImpl->bi().AddForce(mImpl->fromJoltID(id), Vec3(fx,fy,fz));
}
void JoltPhysicsSystem::AddForceAtPosition(
    JoltBodyID id, float fx, float fy, float fz, double px, double py, double pz)
{
    mImpl->bi().AddForce(mImpl->fromJoltID(id), Vec3(fx,fy,fz), RVec3(px,py,pz));
}
void JoltPhysicsSystem::AddTorque(JoltBodyID id, float tx, float ty, float tz) {
    mImpl->bi().AddTorque(mImpl->fromJoltID(id), Vec3(tx,ty,tz));
}
void JoltPhysicsSystem::AddImpulse(JoltBodyID id, float ix, float iy, float iz) {
    mImpl->bi().AddImpulse(mImpl->fromJoltID(id), Vec3(ix,iy,iz));
}
void JoltPhysicsSystem::AddAngularImpulse(JoltBodyID id, float ix, float iy, float iz) {
    mImpl->bi().AddAngularImpulse(mImpl->fromJoltID(id), Vec3(ix,iy,iz));
}
void JoltPhysicsSystem::ActivateBody(JoltBodyID id) {
    mImpl->bi().ActivateBody(mImpl->fromJoltID(id));
}
void JoltPhysicsSystem::DeactivateBody(JoltBodyID id) {
    mImpl->bi().DeactivateBody(mImpl->fromJoltID(id));
}
void JoltPhysicsSystem::SetFriction(JoltBodyID id, float f) {
    mImpl->bi().SetFriction(mImpl->fromJoltID(id), f);
}
float JoltPhysicsSystem::GetFriction(JoltBodyID id) const {
    return mImpl->bi().GetFriction(mImpl->fromJoltID(id));
}
void JoltPhysicsSystem::SetRestitution(JoltBodyID id, float r) {
    mImpl->bi().SetRestitution(mImpl->fromJoltID(id), r);
}
float JoltPhysicsSystem::GetRestitution(JoltBodyID id) const {
    return mImpl->bi().GetRestitution(mImpl->fromJoltID(id));
}
void JoltPhysicsSystem::SetGravityFactor(JoltBodyID id, float f) {
    mImpl->bi().SetGravityFactor(mImpl->fromJoltID(id), f);
}
float JoltPhysicsSystem::GetGravityFactor(JoltBodyID id) const {
    return mImpl->bi().GetGravityFactor(mImpl->fromJoltID(id));
}
unsigned int JoltPhysicsSystem::GetNumBodies() const {
    return mImpl->physics.GetNumBodies();
}
unsigned int JoltPhysicsSystem::GetNumActiveBodies() const {
    return mImpl->physics.GetNumActiveBodies(EBodyType::RigidBody);
}

// ---- Constraints ----

JoltConstraintID JoltPhysicsSystem::AddFixedConstraint(JoltBodyID b1, JoltBodyID b2) {
    FixedConstraintSettings s;
    s.mAutoDetectPoint = true;
    auto* c = static_cast<Constraint*>(
        s.Create(*mImpl->physics.GetBodyLockInterface().TryGetBody(mImpl->fromJoltID(b1)),
                 *mImpl->physics.GetBodyLockInterface().TryGetBody(mImpl->fromJoltID(b2))));

    // Use BodyInterface lock to get bodies properly
    BodyLockWrite lock1(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b1));
    BodyLockWrite lock2(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b2));
    if (!lock1.Succeeded() || !lock2.Succeeded()) { JoltConstraintID r; return r; }
    Ref<Constraint> constraint = s.Create(lock1.GetBody(), lock2.GetBody());
    return mImpl->addConstraint(constraint);
}

JoltConstraintID JoltPhysicsSystem::AddDistanceConstraint(
    JoltBodyID b1, JoltBodyID b2, float minDist, float maxDist)
{
    DistanceConstraintSettings s;
    s.mMinDistance = minDist;
    s.mMaxDistance = maxDist;
    BodyLockWrite lock1(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b1));
    BodyLockWrite lock2(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b2));
    if (!lock1.Succeeded() || !lock2.Succeeded()) { JoltConstraintID r; return r; }
    s.mPoint1 = lock1.GetBody().GetCenterOfMassPosition();
    s.mPoint2 = lock2.GetBody().GetCenterOfMassPosition();
    s.mSpace  = EConstraintSpace::WorldSpace;
    return mImpl->addConstraint(s.Create(lock1.GetBody(), lock2.GetBody()));
}

JoltConstraintID JoltPhysicsSystem::AddPointConstraint(
    JoltBodyID b1, JoltBodyID b2, double px, double py, double pz)
{
    PointConstraintSettings s;
    s.mSpace  = EConstraintSpace::WorldSpace;
    s.mPoint1 = RVec3(px, py, pz);
    s.mPoint2 = RVec3(px, py, pz);
    BodyLockWrite lock1(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b1));
    BodyLockWrite lock2(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b2));
    if (!lock1.Succeeded() || !lock2.Succeeded()) { JoltConstraintID r; return r; }
    return mImpl->addConstraint(s.Create(lock1.GetBody(), lock2.GetBody()));
}

JoltConstraintID JoltPhysicsSystem::AddHingeConstraint(
    JoltBodyID b1, JoltBodyID b2,
    double px, double py, double pz,
    float ax, float ay, float az)
{
    HingeConstraintSettings s;
    s.mSpace       = EConstraintSpace::WorldSpace;
    s.mPoint1      = RVec3(px, py, pz);
    s.mPoint2      = RVec3(px, py, pz);
    s.mHingeAxis1  = Vec3(ax, ay, az).Normalized();
    s.mHingeAxis2  = s.mHingeAxis1;
    s.mNormalAxis1 = s.mHingeAxis1.GetNormalizedPerpendicular();
    s.mNormalAxis2 = s.mNormalAxis1;
    BodyLockWrite lock1(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b1));
    BodyLockWrite lock2(mImpl->physics.GetBodyLockInterface(), mImpl->fromJoltID(b2));
    if (!lock1.Succeeded() || !lock2.Succeeded()) { JoltConstraintID r; return r; }
    return mImpl->addConstraint(s.Create(lock1.GetBody(), lock2.GetBody()));
}

void JoltPhysicsSystem::DestroyConstraint(JoltConstraintID id) {
    mImpl->removeConstraint(id);
}
void JoltPhysicsSystem::SetConstraintEnabled(JoltConstraintID id, bool enabled) {
    if (Constraint* c = mImpl->getConstraint(id)) c->SetEnabled(enabled);
}

// ---------------------------------------------------------------------------
// JoltWorld (backward compat)
// ---------------------------------------------------------------------------
struct JoltWorld::Impl {
    JoltPhysicsSystem sys;
    Impl() { sys.SetGravity(0, -9.81, 0); }
};

JoltWorld::JoltWorld()  { mImpl = new Impl(); }
JoltWorld::~JoltWorld() { delete mImpl; }

void JoltWorld::SetGravity(double x, double y, double z) {
    mImpl->sys.SetGravity(x, y, z);
}

JoltBodyID JoltWorld::AddStaticBox(double hx, double hy, double hz,
                                    double px, double py, double pz)
{
    JoltBoxShape shape(hx, hy, hz);
    JoltBodyCreationSettings s(&shape, px, py, pz,
        JoltMotionType_Static, (unsigned int)(unsigned int)JoltObjectLayer_NonMoving);
    return mImpl->sys.CreateAndAddBody(&s, JoltActivation_DontActivate);
}

JoltBodyID JoltWorld::AddDynamicSphere(double radius, double px, double py, double pz) {
    JoltSphereShape shape((float)radius);
    JoltBodyCreationSettings s(&shape, px, py, pz,
        JoltMotionType_Dynamic, (unsigned int)(unsigned int)JoltObjectLayer_Moving);
    return mImpl->sys.CreateAndAddBody(&s, JoltActivation_Activate);
}

void JoltWorld::Update(double deltaTime) { mImpl->sys.Update((float)deltaTime, 1); }
void JoltWorld::OptimizeBroadPhase()     { mImpl->sys.OptimizeBroadPhase(); }

JoltVec3 JoltWorld::GetBodyPosition(JoltBodyID id) const {
    return mImpl->sys.GetBodyPosition(id);
}
bool JoltWorld::IsBodyActive(JoltBodyID id) const {
    return mImpl->sys.IsBodyActive(id);
}
void JoltWorld::RemoveBody(JoltBodyID id)  { mImpl->sys.RemoveBody(id); }
void JoltWorld::DestroyBody(JoltBodyID id) { mImpl->sys.DestroyBody(id); }
