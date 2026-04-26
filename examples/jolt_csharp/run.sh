#!/bin/bash

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR/.."

. _detail/set_env_vars.sh

./jolt_csharp/clean.sh

mkdir -p jolt_csharp/c_library/tmp

JOLT_ROOT="$(pwd)/../deps/JoltPhysics"
HELPER_DIR="$(pwd)/jolt_c"   # reuse the same jolt_helper.h / jolt_helper.cpp

# Clang-style flags for the parser.
EXTRA_PARSER_CXX_FLAGS=(
    -std=c++17
    -fparse-all-comments
    -I"$HELPER_DIR"
)

# Optional tunable flags for the parser.
EXTRA_PARSER_FLAGS=(
    --copy-inherited-members
)

# Optional tunable flags for the C generator.
EXTRA_GEN_C_FLAGS=(
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
echo "#pragma once" >jolt_csharp/c_library/tmp/combined_input.h
echo "#include \"$HELPER_DIR/jolt_helper.h\"" >>jolt_csharp/c_library/tmp/combined_input.h

# Parse the input header.
../build/mrbind \
    jolt_csharp/c_library/tmp/combined_input.h \
    -o jolt_csharp/c_library/tmp/parse_result.json \
    --ignore :: \
    --allow JoltVec3f \
    --allow JoltVec3 \
    --allow JoltQuat \
    --allow JoltMat44 \
    --allow JoltRMat44 \
    --allow JoltAABox \
    --allow JoltCollisionGroup \
    --allow JoltPhysicsMaterial \
    --allow JoltTwoBodyConstraint \
    --allow JoltBodyID \
    --allow JoltBodyIDList \
    --allow JoltConstraintID \
    --allow JoltShape \
    --allow JoltBoxShape \
    --allow JoltSphereShape \
    --allow JoltCapsuleShape \
    --allow JoltCylinderShape \
    --allow JoltRotatedTranslatedShape \
    --allow JoltBodyCreationSettings \
    --allow JoltSoftBodySharedSettings \
    --allow JoltSoftBodyCreationSettings \
    --allow JoltPhysicsSystem \
    --allow JoltBodyInterface \
    --allow JoltWorld \
    "${EXTRA_PARSER_FLAGS[@]+"${EXTRA_PARSER_FLAGS[@]}"}" \
    -- \
    -xc++-header \
    -resource-dir="$("$CLANG_CXX" -print-resource-dir)" \
    -I"$HELPER_DIR" \
    "${EXTRA_PARSER_CXX_FLAGS[@]}"

# Generate the C bindings (required by the C# bindings).
../build/mrbind_gen_c \
    --input jolt_csharp/c_library/tmp/parse_result.json \
    --output-header-dir jolt_csharp/c_library/include \
    --output-source-dir jolt_csharp/c_library/src \
    --output-desc-json jolt_csharp/c_library/tmp/c_desc.json \
    --helper-name-prefix Jolt_ \
    --helper-macro-name-prefix JOLT_ \
    --map-path "$HELPER_DIR" jolt \
    --assume-include-dir "$HELPER_DIR" \
    --force-emit-common-helpers \
    "${EXTRA_GEN_C_FLAGS[@]}"

# Generate the C# bindings.
../build/mrbind_gen_csharp \
    --input-json jolt_csharp/c_library/tmp/c_desc.json \
    --output-dir jolt_csharp/library/src \
    --imported-lib-name cjolt \
    --helpers-namespace Jolt \
    --force-namespace Jolt \
    "${EXTRA_GEN_FLAGS[@]}"

# Build the C# library.
"$DOTNET" build jolt_csharp/library

# Find all Jolt source files and our wrapper.
JOLT_SOURCES=$(find "$JOLT_ROOT/Jolt" -name "*.cpp" | sort)

# Compile the C bindings + Jolt library into a shared library.
if [[ $JOLT_SOURCES ]]; then
    "$CLANG_CXX" \
        -std=c++17 \
        -I"$JOLT_ROOT" \
        -I"$HELPER_DIR" \
        -Ijolt_csharp/c_library/include \
        -Ijolt_csharp/c_library/src \
        $JOLT_SOURCES \
        "$HELPER_DIR/jolt_helper.cpp" \
        $(find jolt_csharp/c_library/src -name '*.cpp') \
        -shared -fPIC \
        -fvisibility=hidden -fvisibility-inlines-hidden \
        -o jolt_csharp/c_library/${SHARED_LIBRARY_PREFIX}cjolt$SHARED_LIBRARY_EXT \
        "${EXTRA_CXX_FLAGS[@]}"
else
    echo "No Jolt source files found."
    exit 1
fi

# Run the C# example.
if [[ $(uname -o) == Msys ]]; then
    PATH="jolt_csharp/c_library:$PATH" "$DOTNET" run --project jolt_csharp/example_consumer
elif [[ $(uname) == Darwin ]]; then
    DYLD_LIBRARY_PATH=jolt_csharp/c_library "$DOTNET" run --project jolt_csharp/example_consumer
else
    LD_LIBRARY_PATH=jolt_csharp/c_library "$DOTNET" run --project jolt_csharp/example_consumer
fi
