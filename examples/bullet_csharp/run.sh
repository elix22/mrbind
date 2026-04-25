#!/bin/bash

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR/.."

. _detail/set_env_vars.sh

./bullet_csharp/clean.sh

mkdir -p bullet_csharp/c_library/tmp

BULLET_SRC="$(pwd)/../deps/bullet/src"

# Clang-style flags for the parser.
EXTRA_PARSER_CXX_FLAGS=(
    -std=c++17 -Wall -Wextra
    -fparse-all-comments
    -DBT_USE_DOUBLE_PRECISION
    -I"$BULLET_SRC"
)

# Optional tunable flags for the parser.
EXTRA_PARSER_FLAGS=(
    --copy-inherited-members
)

# Optional tunable flags for the C generator.
EXTRA_GEN_C_FLAGS=(
    --max-header-name-length 100
    --no-handle-exceptions
)

# Optional tunable flags for the C# generator.
EXTRA_GEN_FLAGS=(
    --csharp-version=12
    --dotnet-version=std2.0
)

# Optional tunable flags for the C bindings compilation.
EXTRA_CXX_FLAGS=(
    -g
)

DOTNET=dotnet
SHARED_LIBRARY_EXT=.so
SHARED_LIBRARY_PREFIX=lib

# Need extra flags on MSYS2.
if [[ $(uname -o) == Msys ]]; then
    EXTRA_PARSER_CXX_FLAGS+=(--sysroot="$MSYSTEM_PREFIX")
    DOTNET="C:/Program Files/dotnet/dotnet"
    SHARED_LIBRARY_EXT=.dll
    SHARED_LIBRARY_PREFIX=
fi

# Need the SDK sysroot on macOS.
if [[ $(uname) == Darwin ]]; then
    EXTRA_PARSER_CXX_FLAGS+=(-isysroot "$(xcrun --show-sdk-path)")
    SHARED_LIBRARY_EXT=.dylib
    SHARED_LIBRARY_PREFIX=lib
fi

set -x

# Assemble the combined input header.
echo "#pragma once" >bullet_csharp/c_library/tmp/combined_input.h
echo "#include <btBulletDynamicsCommon.h>" >>bullet_csharp/c_library/tmp/combined_input.h

# Parse the input header.
../build/mrbind \
    bullet_csharp/c_library/tmp/combined_input.h \
    -o bullet_csharp/c_library/tmp/parse_result.json \
    --ignore :: \
    --skip-mentions-of btDbvt \
    --skip-mentions-of btDbvtProxy \
    --skip-mentions-of btDbvtNode \
    --skip-mentions-of btDbvtAabbMm \
    --skip-mentions-of btDbvtVolume \
    --skip-mentions-of btOverlappingPairCallback \
    --skip-mentions-of btBroadphasePair \
    --skip-mentions-of btBroadphasePairArray \
    --skip-mentions-of btBroadphaseProxy \
    --skip-mentions-of btPoolAllocator \
    --skip-mentions-of btDispatcherInfo \
    --skip-mentions-of btDispatcherQueryType \
    --skip-mentions-of btSimulationIslandManager \
    --skip-mentions-of btConstraintSolverPoolMt \
    --skip-mentions-of btSolverAnalyticsData \
    --skip-mentions-of btSolverBody \
    --skip-mentions-of btSingleConstraintRowSolver \
    --skip-mentions-of btBatchedConstraints \
    --skip-mentions-of btConstraintArray \
    --skip-mentions-of btContactSolverInfo \
    --skip-mentions-of btJacobianEntry \
    --skip-mentions-of btPersistentManifold \
    --skip-mentions-of btManifoldArray \
    --skip-mentions-of btNearCallback \
    --skip-mentions-of btInternalTickCallback \
    --skip-mentions-of btUnionFind \
    --skip-mentions-of btNodeStack \
    --skip-mentions-of btSSEUnion \
    --skip-mentions-of btSpinMutex \
    --skip-mentions-of btClock \
    --skip-mentions-of btQuantizedBvh \
    --skip-mentions-of btVertexArray \
    --skip-mentions-of btSimplePairArray \
    --skip-mentions-of btAngularLimit \
    --skip-mentions-of btConvexSeparatingDistanceUtil \
    --skip-mentions-of btConvexTriangleCallback \
    --skip-mentions-of btVector3DoubleData \
    --skip-mentions-of btVector3FloatData \
    --skip-mentions-of btQuaternionDoubleData \
    --skip-mentions-of btQuaternionFloatData \
    --skip-mentions-of btTransformDoubleData \
    --skip-mentions-of btTransformFloatData \
    --skip-mentions-of btMatrix3x3DoubleData \
    --skip-mentions-of btMatrix3x3FloatData \
    --skip-mentions-of btRigidBodyDoubleData \
    --skip-mentions-of btRigidBodyFloatData \
    --skip-mentions-of btCollisionObjectDoubleData \
    --skip-mentions-of btCollisionObjectFloatData \
    --skip-mentions-of btDynamicsWorldDoubleData \
    --skip-mentions-of btDynamicsWorldFloatData \
    --allow btVector3 \
    --allow btQuadWord \
    --allow btQuaternion \
    --allow btTransform \
    --allow btMatrix3x3 \
    --allow btScalar \
    --allow btRigidBody \
    --allow btCollisionObject \
    --allow btDiscreteDynamicsWorld \
    --allow btDynamicsWorld \
    --allow btCollisionWorld \
    --allow btCollisionShape \
    --allow btConvexShape \
    --allow btConvexInternalShape \
    --allow btPolyhedralConvexShape \
    --allow btPolyhedralConvexAabbCachingShape \
    --allow btConvexInternalAabbCachingShape \
    --allow btBoxShape \
    --allow btSphereShape \
    --allow btCapsuleShape \
    --allow btMotionState \
    --allow btDefaultMotionState \
    --allow btCollisionConfiguration \
    --allow btDefaultCollisionConfiguration \
    --allow btDispatcher \
    --allow btCollisionDispatcher \
    --allow btBroadphaseInterface \
    --allow btDbvtBroadphase \
    --allow btOverlappingPairCache \
    --allow btConstraintSolver \
    --allow btSequentialImpulseConstraintSolver \
    "${EXTRA_PARSER_FLAGS[@]+"${EXTRA_PARSER_FLAGS[@]}"}" \
    -- \
    -xc++-header \
    -resource-dir="$("$CLANG_CXX" -print-resource-dir)" \
    -I"$BULLET_SRC" \
    "${EXTRA_PARSER_CXX_FLAGS[@]}"

# Generate the C bindings (required by the C# bindings).
../build/mrbind_gen_c \
    --input bullet_csharp/c_library/tmp/parse_result.json \
    --output-header-dir bullet_csharp/c_library/include \
    --output-source-dir bullet_csharp/c_library/src \
    --output-desc-json bullet_csharp/c_library/tmp/c_desc.json \
    --helper-name-prefix Bullet_ \
    --helper-macro-name-prefix BULLET_ \
    --map-path "$BULLET_SRC" bullet \
    --assume-include-dir "$BULLET_SRC" \
    --force-emit-common-helpers \
    "${EXTRA_GEN_C_FLAGS[@]}"

# Generate the C# bindings.
../build/mrbind_gen_csharp \
    --input-json bullet_csharp/c_library/tmp/c_desc.json \
    --output-dir bullet_csharp/library/src \
    --imported-lib-name bullet_bindings \
    --helpers-namespace Bullet \
    --force-namespace Bullet \
    "${EXTRA_GEN_FLAGS[@]}"

# Build the C# library.
"$DOTNET" build bullet_csharp/library

# Find the generated C++ source files for the C bindings.
SOURCES="$(find bullet_csharp/c_library/src -name '*.cpp')"

# Compile the C bindings into a shared library.
if [[ $SOURCES ]]; then
    "$CLANG_CXX" \
        -std=c++17 -Wall -Wextra \
        -I"$BULLET_SRC" \
        -Ibullet_csharp/c_library/include \
        -Ibullet_csharp/c_library/src \
        -DBT_USE_DOUBLE_PRECISION \
        $SOURCES \
        "$BULLET_SRC/btLinearMathAll.cpp" \
        "$BULLET_SRC/btBulletCollisionAll.cpp" \
        "$BULLET_SRC/btBulletDynamicsAll.cpp" \
        -shared -fPIC \
        -fvisibility=hidden -fvisibility-inlines-hidden \
        -o bullet_csharp/c_library/${SHARED_LIBRARY_PREFIX}bullet_bindings$SHARED_LIBRARY_EXT \
        "${EXTRA_CXX_FLAGS[@]}"
else
    echo "The generator didn't produce any source files that need to be compiled."
fi

# Run the test executable.
if [[ $(uname -o) == Msys ]]; then
    PATH="bullet_csharp/c_library:$PATH" "$DOTNET" run --project bullet_csharp/example_consumer
elif [[ $(uname) == Darwin ]]; then
    DYLD_LIBRARY_PATH=bullet_csharp/c_library "$DOTNET" run --project bullet_csharp/example_consumer
else
    LD_LIBRARY_PATH=bullet_csharp/c_library "$DOTNET" run --project bullet_csharp/example_consumer
fi
