#!/bin/bash

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR/.."

rm -rf bullet_csharp/c_library bullet_csharp/library/{src,bin,obj} bullet_csharp/example_consumer/{bin,obj}
