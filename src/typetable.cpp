#include "typetable.h"

TypeTable::OptiType TypeTable::resolvetype (std::string type_name) {
    static const std::map<std::string, OptiType> TypeMap {
        {"def_array", OptiType::DefiniteArrayType},
        {"indef_array", OptiType::IndefiniteArrayType},
        {"bottom", OptiType::BottomType},
        {"fn", OptiType::FnType},
        {"cl", OptiType::ClosureType},
        {"frame", OptiType::FrameType},
        {"mem", OptiType::MemType},
        {"struct", OptiType::StructType},
        {"variant", OptiType::VariantType},
        {"tuple", OptiType::TupleType},
        {"prim", OptiType::PrimType},
        {"ptr", OptiType::PtrType},
    };

    auto it = TypeMap.find(type_name);
    if (it != TypeMap.end())
        return it->second;
    else
        return OptiType::InvalidType;
}


thorin::PrimTypeTag TypeTable::resolvetag (std::string type_tag) {
    static const std::map<std::string, thorin::PrimTypeTag> TypeMap {
#define THORIN_ALL_TYPE(T, M) {#T, thorin::PrimTypeTag::PrimType_##T},
#include <thorin/tables/primtypetable.h>
    };

    auto it = TypeMap.find(type_tag);
    if (it != TypeMap.end())
        return it->second;
    else
        return thorin::PrimTypeTag::PrimType_bool;
}

thorin::Array<const thorin::Type*> TypeTable::get_arglist (json arg_list) {
    //Get all argument types based on their alias.
    thorin::Array<const thorin::Type*> args(arg_list.size());
    for (int argnum = 0; argnum < arg_list.size(); ++argnum) {
        json arg_desc = arg_list[argnum];
        args[argnum] = get_type(arg_desc.get<std::string>());
    }

    return args;
}

const thorin::Type* TypeTable::get_type (std::string type_name) {
    auto it = known_types.find(type_name);
    assert(it != known_types.end() && "Unknown argument type!");
    return it->second;
}

const thorin::Type * TypeTable::reconstruct_type(json desc) {
    const thorin::Type* return_type = nullptr;
    switch (resolvetype(desc["type"])) {
    case OptiType::DefiniteArrayType: {
        auto args = get_arglist(desc["args"]);
        assert(args.size() == 1);
        size_t length = desc["length"].get<size_t>();
        return_type = world_.definite_array_type(args[0], length);
        break;
    }
    case OptiType::IndefiniteArrayType: {
        auto args = get_arglist(desc["args"]);
        assert(args.size() == 1);
        return_type = world_.indefinite_array_type(args[0]);
        break;
    }
    case OptiType::BottomType: {
        return_type = world_.bottom_type();
        break;
    }
    case OptiType::FnType: {
        auto args = get_arglist(desc["args"]);
        return_type = world_.fn_type(args);
        break;
    }
    case OptiType::ClosureType: {
        auto args = get_arglist(desc["args"]);
        return_type = world_.closure_type(args);
        break;
    }
    case OptiType::FrameType: {
        return_type = world_.bottom_type();
        break;
    }
    case OptiType::MemType: {
        return_type = world_.mem_type();
        break;
    }
    case OptiType::StructType: { //TODO: nominal type forward declarations are not supported right now.
        auto args = get_arglist(desc["args"]);
        auto name = desc["struct_name"].get<std::string>();
        auto struct_type = world_.struct_type(name, args.size());
        for (size_t i = 0; i < args.size(); ++i) {
            auto arg_name = desc["arg_names"][i].get<std::string>();
            struct_type->set(i, args[i]);
            struct_type->set_op_name(i, arg_name);
        }
        return_type = struct_type;
        break;
    }
    case OptiType::VariantType: {
        auto args = get_arglist(desc["args"]);
        auto name = desc["variant_name"].get<std::string>();
        auto variant_type = world_.variant_type(name, args.size());
        for (size_t i = 0; i < args.size(); ++i) {
            auto arg_name = desc["arg_names"][i].get<std::string>();
            variant_type->set(i, args[i]);
            variant_type->set_op_name(i, arg_name);
        }
        return_type = variant_type;
        break;
    }
    case OptiType::TupleType: {
        auto args = get_arglist(desc["args"]);
        return_type = world_.tuple_type(args);
        break;
    }
    case OptiType::PrimType : {
        auto tag = resolvetag(desc["tag"]);
        size_t length = desc["length"].get<size_t>();
        return_type = world_.prim_type(tag, length);
        break;
    }
    case OptiType::PtrType: {
        auto args = get_arglist(desc["args"]);
        assert(args.size() == 1);
        size_t length = desc["length"].get<size_t>();
        //TODO: device, addrspace
        return_type = world_.ptr_type(args[0], length);
        break;
    }
    default:
        std::cerr << "Type is invalid" << std::endl;
    }
    assert(return_type);
    return known_types[desc["name"].get<std::string>()] = return_type;
}
