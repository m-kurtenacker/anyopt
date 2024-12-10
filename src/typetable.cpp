#include "anyopt/typetable.h"

namespace anyopt {

TypeTable::OptiType TypeTable::resolvetype (std::string type_name) {
    static const std::map<std::string, OptiType> TypeMap {
#define MAP(NAME, CLASS) {NAME, OptiType::CLASS},
        TypeTableEnum(MAP)
#undef MAP
    };

    auto it = TypeMap.find(type_name);
    if (it != TypeMap.end())
        return it->second;
    else {
        std::cerr << "Unknown type: " << type_name << std::endl;
        abort();
    }
}


thorin::PrimTypeTag TypeTable::resolvetag (std::string type_tag) {
    static const std::map<std::string, thorin::PrimTypeTag> TypeMap {
#define THORIN_ALL_TYPE(T, M) {#T, thorin::PrimTypeTag::PrimType_##T},
#include <thorin/tables/primtypetable.h>
    };

    auto it = TypeMap.find(type_tag);
    if (it != TypeMap.end())
        return it->second;
    else {
        std::cerr << "Unknown primtypetag: " << type_tag << std::endl;
        abort();
    }
}

thorin::AddrSpace TypeTable::resolveaddrspace (std::string addr_space) {
    static const std::map<std::string, thorin::AddrSpace> AddrSpaceMap {
#define MAP(NAME, CLASS) {NAME, thorin::AddrSpace::CLASS},
        AddrSpaceEnum(MAP)
#undef MAP
    };

    auto it = AddrSpaceMap.find(addr_space);
    if (it != AddrSpaceMap.end())
        return it->second;
    else {
        std::cerr << "Unknown addrspace: " << addr_space << std::endl;
        abort();
    }
}

thorin::Array<const thorin::Type*> TypeTable::get_arglist (json arg_list) {
    //Get all argument types based on their alias.
    thorin::Array<const thorin::Type*> args(arg_list.size());
    for (int argnum = 0; argnum < arg_list.size(); ++argnum) {
        json arg_desc = arg_list[argnum];
        args[argnum] = get_type(arg_desc);
    }

    return args;
}

const thorin::Type* TypeTable::get_type (std::string type_name) {
    auto it = known_types.find(type_name);
    if (it == known_types.end())
        std::cerr << "Unknown argument type: " << type_name << std::endl;
    assert(it != known_types.end() && "Unknown argument type!");
    return it->second;
}

const thorin::Type * TypeTable::build_DefiniteArrayType(json desc) {
        auto args = get_arglist(desc["args"]);
        assert(args.size() == 1);
        size_t length = desc["length"].get<size_t>();
        return world().definite_array_type(args[0], length);
}

const thorin::Type * TypeTable::build_IndefiniteArrayType(json desc) {
        auto args = get_arglist(desc["args"]);
        assert(args.size() == 1);
        return world().indefinite_array_type(args[0]);
}

const thorin::Type * TypeTable::build_BottomType(json desc) {
        return world().bottom_type();
}

const thorin::Type * TypeTable::build_FnType(json desc) {
        auto args = get_arglist(desc["args"]);
        return world().fn_type(args);
}

const thorin::Type * TypeTable::build_ClosureType(json desc) {
        auto args = get_arglist(desc["args"]);
        return world().closure_type(args);
}

const thorin::Type * TypeTable::build_FrameType(json desc) {
        return world().bottom_type();
}

const thorin::Type * TypeTable::build_MemType(json desc) {
        return world().mem_type();
}

const thorin::Type * TypeTable::build_StructType(json desc) {
        thorin::StructType* struct_type;
        auto forward_decl = known_types.find(desc["name"]);
        auto arg_names = desc["arg_names"];

        if (forward_decl != known_types.end()) {
            struct_type = const_cast<thorin::StructType*>(forward_decl->second->as<thorin::StructType>());
        } else {
            auto name = desc["struct_name"].get<std::string>();
            struct_type = world().struct_type(name, arg_names.size());
        }

        if (desc.contains("args")) {
            auto args = get_arglist(desc["args"]);
            assert(arg_names.size() == args.size());
            for (size_t i = 0; i < args.size(); ++i) {
                auto arg_name = desc["arg_names"][i].get<std::string>();
                struct_type->set_op(i, args[i]);
                struct_type->set_op_name(i, arg_name);
            }
        }

        return struct_type;
}

const thorin::Type * TypeTable::build_VariantType(json desc) {
        thorin::VariantType* variant_type;
        auto forward_decl = known_types.find(desc["name"]);
        auto arg_names = desc["arg_names"];

        if (forward_decl != known_types.end()) {
            variant_type = const_cast<thorin::VariantType*>(forward_decl->second->as<thorin::VariantType>());
        } else {
            auto name = desc["variant_name"].get<std::string>();
            variant_type = world().variant_type(name, arg_names.size());
        }

        if (desc.contains("args")) {
            auto args = get_arglist(desc["args"]);
            for (size_t i = 0; i < args.size(); ++i) {
                auto arg_name = desc["arg_names"][i].get<std::string>();
                variant_type->set_op(i, args[i]);
                variant_type->set_op_name(i, arg_name);
            }
        }

        return variant_type;
}

const thorin::Type * TypeTable::build_TupleType(json desc) {
        auto args = get_arglist(desc["args"]);
        return world().tuple_type(args);
}

const thorin::Type * TypeTable::build_PrimType(json desc) {
        auto tag = resolvetag(desc["tag"]);
        size_t length = desc["length"].get<size_t>();
        return world().prim_type(tag, length);
}

const thorin::Type * TypeTable::build_PtrType(json desc) {
        auto args = get_arglist(desc["args"]);
        assert(args.size() == 1);
        size_t length = desc["length"].get<size_t>();
        int32_t device = -1;
        auto addrspace = thorin::AddrSpace::Generic;
        if (desc.contains("addrspace")) {
            addrspace = resolveaddrspace(desc["addrspace"]);
        }

        return world().ptr_type(args[0], length, addrspace);
}

const thorin::Type * TypeTable::reconstruct_type(json desc) {
    const thorin::Type* return_type = nullptr;
    switch (resolvetype(desc["type"])) {
#define CASE(NAME, CLASS) case OptiType::CLASS: { return_type = build_##CLASS(desc); break; }
    TypeTableEnum(CASE)
#undef CASE
    default:
        std::cerr << "Type is invalid" << std::endl;
        std::cerr << desc["name"] << std::endl;
    }
    assert(return_type);
    return known_types[desc["name"]] = return_type;
}

}
