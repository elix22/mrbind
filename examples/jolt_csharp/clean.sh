#!/bin/bash

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR/.."

rm -rf jolt_csharp/c_library jolt_csharp/library/{src,bin,obj} jolt_csharp/example_consumer/{bin,obj}
