#include "generators/c/binding_containers.h"
#include "generators/c/module.h"

#include <string_view>

namespace mrbind::C::Modules
{
    struct StdVector : DeriveModule<StdVector>
    {
        // Helper: split "JPH::Array" → QualifiedName{}.AddPart("JPH").AddPart("Array")
        static cppdecl::QualifiedName ParseQualifiedName(std::string_view s)
        {
            cppdecl::QualifiedName result;
            while (!s.empty())
            {
                auto pos = s.find("::");
                std::string_view part = (pos == std::string_view::npos) ? s : s.substr(0, pos);
                result.AddPart(std::string(part));
                s = (pos == std::string_view::npos) ? std::string_view{} : s.substr(pos + 2);
            }
            return result;
        }

        void RegisterCommandLineFlags(CommandLineParser &args_parser) override
        {
            args_parser.AddFlag("--vector-like-container", {
                .allow_repeat = true,
                .arg_names = {"qualified::Name", "include/header.h"},
                .desc = "Bind an additional vector-like container (same interface as std::vector) "
                        "under its fully-qualified C++ name. The second argument is the header to "
                        "#include in the generated source. Can be specified multiple times.\n"
                        "Example: --vector-like-container JPH::Array Jolt/Core/Array.h\n"
                        "If the container's iterator type is a raw pointer (e.g. T*), pass the "
                        "additional flag --vector-like-container-raw-pointer-iterators immediately "
                        "after to suppress iterator binding (which would otherwise generate invalid C++).",
                .func = [this](CommandLineParser::ArgSpan args)
                {
                    binder_vector.targets.push_back({
                        .generic_cpp_container_names = {ParseQualifiedName(args[0])},
                        .stdlib_container_header = std::string(args[1]),
                    });
                },
            });
            args_parser.AddFlag("--vector-like-container-raw-pointer-iterators", {
                .allow_repeat = true,
                .arg_names = {},
                .desc = "Mark the most recently added --vector-like-container as having raw-pointer "
                        "iterators (e.g. T*). Suppresses all iterator-related C API for that container.",
                .func = [this](CommandLineParser::ArgSpan)
                {
                    if (binder_vector.targets.empty())
                        throw std::runtime_error("--vector-like-container-raw-pointer-iterators must follow --vector-like-container");
                    binder_vector.targets.back().iterator_is_raw_pointer = true;
                },
            });
        }


        MetaContainerBinder binder_vector = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("vector")},
                    .stdlib_container_header = "vector",
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::contiguous,
                .has_mutable_iterators = true,
                .has_resize = true,
                .has_capacity = true,
                .has_front_back = true,
                .has_push_back = true,
                .push_front_requires_assignment = true,
                .insert_requires_assignment = true,
            },
        };

        MetaContainerBinder binder_deque = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("deque")},
                    .stdlib_container_header = "deque",
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::random_access,
                .has_mutable_iterators = true,
                .has_resize = true,
                .has_front_back = true,
                .has_push_back = true,
                .has_push_front = true,
                .insert_requires_assignment = true,
            },
        };

        // Only `std::list`. No `std::forward_list` support for now because the interface is way too different.
        MetaContainerBinder binder_list = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("list")},
                    .stdlib_container_header = "list",
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::bidirectional,
                .has_mutable_iterators = true,
                .has_resize = true,
                .has_front_back = true,
                .has_push_back = true,
                .has_push_front = true,
            },
        };

        MetaContainerBinder binder_set = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("set")},
                    .stdlib_container_header = "set",
                },
                {
                    .generic_cpp_container_names = {
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("btree_set"),
                    },
                    .stdlib_container_header = "parallel_hashmap/btree.h", // Don't have a separate category for third-party headers yet, so this goes into the standard headers. Oh well.
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::bidirectional,
                .is_set = true,
            },
        };

        MetaContainerBinder binder_multiset = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("multiset")},
                    .stdlib_container_header = "set",
                },
                {
                    .generic_cpp_container_names = {
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("btree_multiset"),
                    },
                    .stdlib_container_header = "parallel_hashmap/btree.h", // Don't have a separate category for third-party headers yet, so this goes into the standard headers. Oh well.
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::bidirectional,
                .is_set = true,
                .is_multi = true,
            },
        };

        MetaContainerBinder binder_unordered_set = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("unordered_set")},
                    .stdlib_container_header = "unordered_set",
                },
                {
                    .generic_cpp_container_names = {
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("flat_hash_set"),
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("node_hash_set"),
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("parallel_flat_hash_set"),
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("parallel_node_hash_set"),
                    },
                    .stdlib_container_header = "parallel_hashmap/phmap.h", // Don't have a separate category for third-party headers yet, so this goes into the standard headers. Oh well.
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::forward, // Unordered sets have forward iterators, while the normal sets have bidirectional ones. Interesting!
                .is_set = true,
                .insert_requires_assignment = true, // Huh!
            },
        };

        MetaContainerBinder binder_unordered_multiset = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("unordered_multiset")},
                    .stdlib_container_header = "unordered_set",
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::forward, // Unordered sets have forward iterators, while the normal sets have bidirectional ones. Interesting!
                .is_set = true,
                .is_multi = true,
                .insert_requires_assignment = true, // Huh!
            },
        };

        MetaContainerBinder binder_map = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("map")},
                    .stdlib_container_header = "map",
                },
                {
                    .generic_cpp_container_names = {
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("btree_map"),
                    },
                    .stdlib_container_header = "parallel_hashmap/btree.h", // Don't have a separate category for third-party headers yet, so this goes into the standard headers. Oh well.
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::bidirectional,
                .is_map = true,
                .has_mutable_iterators = true,
            },
        };

        MetaContainerBinder binder_multimap = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("multimap")},
                    .stdlib_container_header = "map",
                },
                {
                    .generic_cpp_container_names = {
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("btree_multimap"),
                    },
                    .stdlib_container_header = "parallel_hashmap/btree.h", // Don't have a separate category for third-party headers yet, so this goes into the standard headers. Oh well.
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::bidirectional,
                .is_map = true,
                .is_multi = true,
                .has_mutable_iterators = true,
            },
        };

        MetaContainerBinder binder_unordered_map = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("unordered_map")},
                    .stdlib_container_header = "unordered_map",
                },
                {
                    .generic_cpp_container_names = {
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("flat_hash_map"),
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("node_hash_map"),
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("parallel_flat_hash_map"),
                        cppdecl::QualifiedName{}.AddPart("phmap").AddPart("parallel_node_hash_map"),
                    },
                    .stdlib_container_header = "parallel_hashmap/phmap.h", // Don't have a separate category for third-party headers yet, so this goes into the standard headers. Oh well.
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::forward,
                .is_map = true,
                .has_mutable_iterators = true,
                .insert_requires_assignment = true, // Huh!
            },
        };

        MetaContainerBinder binder_unordered_multimap = {
            .targets = {
                {
                    .generic_cpp_container_names = {cppdecl::QualifiedName{}.AddPart("std").AddPart("unordered_multimap")},
                    .stdlib_container_header = "unordered_map",
                },
            },
            .params = {
                .iter_category = ContainerBinder::IteratorCategory::forward,
                .is_map = true,
                .is_multi = true,
                .has_mutable_iterators = true,
                .insert_requires_assignment = true, // Huh!
            },
        };

        bool ForEachBinder(auto &&func)
        {
            if (func(binder_vector            )) return true;
            if (func(binder_deque             )) return true;
            if (func(binder_list              )) return true;
            if (func(binder_set               )) return true;
            if (func(binder_multiset          )) return true;
            if (func(binder_unordered_set     )) return true;
            if (func(binder_unordered_multiset)) return true;
            if (func(binder_map               )) return true;
            if (func(binder_multimap          )) return true;
            if (func(binder_unordered_map     )) return true;
            if (func(binder_unordered_multimap)) return true;
            return false;
        }

        std::optional<Generator::BindableType> GetBindableType(Generator &generator, const cppdecl::Type &type, const std::string &type_str) override
        {
            (void)type_str;

            std::optional<Generator::BindableType> ret;
            ForEachBinder([&](MetaContainerBinder &binder){ret = binder.MakeBinding(generator, type); return bool(ret);});

            return ret;
        }

        std::optional<std::string> GetCppIncludeForQualifiedName(Generator &generator, const cppdecl::QualifiedName &name) override
        {
            (void)generator;

            std::optional<std::string> ret;

            ForEachBinder([&](MetaContainerBinder &binder)
            {
                for (const auto &target : binder.targets)
                {
                    bool found = false;
                    for (const auto &target_name : target.generic_cpp_container_names)
                    {
                        if (name.Equals(target_name, cppdecl::QualifiedName::EqualsFlags::allow_less_parts_in_target | cppdecl::QualifiedName::EqualsFlags::allow_missing_final_template_args_in_target))
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found)
                    {
                        ret = target.stdlib_container_header;
                        return true;
                    }
                }

                return false;
            });

            return ret;
        }
    };
}
