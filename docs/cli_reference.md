# mrbind CLI Reference

This page is a complete reference for all command-line flags of the three mrbind executables.
For step-by-step usage instructions, see the individual workflow docs:
[Running the parser](running_parser.md) · [Generating C bindings](generating_c.md) · [Generating C# bindings](generating_csharp.md)

---

## mrbind — the parser

```
mrbind [mrbind_flags] -- [clang_flags]
```

`[clang_flags]` are the normal Clang/GCC-style compiler flags (`-I`, `-D`, `-std=c++??`, etc.).
If `--` is omitted entirely, Clang will try to extract flags from `compile_commands.json`, which is rarely what you want.

### Output & filtering

| Flag | Arguments | Description |
|---|---|---|
| `-o` | `<output.cpp>` | Redirect output to a file. Specifying this flag multiple times multiplexes the output between several files, which can be compiled in parallel or sequentially for lower RAM usage. |
| `--format` | `<F>` | Output format: `json` (default) or `macros`. Use `json` for the C / C# generators; `macros` is for the Python (pybind11) backend. |
| `--ignore` | `<T>` | Do not emit bindings for entity `T`. Use a fully qualified C++ name. The final template arguments can be omitted to match any specialization. Use `::` to reject the global namespace. Enclose in `/slashes/` to match as a regex. Can be repeated. |
| `--allow` | `<T>` | Un-ban a sub-entity of something banned by `--ignore`. Same syntax as `--ignore`. |
| `--skip-mentions-of` | `<T>` | Skip any data members, base classes, and functions whose return type or parameter type is `T` (cv-ref qualifiers are ignored). The type can be a regex in `/slashes/`. Unlike `--ignore`, template arguments cannot be omitted here. |

### Cross-platform type canonicalization

| Flag | Description |
|---|---|
| `--canonicalize-to-fixed-size-typedefs` | Canonicalize integer types to the standard fixed-width typedefs (`int32_t`, `uint64_t`, etc.) instead of their platform-specific spellings. Do not use `long` or `long long` directly in your interface if you enable this. |
| `--canonicalize-64-to-fixed-size-typedefs` | Subset of the above: only canonicalize 64-bit wide types. Canonicalizes either `unsigned long` or `unsigned long long` to `uint64_t`, depending on the platform. |
| `--canonicalize-size_t-to-uint64_t` | Only effective if at least one of the above flags is also set, and only on Mac. On Mac, `uint64_t` is `unsigned long long` while `size_t` is `unsigned long`. Enabling this canonicalizes `unsigned long long` to `uint64_t`, which lets you use `size_t` and `ptrdiff_t` in your interface — but you must then avoid using `uint64_t` directly. |
| `--implicit-enum-underlying-type-is-always-int` | Treat the implicit underlying type of unscoped enums as `int` on all platforms. On Linux, enums with all non-negative constants may default to `unsigned int`; this flag normalizes that behavior. |

### Inheritance & templates

| Flag | Description |
|---|---|
| `--copy-inherited-members` | Copy inherited methods from base classes into derived classes. Required for languages (like C# and C) that do not have native inheritance. Does not affect constructors or members imported via `using`. |
| `--buggy-substitute-default-template-args` | Automatically instantiate function templates whose arguments are all defaulted. Currently buggy — chokes on old-style SFINAE. Works reasonably with `requires`. |

### Lifetime annotations

| Flag | Description |
|---|---|
| `--no-infer-lifetime-iterators` | Disable automatic `[[clang::lifetimebound]]` inference for `begin()`, `end()`, and unary `operator*`. These annotations are used by some backends to extend object lifetimes and prevent dangling references. |
| `--infer-lifetime-constructors` | Infer lifetime annotations for non-copy/move constructors that take pointers or references, as if `[[clang::lifetimebound]]` was used on their parameters. |

### Comment processing

| Flag | Arguments | Description |
|---|---|---|
| `--adjust-comments` | `<s/A/B/g>` | Apply a sed-like substitution rule to all parsed comments. The separator can be any character not appearing in `A` or `B`. Use `s/A/B/` for single replacement or `s/A/B/g` for global. Can be repeated for multiple rules. |

### Python-specific

| Flag | Arguments | Description |
|---|---|---|
| `--combine-types` | `<list>` | Only useful with `--format=macros` (Python backend). Merges type registration information for certain types to improve build times. `<list>` is comma-separated and can include: `cv`, `ref`, `ptr`, `smart_ptr`. |

### Diagnostics & advanced

| Flag | Arguments | Description |
|---|---|---|
| `--help` | | Show the help page. |
| `--no-cppdecl` | | Do not postprocess type names with the `cppdecl` library. Rarely needed; disabling it breaks most non-trivial use cases. |
| `--dump-command` | `<output.txt>` | Dump the resulting compilation command to a file, one argument per line. Useful for extracting commands from `compile_commands.json`. |
| `--dump-command0` | `<output.txt>` | Same as `--dump-command`, but separates arguments with null bytes instead of newlines. |
| `--ignore-pch-flags` | | Attempt to ignore PCH inclusion flags found in `compile_commands.json`. Useful when the PCH was built with a different compiler. |

---

## mrbind_gen_c — C bindings generator

Consumes the parser JSON and produces C headers and C++ implementation files.

```
mrbind_gen_c --input <file.json> --output-header-dir <dir> --output-source-dir <dir> [flags]
```

### Input / output

| Flag | Arguments | Description |
|---|---|---|
| `--help` | | Show the help page. |
| `--input` | `<filename.json>` | Input JSON produced by `mrbind --format=json`. |
| `--output-header-dir` | `<dir>` | Output directory for generated C headers. Must be empty or non-existent unless `--clean-output-dirs` is also passed. |
| `--output-source-dir` | `<dir>` | Output directory for generated C++ source files. Same rules as `--output-header-dir`. |
| `--clean-output-dirs` | | Delete the contents of the output directories before writing. Without this, non-empty output directories are an error. |
| `--output-desc-json` | `<filename.json>` | Optional. Emit an additional JSON describing the generated C code. This file is the required input for `mrbind_gen_csharp`. |
| `--verbose` | | Write additional logs. |

### Path mapping

| Flag | Arguments | Description |
|---|---|---|
| `--map-path` | `<from> <to>` | Map input header directories to output directory paths. `<from>` is an absolute or relative input directory (canonicalized internally). `<to>` is a path relative to the output directories. Can be repeated; longer prefixes take priority. Every input file must be covered by at least one `--map-path`. |
| `--assume-include-dir` | `<dir>` | Tell the generator which directories will be passed as `-I` at compile time, so that generated `#include` directives can use relative paths. Can be repeated; more-specific (deeper) directories take priority. Prefer passing a parent of your headers rather than the header directory itself when header names match generated output names. |
| `--strip-filename-suffix` | `<.ext>` | Strip this suffix from input filenames before computing output names. Common C++ extensions (`.cpp`, `.hpp`, etc.) are handled automatically. |
| `--max-header-name-length` | `<n>` | Shorten generated header filenames to at most `<n>` characters (not counting the output directory prefix or the extension). Especially relevant on Windows, which has a smaller maximum path length. A value around 100 works well. |

### Naming

| Flag | Arguments | Description |
|---|---|---|
| `--helper-name-prefix` | `<string>` | Prefix for generated helper function/type names. Typically your library name followed by `_` (e.g. `MyLib_`). Required whenever the generator needs to emit helpers, which it almost always does for non-trivial inputs. |
| `--helper-macro-name-prefix` | `<string>` | Override `--helper-name-prefix` specifically for macro names. Useful when you want the macro prefix in ALL_CAPS but the function prefix in MixedCase. |
| `--helper-header-dir` | `<dir>` | Where to put the additional helper headers, relative to `--output-header-dir`. Typically set to your library name (e.g. `--helper-header-dir MyLib_helpers`). |

### Multi-library splitting

| Flag | Arguments | Description |
|---|---|---|
| `--split-library` | `<macro_prefix> <deps> <dirs>` | Split output files into a separate sub-library. `<macro_prefix>` prefixes the export macro for that sub-library. `<deps>` is a `:` separated list of other sub-library macro prefixes this one depends on (empty string = main library). `<dirs>` is a `:`-separated list of output directories (relative to the output dirs) that belong to this sub-library. Can be repeated for multiple sub-libraries. |

### Exception and RTTI handling

| Flag | Description |
|---|---|
| `--no-handle-exceptions` | Do not wrap C++ calls in try/catch. Useful when building with exceptions disabled. The generated code already guards this with `#if`, so you may not need this flag even in that case. |
| `--no-dynamic-cast` | Do not generate `DynamicDowncastFrom` / `DynamicDowncastFromOrFail` functions. Use when the target library is compiled with `-fno-rtti`. |

### Type exposure

| Flag | Arguments | Description |
|---|---|---|
| `--expose-as-struct` | `<type>` | Bind the specified C++ class or struct as an actual C struct with the same memory layout, instead of an opaque pointer. The argument is either an exact name (with template arguments if any) or a regex in `/slashes/`. The type must be trivially copyable and standard-layout. This is required when you need to use the type as an array element. |
| `--preferred-max-num-aggregate-init-fields` | `<n>` | Do not generate aggregate initialization constructors for structs with more than `<n>` fields. Ignored if the struct is not default-constructible. |
| `--add-convenience-includes` | | Add extra `#include` directives to the output that are not strictly necessary, but may help users. Off by default due to excessive bloat in large projects. |

### Comment processing

| Flag | Arguments | Description |
|---|---|---|
| `--adjust-comments` | `<s/A/B/g>` | Apply a sed-like substitution to all comments in generated C code. Same syntax as the parser's `--adjust-comments`. Can be repeated. |

### Template instantiation

| Flag | Arguments | Description |
|---|---|---|
| `--skip-template-args-on-func` | `<func>` | Do not append explicit template arguments when calling the named function. Useful for overloaded functions that misbehave with explicit template arguments. Enabled by default for `begin`, `end`, and `swap`. Can be specified multiple times. |
| `--no-skip-template-args-on-func` | `<func>` | Opposite of the above. Primarily useful for the identifiers that have `--skip-template-args-on-func` enabled by default. |

### `std::expected` / `tl::expected`

| Flag | Description |
|---|---|
| `--merge-std-and-tl-expected` | Bind both `std::expected` and `tl::expected` to the same common C name, without the `std`/`tl` prefix. |
| `--bind-shared-ptr-virally` | When `std::shared_ptr<T>` is encountered, also generate `shared_ptr` bindings for all base and derived classes of `T`, recursively. |
| `--use-size_t-typedef-for-uint64_t` | Intended for use with the parser's `--canonicalize-size_t-to-uint64_t`. Binds `[u]int64_t` as `size_t`/`ptrdiff_t`. |
| `--reject-long-and-long-long` | Fail if the input contains `long` or `long long`. Use together with `--canonicalize-to-fixed-size-typedefs` to ensure the input was fully canonicalized. |
| `--force-emit-common-helpers` | Always emit the common helper header, even if not otherwise needed. Required by the C# generator; pass this whenever you intend to generate C# bindings. |

---

## mrbind_gen_csharp — C# bindings generator

Consumes the C bindings description JSON (produced by `mrbind_gen_c --output-desc-json`) and generates C# source files.

```
mrbind_gen_csharp --input-json <c_desc.json> --output-dir <dir> \
    --imported-lib-name <name> --helpers-namespace <NS> [flags]
```

### Input / output

| Flag | Arguments | Description |
|---|---|---|
| `--help` | | Show the help page. |
| `--input-json` | `<filename.json>` | Input JSON produced by `mrbind_gen_c --output-desc-json`. |
| `--output-dir` | `<dir>` | Directory where C# source files will be written. Must be empty unless `--clean-output-dir` is also passed. |
| `--clean-output-dir` | | Delete the contents of `--output-dir` before generating. |

### Library loading

| Flag | Arguments | Description |
|---|---|---|
| `--imported-lib-name` | `<name>` | Name of the C shared library to load at runtime. Omit the `lib` prefix and the `.dll`/`.so`/`.dylib` extension — the correct suffix is added per-platform automatically. For example, `--imported-lib-name MyLib` loads `MyLib.dll` on Windows, `libMyLib.so` on Linux, `libMyLib.dylib` on Mac. |
| `--imported-split-lib-name` | `<macro_prefix> <name>` | Override `--imported-lib-name` for a subset of files corresponding to a sub-library created with `--split-library` in the C generator. Pass the same `<macro_prefix>` values as you used in the C generator. Can be repeated. |

### Namespace and naming

| Flag | Arguments | Description |
|---|---|---|
| `--helpers-namespace` | `<name>` | C++-style `Foo::Bar` namespace name for generated helper utilities. This does not need to exist in C++. In C# it is emitted as a static class, not a true namespace. |
| `--replace-namespace` | `<from> <to>` | Replace a namespace prefix in all C++ names. Both arguments are C++-style names. The second can be `::` to remove a namespace entirely. Can be repeated; replacements apply in order. |
| `--force-namespace` | `<name>` | Prepend this C++-style namespace to any C++ name whose first component does not already match. |
| `--begin-func-names-with-lowercase` | | Name C# methods in `camelCase` style (`fooBar`) instead of the default `PascalCase` (`FooBar`). |

### Code generation style

| Flag | Arguments | Description |
|---|---|---|
| `--fat-objects` | | Emit C++ class fields as C# class fields instead of C# properties. Faster field access at the cost of larger, slower-to-construct C# objects. |
| `--wrap-doc-comments-in-summary-tag` | | Wrap every doc comment in a `<summary>` XML tag. Enables display in C# IDEs that only show XML-tagged comments. Doxygen tags inside are not translated — they are pasted verbatim. |
| `--csharp-version` | `<number>` | Target a specific C# language version. Defaults to `12`. |
| `--dotnet-version` | `<number>` | Target a specific .NET version. Pass e.g. `8` for .NET 8, `2.1` for .NET Core 2.1, or `std2.0` for .NET Standard 2.0. |
| `--no-csharp-spans` | | Disable the use of `Span<T>` and `ReadOnlySpan<T>` in generated code (including for strings). Applied automatically for older `--dotnet-version` values; pass this flag to force it regardless of version. |
| `--move-classes-returned-by-value` | | Wrap non-trivial classes returned by value from C++ functions in a `_Moved<T>` helper to emulate C++ value semantics. Makes the API harder to use (callers must unwrap `_Moved<T>`), but more semantically precise. |

### Return value handling

| Flag | Description |
|---|---|
| `--no-deref-expected` | When a C++ function returns `std/tl::expected`, return the raw `expected` object instead of automatically dereferencing it (which would throw on failure). |
| `--buggy-transparent-shared-pointers` | Experimental / not fully implemented. Attempts to make `shared_ptr`-managed classes transparent by storing the pointer directly in the class, rather than exposing a separate wrapper. Requires `--bind-shared-ptr-virally` in the parser. |

### Reference counting

| Flag | Arguments | Description |
|---|---|---|
| `--intrinsic-ref-counted-base` | `<cpp_name_prefix>` | C++ qualified name prefix of a ref-counted base class (e.g. `JPH::RefTarget`). Classes that transitively inherit from a matching base will call `AddRef()` on construction and `Release()` on disposal, instead of the generated `Destroy()` function. |

### Per-method customization

| Flag | Arguments | Description |
|---|---|---|
| `--method-precondition` | `<c_name> <csharp_condition> <message>` | Inject a runtime guard into the named C binding function. `<c_name>` is the C function name (e.g. `JPH_Body_GetInverseInertia`). `<csharp_condition>` is a C# boolean expression evaluated in the context of `this` — if it evaluates to `false`, a `System.InvalidOperationException` is thrown with `<message>` before the native call is made. Can be repeated. |
| `--array-overload-param` | `<c_name> <array_param> <size_param>` | Emit an additional C# overload for the named C function where `<array_param>` becomes a managed `T[]` array and `<size_param>` is dropped (replaced by `array.Length`). The element type `T` is inferred from the C pointer type of the parameter. A `fixed` block pins the array before the native call. Can be repeated for multiple functions, and multiple times for the same function. |

---

## Quick-reference: flags by task

| Goal | Flags to use |
|---|---|
| Filter to your namespace | `mrbind --ignore :: --allow YourNamespace` |
| Cross-platform integer types | `mrbind --canonicalize-64-to-fixed-size-typedefs` + `mrbind_gen_c --reject-long-and-long-long` |
| Expose inherited methods in C / C# | `mrbind --copy-inherited-members` |
| Expose a type as a C struct for array use | `mrbind_gen_c --expose-as-struct "Ns::Foo"` |
| Enable C# generation | `mrbind_gen_c --output-desc-json c_desc.json --force-emit-common-helpers` |
| Suppress exception wrapping | `mrbind_gen_c --no-handle-exceptions` |
| Split output into multiple DLLs | `mrbind_gen_c --split-library …` + `mrbind_gen_csharp --imported-split-lib-name …` |
| Emit batch array overloads in C# | `mrbind_gen_csharp --array-overload-param <c_func> <array_param> <size_param>` |
| Inject a runtime precondition check | `mrbind_gen_csharp --method-precondition <c_func> <condition> <message>` |
| Unity / .NET Standard 2.0 compatibility | `mrbind_gen_csharp --dotnet-version std2.0 --csharp-version 12` |
