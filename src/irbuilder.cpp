#include "irbuilder.h"

IRBuilder::DefType IRBuilder::resolvedef (std::string def_type) {
    static const std::map<std::string, DefType> TypeMap {
#define MAP(NAME, CLASS) {NAME, DefType::CLASS},
        DefTypeEnum(MAP)
#undef MAP
        //{"continuation", DefType::Continuation},
        //{"const", DefType::Constant},
    };

    auto it = TypeMap.find(def_type);
    if (it != TypeMap.end())
        return it->second;
    else
        assert(false);
}

thorin::Array<const thorin::Def*> IRBuilder::get_arglist (json arg_list) {
    //Get all argument types based on their alias.
    thorin::Array<const thorin::Def*> args(arg_list.size());
    for (int argnum = 0; argnum < arg_list.size(); ++argnum) {
        json arg_desc = arg_list[argnum];
        args[argnum] = get_def(arg_desc);
    }

    return args;
}

const thorin::Def* IRBuilder::get_def (std::string type_name) {
    auto it = known_defs.find(type_name);
    assert(it != known_defs.end() && "Unknown argument name!");
    return it->second;
}

const thorin::Def * IRBuilder::build_Constant (json desc) {
    auto const_type = typetable_.get_type(desc["const_type"]);
    auto primtype = const_type->as<thorin::PrimType>();
    thorin::Box value;
    switch (primtype->tag()) {
#define THORIN_I_TYPE(T, M) case thorin::PrimType_##T: { value = thorin::Box(desc["value"].get<thorin::M>()); break; }
#define THORIN_BOOL_TYPE(T, M) case thorin::PrimType_##T: { value = thorin::Box(desc["value"].get<M>()); break; }
#define THORIN_F_TYPE(T, M) case thorin::PrimType_##T: { value = thorin::Box(desc["value"].get<double>()); break; }
#include <thorin/tables/primtypetable.h>
    }
    //thorin::Box value(desc["value"].get<thorin::s64>());
    const thorin::Def* literal = world_.literal(primtype->primtype_tag(), value, {}, primtype->length());
    return literal;
}

const thorin::Def * IRBuilder::build_Continuation (json desc) {
    thorin::Continuation* continuation = nullptr;

    auto forward_decl = known_defs.find(desc["name"]);
    if (forward_decl != known_defs.end()) {
        continuation = forward_decl->second->as_nom<thorin::Continuation>();
    } else {
        auto fn_type = typetable_.get_type(desc["fn_type"])->as<thorin::FnType>();
        continuation = world_.continuation(fn_type);
    }
    
    if (desc.contains("arg_names")) {
        for (size_t i = 0; i < desc["arg_names"].size(); ++i) {
            auto arg = desc["arg_names"][i];
            known_defs[arg] = continuation->param(i);
        }
    }

    if (desc.contains("app")) {
        auto args = get_arglist(desc["app"]["args"]);
        auto callee = get_def(desc["app"]["target"]);
        continuation->jump(callee, args);
    }

    if (desc.contains("external")) {
        if (desc["external"].get<bool>()) {
            continuation->set_name(desc["name"]);
            world_.make_external(continuation);
        }
    }

    return continuation;
}

thorin::ArithOpTag IRBuilder::resolve_arithop_tag (std::string arithop_tag) {
    static const std::map<std::string, thorin::ArithOpTag> ArithOpMap {
#define THORIN_ARITHOP(OP) {#OP, thorin::ArithOpTag::ArithOp_##OP},
#include <thorin/tables/arithoptable.h>
    };

    auto it = ArithOpMap.find(arithop_tag);
    if (it != ArithOpMap.end())
        return it->second;
    else
        return thorin::ArithOpTag::ArithOp_add;
}

const thorin::Def * IRBuilder::build_ArithOp (json desc) {
    auto args = get_arglist(desc["args"]);
    auto tag = resolve_arithop_tag(desc["op"]);

    assert(args.size() == 2);

    return world_.arithop(tag, args[0], args[1]);
}

thorin::CmpTag IRBuilder::resolve_cmp_tag (std::string cmp_tag) {
    static const std::map<std::string, thorin::CmpTag> CmpTagMap {
#define THORIN_CMP(OP) {#OP, thorin::CmpTag::Cmp_##OP},
#include <thorin/tables/cmptable.h>
    };

    auto it = CmpTagMap.find(cmp_tag);
    if (it != CmpTagMap.end())
        return it->second;
    else
        return thorin::CmpTag::Cmp_eq;
}

const thorin::Def * IRBuilder::build_Cmp (json desc) {
    auto args = get_arglist(desc["args"]);
    auto tag = resolve_cmp_tag(desc["op"]);

    assert(args.size() == 2);

    return world_.cmp(tag, args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Branch (json desc) {
    return world_.branch();
}

const thorin::Def * IRBuilder::reconstruct_def(json desc) {
    const thorin::Def* return_def = nullptr;
    switch (resolvedef(desc["type"])) {
#define CASE(NAME, CLASS) case DefType::CLASS: { return_def = build_##CLASS(desc); break; }
    DefTypeEnum(CASE)
#undef CASE
    default:
        std::cerr << "Def is invalid" << std::endl;
    }
    return known_defs[desc["name"]] = return_def;
}
