# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## What This Project Is

mrbind parses C++ headers and auto-generates **C**, **C#**, and **Python** bindings with no separate interface description files. All annotations live in the headers themselves. It is used in production for [MeshLib](https://github.com/MeshInspector/MeshLib).

- **C backend** — custom; dumps generated function metadata as JSON so other language backends can build on it.
- **C# backend** — custom; uses `[DllImport]` to load the C shared library. The resulting assembly is cross-platform as long as the C library is compiled per-platform.
- **Python backend** — pybind11-based with custom template handling.

## One-Time Setup

```bash
# Initialize submodules (bullet, JoltPhysics, pybind11, …)
git submodule update --init --recursive

# Put your clang++ binary name in examples/cxx.txt — used by all example run.sh scripts
echo "clang++" > examples/cxx.txt
# On macOS with Homebrew LLVM:
echo "/opt/homebrew/opt/llvm/bin/clang++" > examples/cxx.txt
```

## Build mrbind

```bash
# macOS (Homebrew LLVM required)
cmake -B build \
    -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/llvm \
    -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++
cmake --build build -j4

# The three binaries produced:
#   build/mrbind           — header parser → JSON
#   build/mrbind_gen_c     — JSON → C bindings
#   build/mrbind_gen_csharp — JSON → C# bindings
```

## Running Examples

Each example has a self-contained `run.sh` that drives the full pipeline. All `run.sh` scripts must be run from the `examples/` directory (they `cd` there themselves via `$SCRIPT_DIR`).

```bash
cd examples

bash bullet_c/run.sh        # Bullet Physics — C bindings
bash bullet_csharp/run.sh   # Bullet Physics — C# bindings

bash jolt_c/run.sh          # JoltPhysics — C bindings
bash jolt_csharp/run.sh     # JoltPhysics — C# bindings

bash c/run.sh               # minimal C example
bash csharp/run.sh          # minimal C# example
bash python/run.sh          # Python (pybind11) example
```

`examples/_detail/set_env_vars.sh` is sourced by every `run.sh` — it validates that `../build/mrbind` exists and reads `CLANG_CXX` from `examples/cxx.txt`.

## mrbind Pipeline

```
C++ header (.h)
      ↓  mrbind (parser)
parse_result.json
      ↓  mrbind_gen_c
  C header + C++ source (generated bindings)
      ↓  clang++ → shared library (.dylib / .so / .dll)
      ↓  mrbind_gen_csharp (from C description JSON)
  C# source files
      ↓  dotnet build → .NET assembly
```

Key `mrbind` parser flags used in examples:

| Flag | Effect |
|---|---|
| `--ignore ::` | Ignore everything by default |
| `--allow ClassName` | Opt-in a specific type |
| `--copy-inherited-members` | Expose inherited methods on derived classes |
| `--map-path <src> <dst>` | Remap include paths in generated headers |
| `--assume-include-dir <dir>` | Treat dir as an implicit include root |

Key `mrbind_gen_c` flags:

| Flag | Effect |
|---|---|
| `--helper-name-prefix Foo_` | Prefix for helper C functions |
| `--helper-macro-name-prefix FOO_` | Prefix for helper macros |
| `--no-handle-exceptions` | Skip exception wrapping (faster, simpler) |
| `--force-emit-common-helpers` | Always emit shared helper code |
| `--expose-as-struct Ns::Foo` | Generate a C struct with same memory layout instead of an opaque pointer; enables `Foo*` as array pointer |

## Generated C Naming Conventions

Knowing these prevents guessing errors in consumer code:

| C++ | Generated C |
|---|---|
| Default constructor | `ClassName_DefaultConstruct()` |
| Single non-default constructor | `ClassName_Construct(params)` |
| Multiple constructors (N params) | `ClassName_Construct_N(params)` |
| Method | `ClassName_methodName(self, ...)` |
| Field getter | `ClassName_Get_fieldName(self)` → `const T*` |
| Field setter | `ClassName_Set_fieldName(self, value)` |
| Field mutable getter | `ClassName_GetMutable_fieldName(self)` → `T*` |
| Destructor | `ClassName_Destroy(self)` |
| Upcast (derived → base) | `Derived_MutableUpcastTo_Base(self)` |

**Never guess field accessor names** — always check the generated header. The pattern is `Get_` not just the field name.

## Wrapping Third-Party Physics Libraries

Both `examples/bullet_c` and `examples/jolt_c` follow the same pattern: a thin `*_helper.h` + `*_helper.cpp` wrapper hides the physics library's Jolt/Bullet internals (virtual interface implementations, layer systems) and exposes a clean header using only standard C++ types. mrbind then parses only the helper header.

**Do not expose the raw physics library headers to mrbind** — they contain types mrbind cannot handle cleanly (virtual callbacks, template-heavy internals).

## Known Pitfalls

### mrbind: `Destroy()` method name conflicts with generated destructor

mrbind generates `ClassName_Destroy(ptr)` as the C destructor for every class. If the wrapped C++ class also has a method named `Destroy()`, the C function names collide.

**Fix:** Rename any `Destroy()` methods on wrapped classes to something else (e.g. `Release()`).

### mrbind: `enum class` in your own wrapper header causes ODR conflict in generated C++ TU

When your mrbind *input wrapper header* defines `enum class Foo { ... }` and you `--allow Foo`, mrbind's generated `.cpp` includes both the C header (`typedef enum Foo {...} Foo`) and the wrapper header (`enum class Foo : int {...}`). These cannot coexist in the same C++ TU — clang reports "enumeration previously declared as unscoped".

**This does NOT apply to third-party headers** (e.g. Jolt's `enum class EActivation`). The generated C name is `JPH_EActivation`, which differs from `JPH::EActivation`, so there is no ODR conflict. Just add `--allow JPH::EActivation` and it works.

**Fix (own wrapper headers only):** Replace `enum class` types in your own mrbind wrapper header with `static const int` / `static const unsigned int` constants. Update all method signatures to use `int`/`unsigned int`. Remove the corresponding `--allow EnumName` lines from `run.sh`.

### mrbind: `--allow` only matches named types, not global variables

`--allow FooName` does nothing when `FooName` is a `static const int` variable rather than a class/struct/enum. mrbind will not generate C bindings for individual global constants.

**Fix:** Remove those `--allow` lines. C consumers use integer literals with comments, or a separate `#define`-based constants header.

### mrbind: protected member access from free-function helpers

A free-function helper `void helper(BaseClass& b)` cannot access `b.mProtectedField` even if `mProtectedField` is `protected` in `BaseClass`. C++ only permits protected access through a derived-class `this`.

**Fix:** Use a `#define HELPER_MACRO(ref)` that expands inside each derived-class constructor body (valid via `this`). For cross-object access, add a `void* getHandle() const` accessor to the base class.

### mrbind: `--expose-as-struct` requires public fields; private-only classes fall back to byte array

`--expose-as-struct Foo` reconstructs the C struct from parsed public fields. If all fields are `private` (e.g. `JPH::BodyID` has only `private uint32_t mID`), the generator previously threw "The struct has no known fields". The generator has been patched to fall back to `unsigned char _data[N]` using the known type size, preserving layout for array use.

**Fix:** No action needed after the patch — the fallback is automatic. If you encounter this on an unpatched binary, either make fields `public` or use `struct` instead of `class` in your wrapper.

### mrbind: forward-declared types must have their definition header included

`BodyInterface.h` only forward-declares `BroadPhaseLayerFilter` (`class BroadPhaseLayerFilter;`). Adding `--allow JPH::BroadPhaseLayerFilter` alone is not enough — mrbind needs the full definition. Add the defining header (`BroadPhaseLayer.h`) to `combined_input.h` explicitly.

**Fix:** When a parameter type is only forward-declared in the headers you're binding, add that type's defining header to your combined input header.

### Jolt: `Trace` and `AssertFailed` function pointers must be set at init

Without setting `Trace = myFn;` before any Jolt call, the process crashes immediately (SIGTRAP / exit 133) because Jolt's internal logging hits a null function pointer.

**Fix:** In your init function, before `RegisterDefaultAllocator()`:
```cpp
Trace = [](const char* fmt, ...) {
    va_list a; va_start(a, fmt); vprintf(fmt, a); va_end(a); putchar('\n');
};
JPH_IF_ENABLE_ASSERTS(AssertFailed = myAssertFn;)
```

### Jolt: TempAllocator needs at least 64 MB

`TempAllocatorImpl(16 * 1024 * 1024)` is too small for even a 2-body scene. Jolt logs "Out of memory trying to allocate ~56 MB" and crashes.

**Fix:** Use `TempAllocatorImpl(64 * 1024 * 1024)` as the minimum.

### Bullet/Windows: Use ClangCL, not pure MSVC

Pure MSVC with `/EHsc` refuses to compile `btDbvtBroadphase` because `btDbvt` has a private copy constructor. Clang accepts it.

**Fix:** Pass `-T ClangCL` to the CMake configure step on Windows. In CMakeLists.txt, use `NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang"` to distinguish ClangCL from MSVC (CMake sets `MSVC=TRUE` for both).

### Bullet/macOS: Xcode is a multi-config generator

Do not pass `-DCMAKE_BUILD_TYPE` at configure time when using `-G Xcode`. Set build type at build time: `cmake --build . --config Release`.

### Bullet/Emscripten: Avoid `lib` prefix on static library

CMake adds the `lib` prefix by default; Emscripten targets expect `cbullet.a` not `libcbullet.a`.

**Fix:** `set_target_properties(cbullet PROPERTIES PREFIX "")` inside the `if(EMSCRIPTEN)` branch.

### Bullet/submodule: push to the bullet submodule repo, not mrbind

`deps/bullet` is a submodule pointing to `elix22/bullet3`. Pushing from the mrbind repo root does not update bullet's CI.

**Fix:** `cd deps/bullet && git push origin master`.

## Coding Principles

### Think before coding

State assumptions explicitly. If a simpler approach exists, say so. If behavior may differ across the binding pipeline (parser → generator → consumer), name which layer an assumption applies to.

### Simplicity first

No features beyond what was asked. No abstractions for single-use code. The binding pipeline already adds inherent complexity — don't compound it.

### Surgical changes

Touch only what you must. When editing `run.sh` or generated code patterns, match the existing style. Never manually edit files under `output/` or `c_library/` — they are generated artifacts rebuilt by `run.sh`.

### Never edit machine-generated binding files

Files under `deps/JoltPhysics/bindings/c/` (and any other `bindings/` output directories) are machine-generated by `generate.sh`. **Never edit them directly** — changes will be overwritten on the next regeneration. If a generated file has a bug (wrong type, missing include, etc.), fix the generator (`src/generators/c/`, `src/generators/csharp/`) or the parser flags in `generate.sh`, then re-run `generate.sh`.

**Example 1:** `unsigned long long` vs `uint64_t` cross-platform mismatch — fix: `--canonicalize-64-to-fixed-size-typedefs` in the `mrbind` parser invocation inside `generate.sh`.

**Example 2:** `size_t` fields emitted as `unsigned long` (wrong on 32-bit Android and Windows) — fix: `PortableTypeStr()` in `src/generators/c/generator.cpp` that prefers `type.pretty` (e.g. `size_t`) over `type.canonical` (e.g. `unsigned long`) when the pretty form is a plain C identifier.

### Verify end-to-end

For binding work, "done" means the full pipeline runs without errors **and** the consumer program produces correct output. Compilation success alone is not sufficient.
