#!/bin/bash

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR/.."

. _detail/set_env_vars.sh

./jolt_csharp/clean.sh

JOLT_ROOT="$(cd "$(pwd)/../deps/JoltPhysics" && pwd)"
INIT_DIR="$(pwd)/jolt_c"   # jolt_init_wrapper.h / jolt_init.cpp live in jolt_c/
LIB_DIR="jolt_csharp/c_library"

mkdir -p "$LIB_DIR/tmp" "$LIB_DIR/include" "$LIB_DIR/src"

EXTRA_PARSER_CXX_FLAGS=(
    -std=c++17 -Wall -Wextra
    -fparse-all-comments
    -DNDEBUG
    -I"$INIT_DIR"
)

EXTRA_PARSER_FLAGS=(
    --copy-inherited-members
)

EXTRA_GEN_C_FLAGS=(
    --max-header-name-length 100
    --no-handle-exceptions
    --expose-as-struct JPH::BodyID
    --no-dynamic-cast
)

EXTRA_GEN_FLAGS=(
    --csharp-version=12
    --dotnet-version=std2.0
)

EXTRA_CXX_FLAGS=(
    -g
    -DNDEBUG
)
# NOTE: -DNDEBUG matches the parser flags and disables Jolt's debug assertions.

DOTNET=dotnet
SHARED_LIBRARY_EXT=.so
SHARED_LIBRARY_PREFIX=lib

if [[ $(uname -o 2>/dev/null) == Msys ]]; then
    EXTRA_PARSER_CXX_FLAGS+=(--sysroot="$MSYSTEM_PREFIX")
    DOTNET="C:/Program Files/dotnet/dotnet"
    SHARED_LIBRARY_EXT=.dll
    SHARED_LIBRARY_PREFIX=
fi

PARSER_RESOURCE_DIR=""
if [[ $(uname) == Darwin ]]; then
    EXTRA_PARSER_CXX_FLAGS+=(-isysroot "$(xcrun --show-sdk-path)" -fno-blocks)
    SHARED_LIBRARY_EXT=.dylib
    SHARED_LIBRARY_PREFIX=lib
    if [[ -x /opt/homebrew/opt/llvm/bin/clang ]]; then
        PARSER_RESOURCE_DIR="$(/opt/homebrew/opt/llvm/bin/clang -print-resource-dir)"
    fi
fi

set -x

# Assemble combined input header (raw Jolt headers + JoltHelpers wrapper).
{
    echo "#pragma once"
    echo "#include \"$JOLT_ROOT/Jolt/Jolt.h\""
    echo "#include \"$JOLT_ROOT/Jolt/RegisterTypes.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Core/Factory.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Core/JobSystemThreadPool.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/PhysicsSettings.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/PhysicsSystem.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/BoxShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/SphereShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/CapsuleShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/CylinderShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/TaperedCylinderShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/TriangleShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/PlaneShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/EmptyShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/ScaledShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/StaticCompoundShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/MutableCompoundShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/MeshShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/ConvexHullShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/HeightFieldShape.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Body/BodyCreationSettings.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Body/Body.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/PhysicsMaterial.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/CollisionGroup.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Constraints/TwoBodyConstraint.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Constraints/FixedConstraint.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Constraints/DistanceConstraint.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Constraints/PointConstraint.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Constraints/HingeConstraint.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/SoftBody/SoftBodySharedSettings.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/SoftBody/SoftBodyCreationSettings.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Geometry/AABox.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Core/TempAllocator.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Body/BodyInterface.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Body/BodyActivationListener.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/SubShapeID.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/CastResult.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/Shape/SubShapeIDPair.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Body/BodyFilter.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/ShapeFilter.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/ContactListener.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/NarrowPhaseQuery.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/ObjectLayerPairFilterTable.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Character/CharacterBase.h\""
    echo "#include \"$JOLT_ROOT/Jolt/Physics/Character/CharacterVirtual.h\""
    echo "#include \"$INIT_DIR/jolt_init_wrapper.h\""
} >"$LIB_DIR/tmp/combined_input.h"

# Parse the input header.
../build/mrbind \
    "$LIB_DIR/tmp/combined_input.h" \
    -o "$LIB_DIR/tmp/parse_result.json" \
    --ignore :: \
    --skip-mentions-of std::align_val_t \
    --allow JPH::RefTarget \
    --allow JPH::NonCopyable \
    --allow JPH::SerializableObject \
    --allow JPH::EMotionType \
    --allow JPH::EPhysicsUpdateError \
    --allow JPH::ShapeSettings \
    --allow JPH::ConvexShapeSettings \
    --allow JPH::DecoratedShapeSettings \
    --allow JPH::CompoundShapeSettings \
    --allow JPH::BoxShapeSettings \
    --allow JPH::SphereShapeSettings \
    --allow JPH::CapsuleShapeSettings \
    --allow JPH::CylinderShapeSettings \
    --allow JPH::TriangleShapeSettings \
    --allow JPH::TaperedCapsuleShapeSettings \
    --allow JPH::TaperedCylinderShapeSettings \
    --allow JPH::ConvexHullShapeSettings \
    --allow JPH::RotatedTranslatedShapeSettings \
    --allow JPH::ScaledShapeSettings \
    --allow JPH::OffsetCenterOfMassShapeSettings \
    --allow JPH::StaticCompoundShapeSettings \
    --allow JPH::MutableCompoundShapeSettings \
    --allow JPH::MeshShapeSettings \
    --allow JPH::EmptyShapeSettings \
    --allow JPH::PlaneShapeSettings \
    --allow JPH::HeightFieldShapeSettings \
    --allow JPH::Shape \
    --allow JPH::ConvexShape \
    --allow JPH::DecoratedShape \
    --allow JPH::CompoundShape \
    --allow JPH::BoxShape \
    --allow JPH::SphereShape \
    --allow JPH::CapsuleShape \
    --allow JPH::CylinderShape \
    --allow JPH::TriangleShape \
    --allow JPH::TaperedCapsuleShape \
    --allow JPH::TaperedCylinderShape \
    --allow JPH::ConvexHullShape \
    --allow JPH::RotatedTranslatedShape \
    --allow JPH::ScaledShape \
    --allow JPH::OffsetCenterOfMassShape \
    --allow JPH::StaticCompoundShape \
    --allow JPH::MutableCompoundShape \
    --allow JPH::MeshShape \
    --allow JPH::EmptyShape \
    --allow JPH::PlaneShape \
    --allow JPH::HeightFieldShape \
    --allow JPH::BodyCreationSettings \
    --allow JPH::Body \
    --allow JPH::PhysicsMaterial \
    --allow JPH::CollisionGroup \
    --allow JPH::ConstraintSettings \
    --allow JPH::TwoBodyConstraintSettings \
    --allow JPH::FixedConstraintSettings \
    --allow JPH::DistanceConstraintSettings \
    --allow JPH::PointConstraintSettings \
    --allow JPH::HingeConstraintSettings \
    --allow JPH::Constraint \
    --allow JPH::TwoBodyConstraint \
    --allow JPH::FixedConstraint \
    --allow JPH::DistanceConstraint \
    --allow JPH::PointConstraint \
    --allow JPH::HingeConstraint \
    --allow JPH::SoftBodySharedSettings \
    --allow JPH::SoftBodyCreationSettings \
    --allow JPH::SubShapeID \
    --allow JPH::AABox \
    --allow JPH::BodyID \
    --allow JPH::EActivation \
    --allow JPH::BroadPhaseLayer \
    --allow JPH::BroadPhaseLayerInterface \
    --allow JPH::BroadPhaseLayerInterfaceTable \
    --allow JPH::ObjectVsBroadPhaseLayerFilter \
    --allow JPH::ObjectLayerPairFilter \
    --allow JPH::ObjectLayerPairFilterTable \
    --allow JPH::ObjectVsBroadPhaseLayerFilterTable \
    --allow JPH::BroadPhaseLayerFilter \
    --allow JPH::DefaultBroadPhaseLayerFilter \
    --allow JPH::SpecifiedBroadPhaseLayerFilter \
    --allow JPH::ObjectLayerFilter \
    --allow JPH::DefaultObjectLayerFilter \
    --allow JPH::SpecifiedObjectLayerFilter \
    --allow JPH::BodyActivationListener \
    --allow JPH::BodyInterface \
    --allow JPH::TempAllocator \
    --allow JPH::TempAllocatorImpl \
    --allow JPH::TempAllocatorMalloc \
    --allow JPH::TempAllocatorImplWithMallocFallback \
    --allow JPH::JobSystem \
    --allow JPH::JobSystemWithBarrier \
    --allow JPH::JobSystemThreadPool \
    --allow JPH::PhysicsSettings \
    --allow JPH::PhysicsSystem \
    --allow JPH::Factory \
    --allow JPH::BroadPhaseCastResult \
    --allow JPH::RayCastResult \
    --allow JPH::SubShapeIDPair \
    --allow JPH::BodyFilter \
    --allow JPH::IgnoreSingleBodyFilter \
    --allow JPH::IgnoreMultipleBodiesFilter \
    --allow JPH::ShapeFilter \
    --allow JPH::ReversedShapeFilter \
    --allow JPH::ContactManifold \
    --allow JPH::ContactSettings \
    --allow JPH::ContactListener \
    --allow JPH::BroadPhaseQuery \
    --allow JPH::NarrowPhaseQuery \
    --allow JPH::CharacterBaseSettings \
    --allow JPH::CharacterBase \
    --allow JPH::CharacterVirtualSettings \
    --allow JPH::CharacterContactSettings \
    --allow JPH::CharacterContactListener \
    --allow JPH::CharacterVsCharacterCollision \
    --allow JPH::CharacterVsCharacterCollisionSimple \
    --allow JPH::CharacterID \
    --allow JPH::CharacterVirtual \
    --allow JoltHelpers \
    "${EXTRA_PARSER_FLAGS[@]+"${EXTRA_PARSER_FLAGS[@]}"}" \
    -- \
    -xc++-header \
    -resource-dir="${PARSER_RESOURCE_DIR:-$("$CLANG_CXX" -print-resource-dir)}" \
    -I"$INIT_DIR" \
    -I"$JOLT_ROOT" \
    "${EXTRA_PARSER_CXX_FLAGS[@]}"

# Generate the C bindings.
../build/mrbind_gen_c \
    --input "$LIB_DIR/tmp/parse_result.json" \
    --output-header-dir "$LIB_DIR/include" \
    --output-source-dir "$LIB_DIR/src" \
    --output-desc-json "$LIB_DIR/tmp/c_desc.json" \
    --helper-name-prefix Jolt_ \
    --helper-macro-name-prefix JOLT_ \
    --map-path "$(pwd)/jolt_c" jolt \
    --map-path "$(cd "$(pwd)/../deps/JoltPhysics" && pwd)" jolt \
    --assume-include-dir "$(pwd)/jolt_c" \
    --assume-include-dir "$(cd "$(pwd)/../deps/JoltPhysics" && pwd)" \
    --force-emit-common-helpers \
    "${EXTRA_GEN_C_FLAGS[@]}"

# ODR definitions for static const members.
cat >"$LIB_DIR/src/jolt_odr_defs.cpp" <<'EOF'
#include <Jolt/Physics/Collision/CollisionGroup.h>
namespace JPH {
const CollisionGroup::GroupID    CollisionGroup::cInvalidGroup;
const CollisionGroup::SubGroupID CollisionGroup::cInvalidSubGroup;
}
EOF

# Generate the C# bindings.
../build/mrbind_gen_csharp \
    --input-json "$LIB_DIR/tmp/c_desc.json" \
    --output-dir jolt_csharp/library/src \
    --imported-lib-name cjolt \
    --helpers-namespace Jolt \
    --force-namespace Jolt \
    "${EXTRA_GEN_FLAGS[@]}"

# Build the C# library.
"$DOTNET" build jolt_csharp/library

# Compile shared library: Jolt + generated bindings + jolt_init.cpp.
JOLT_SOURCES=$(find "$JOLT_ROOT/Jolt" -name "*.cpp" | sort)

if [[ -z "$JOLT_SOURCES" ]]; then
    echo "No Jolt source files found in $JOLT_ROOT/Jolt"
    exit 1
fi

"$CLANG_CXX" \
    -std=c++17 \
    -I"$JOLT_ROOT" \
    -I"$INIT_DIR" \
    -I"$LIB_DIR/include" \
    -I"$LIB_DIR/src" \
    -include "$JOLT_ROOT/Jolt/Jolt.h" \
    -include "$JOLT_ROOT/Jolt/Core/Atomics.h" \
    $JOLT_SOURCES \
    "$INIT_DIR/jolt_init.cpp" \
    $(find "$LIB_DIR/src" -name "*.cpp") \
    -shared -fPIC \
    -fvisibility=hidden -fvisibility-inlines-hidden \
    -o "$LIB_DIR/${SHARED_LIBRARY_PREFIX}cjolt${SHARED_LIBRARY_EXT}" \
    "${EXTRA_CXX_FLAGS[@]}"

# Run the C# example.
if [[ $(uname -o 2>/dev/null) == Msys ]]; then
    PATH="$LIB_DIR:$PATH" "$DOTNET" run --project jolt_csharp/example_consumer
elif [[ $(uname) == Darwin ]]; then
    DYLD_LIBRARY_PATH="$LIB_DIR" "$DOTNET" run --project jolt_csharp/example_consumer
else
    LD_LIBRARY_PATH="$LIB_DIR" "$DOTNET" run --project jolt_csharp/example_consumer
fi
