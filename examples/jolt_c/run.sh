#!/bin/bash

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR/.."

. _detail/set_env_vars.sh

./jolt_c/clean.sh

mkdir -p jolt_c/output/tmp

JOLT_ROOT="$(pwd)/../deps/JoltPhysics"
HELPER_DIR="$(pwd)/jolt_c"

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
EXTRA_GEN_FLAGS=(
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
echo "#pragma once" >jolt_c/output/tmp/combined_input.h
echo "#include \"$HELPER_DIR/jolt_helper.h\"" >>jolt_c/output/tmp/combined_input.h

# Parse the input header.
../build/mrbind \
    jolt_c/output/tmp/combined_input.h \
    -o jolt_c/output/tmp/parse_result.json \
    --ignore :: \
    --allow JoltVec3f \
    --allow JoltVec3 \
    --allow JoltQuat \
    --allow JoltBodyID \
    --allow JoltConstraintID \
    --allow JoltShape \
    --allow JoltBoxShape \
    --allow JoltSphereShape \
    --allow JoltCapsuleShape \
    --allow JoltCylinderShape \
    --allow JoltRotatedTranslatedShape \
    --allow JoltBodyCreationSettings \
    --allow JoltPhysicsSystem \
    --allow JoltWorld \
    "${EXTRA_PARSER_FLAGS[@]+"${EXTRA_PARSER_FLAGS[@]}"}" \
    -- \
    -xc++-header \
    -resource-dir="$("$CLANG_CXX" -print-resource-dir)" \
    -I"$HELPER_DIR" \
    "${EXTRA_PARSER_CXX_FLAGS[@]}"

# Generate the C bindings.
../build/mrbind_gen_c \
    --input jolt_c/output/tmp/parse_result.json \
    --output-header-dir jolt_c/output/include \
    --output-source-dir jolt_c/output/src \
    --helper-name-prefix Jolt_ \
    --helper-macro-name-prefix JOLT_ \
    --map-path "$HELPER_DIR" jolt \
    --assume-include-dir "$HELPER_DIR" \
    "${EXTRA_GEN_FLAGS[@]}"

# Find all Jolt source files and our wrapper.
JOLT_SOURCES=$(find "$JOLT_ROOT/Jolt" -name "*.cpp" | sort)

# Compile the generated bindings + Jolt library into a shared library.
if [[ $JOLT_SOURCES ]]; then
    "$CLANG_CXX" \
        -std=c++17 \
        -I"$JOLT_ROOT" \
        -I"$HELPER_DIR" \
        -Ijolt_c/output/include \
        -Ijolt_c/output/src \
        $JOLT_SOURCES \
        "$HELPER_DIR/jolt_helper.cpp" \
        $(find jolt_c/output/src -name '*.cpp') \
        -shared -fPIC \
        -fvisibility=hidden -fvisibility-inlines-hidden \
        -o jolt_c/output/libjolt_bindings$SHARED_LIBRARY_EXT \
        "${EXTRA_CXX_FLAGS[@]}"
else
    echo "No Jolt source files found."
    exit 1
fi

# Compile the test executable.
cc \
    -std=c11 -Wall -Wextra \
    jolt_c/example_consumer.c \
    -Ijolt_c/output/include \
    -Ljolt_c/output -ljolt_bindings \
    -o jolt_c/output/example_consumer \
    "${EXTRA_C_FLAGS[@]}"

# Run the test executable.
if [[ $(uname) == Darwin ]]; then
    DYLD_LIBRARY_PATH=jolt_c/output jolt_c/output/example_consumer
else
    LD_LIBRARY_PATH=jolt_c/output jolt_c/output/example_consumer
fi
