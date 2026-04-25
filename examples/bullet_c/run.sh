#!/bin/bash

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR/.."

. _detail/set_env_vars.sh

./bullet_c/clean.sh

mkdir -p bullet_c/output/tmp

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
EXTRA_GEN_FLAGS=(
    --max-header-name-length 100
    --no-handle-exceptions
)

# Optional tunable flags for the library compilation.
EXTRA_CXX_FLAGS=(
    -g
)

# Optional tunable flags for the example program compilation.
EXTRA_C_FLAGS=(
    -g
)

SHARED_LIBRARY_EXT=.so

# Need extra flags on MSYS2.
if [[ $(uname -o) == Msys ]]; then
    EXTRA_PARSER_CXX_FLAGS+=(--sysroot="$MSYSTEM_PREFIX")
    SHARED_LIBRARY_EXT=.dll
fi

# Need the SDK sysroot on macOS.
if [[ $(uname) == Darwin ]]; then
    EXTRA_PARSER_CXX_FLAGS+=(-isysroot "$(xcrun --show-sdk-path)")
    SHARED_LIBRARY_EXT=.dylib
fi

set -x

# Assemble the combined input header.
echo "#pragma once" >bullet_c/output/tmp/combined_input.h
echo "#include <btBulletDynamicsCommon.h>" >>bullet_c/output/tmp/combined_input.h

# Parse the input header.
../build/mrbind \
    bullet_c/output/tmp/combined_input.h \
    -o bullet_c/output/tmp/parse_result.json \
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

# Generate the C bindings.
../build/mrbind_gen_c \
    --input bullet_c/output/tmp/parse_result.json \
    --output-header-dir bullet_c/output/include \
    --output-source-dir bullet_c/output/src \
    --helper-name-prefix Bullet_ \
    --helper-macro-name-prefix BULLET_ \
    --map-path "$BULLET_SRC" bullet \
    --assume-include-dir "$BULLET_SRC" \
    "${EXTRA_GEN_FLAGS[@]}"

# Find the generated source files.
SOURCES="$(find bullet_c/output/src -name '*.cpp')"

# Compile the generated sources into a shared library.
if [[ $SOURCES ]]; then
    "$CLANG_CXX" \
        -std=c++17 -Wall -Wextra \
        -I"$BULLET_SRC" \
        -Ibullet_c/output/include \
        -Ibullet_c/output/src \
        -DBT_USE_DOUBLE_PRECISION \
        $SOURCES \
        "$BULLET_SRC/btLinearMathAll.cpp" \
        "$BULLET_SRC/btBulletCollisionAll.cpp" \
        "$BULLET_SRC/btBulletDynamicsAll.cpp" \
        -shared -fPIC \
        -fvisibility=hidden -fvisibility-inlines-hidden \
        -o bullet_c/output/libbullet_bindings$SHARED_LIBRARY_EXT \
        "${EXTRA_CXX_FLAGS[@]}"
else
    echo "The generator didn't produce any source files that need to be compiled."
fi

# Compile the test executable.
cc \
    -std=c11 -Wall -Wextra \
    bullet_c/example_consumer.c \
    -Ibullet_c/output/include \
    -Lbullet_c/output -lbullet_bindings \
    -o bullet_c/output/example_consumer \
    "${EXTRA_C_FLAGS[@]}"

# Run the test executable.
if [[ $(uname) == Darwin ]]; then
    DYLD_LIBRARY_PATH=bullet_c/output bullet_c/output/example_consumer
else
    LD_LIBRARY_PATH=bullet_c/output bullet_c/output/example_consumer
fi
