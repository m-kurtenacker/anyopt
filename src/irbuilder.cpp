#include "irbuilder.h"

IRBuilder::DefType IRBuilder::resolvedef (std::string def_type) {
    static const std::map<std::string, DefType> TypeMap {
#define MAP(NAME, CLASS) {NAME, DefType::CLASS},
        DefTypeEnum(MAP)
#undef MAP
    };

    auto it = TypeMap.find(def_type);
    if (it != TypeMap.end())
        return it->second;
    else {
        std::cerr << "Unknown def type: " << def_type << std::endl;
        assert(false);
    }
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
    if(it == known_defs.end())
       std::cerr << "Unknown argument name: " << type_name << std::endl;
    assert(it != known_defs.end() && "Unknown argument name!");
    return it->second;
}

const thorin::Def * IRBuilder::build_Constant (json desc) {
    auto const_type = typetable_.get_type(desc["const_type"]);
    auto primtype = const_type->as<thorin::PrimType>();
    assert(primtype->length() == 1);

    thorin::Box value;
    switch (primtype->tag()) {
#define THORIN_I_TYPE(T, M) case thorin::PrimType_##T: { value = thorin::Box(desc["value"].get<thorin::M>()); break; }
#define THORIN_BOOL_TYPE(T, M) case thorin::PrimType_##T: { value = thorin::Box(desc["value"].get<M>()); break; }
#define THORIN_F_TYPE(T, M) case thorin::PrimType_##T: { value = thorin::Box(desc["value"].get<double>()); break; }
#include <thorin/tables/primtypetable.h>
    default:
        assert(false && "not implemented");
    }
    const thorin::Def* literal = world_.literal(primtype->primtype_tag(), value, {});
    return literal;
}

const thorin::Def * IRBuilder::build_Top (json desc) {
    auto const_type = typetable_.get_type(desc["const_type"]);
    if (auto vector_type = const_type->isa<thorin::VectorType>())
        assert(vector_type->length() == 1);

    return world_.top(const_type);
}

const thorin::Def * IRBuilder::build_Bottom (json desc) {
    auto const_type = typetable_.get_type(desc["const_type"]);
    if (auto vector_type = const_type->isa<thorin::VectorType>())
        assert(vector_type->length() == 1);

    return world_.bottom(const_type);
}

const thorin::Def * IRBuilder::build_Alloc (json desc) {
    auto args = get_arglist(desc["args"]);
    auto target_type = typetable_.get_type(desc["target_type"]);

    assert(args.size() == 2);

    return world_.alloc(target_type, args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Known (json desc) {
    auto def = get_def(desc["def"]);

    return world_.known(def);
}

const thorin::Def * IRBuilder::build_Sizeof (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);

    return world_.size_of(target_type);
}

const thorin::Def * IRBuilder::build_Alignof (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);

    return world_.align_of(target_type);
}

const thorin::Def * IRBuilder::build_Select (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 3);

    return world_.select(args[0], args[1], args[2]);
}

const thorin::Def * IRBuilder::build_Continuation (json desc) {
    thorin::Continuation* continuation = nullptr;

    auto forward_decl = known_defs.find(desc["name"]);
    if (forward_decl != known_defs.end()) {
        continuation = forward_decl->second->as_nom<thorin::Continuation>();
    } else {
        if (desc.contains("intrinsic")) {
            if (desc["intrinsic"].get<std::string>() == "branch") {
                continuation = world_.branch();
            }
        } else {
            auto fn_type = typetable_.get_type(desc["fn_type"])->as<thorin::FnType>();
            continuation = world_.continuation(fn_type);
        }
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

    if (desc.contains("intrinsic")) {
        if (desc["intrinsic"].get<std::string>() == "branch") {
            //pass
        } else {
            continuation->set_name(desc["intrinsic"]);
            continuation->set_intrinsic();
        }
    } else {
        continuation->set_name(desc["name"]);
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

const thorin::Def * IRBuilder::build_LEA (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 2);

    return world_.lea(args[0], args[1], {});
}

const thorin::Def * IRBuilder::build_Load (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 2);

    return world_.load(args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Extract (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 2);

    return world_.extract(args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Insert (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 3);

    return world_.insert(args[0], args[1], args[2]);
}

const thorin::Def * IRBuilder::build_Cast (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);
    auto source = get_def(desc["source"]);

    return world_.cast(target_type, source);
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
        assert(false);
}

const thorin::Def * IRBuilder::build_Cmp (json desc) {
    auto args = get_arglist(desc["args"]);
    auto tag = resolve_cmp_tag(desc["op"]);

    assert(args.size() == 2);

    return world_.cmp(tag, args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Run (json desc) {
    auto target = get_def(desc["target"]);

    return world_.run(target);
}

const thorin::Def * IRBuilder::build_Hlt (json desc) {
    auto target = get_def(desc["target"]);

    return world_.hlt(target);
}

const thorin::Def * IRBuilder::build_Store (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 3);

    return world_.store(args[0], args[1], args[2]);
}

const thorin::Def * IRBuilder::build_Enter (json desc) {
    auto mem = get_def(desc["mem"]);

    return world_.enter(mem);
}

const thorin::Def * IRBuilder::build_Slot (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);
    auto frame = get_def(desc["frame"]);

    return world_.slot(target_type, frame);
}

const thorin::Def * IRBuilder::build_Bitcast (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);
    auto source = get_def(desc["source"]);

    return world_.bitcast(target_type, source);
}

const thorin::Def * IRBuilder::build_IndefiniteArray (json desc) {
    auto elem_type = typetable_.get_type(desc["elem_type"]);
    auto dim = get_def(desc["dim"]);

    return world_.indefinite_array(elem_type, dim);
}

const thorin::Def * IRBuilder::build_DefiniteArray (json desc) {
    auto elem_type = typetable_.get_type(desc["elem_type"]);
    auto args = get_arglist(desc["args"]);

    return world_.definite_array(elem_type, args);
}

const thorin::Def * IRBuilder::build_Global (json desc) {
    bool is_mutable = desc["mutable"];
    auto init = get_def(desc["init"]);

    return world_.global(init, is_mutable);
}

const thorin::Def * IRBuilder::build_Closure (json desc) {
    auto args = get_arglist(desc["args"]);
    auto closure_type = typetable_.get_type(desc["closure_type"])->as<thorin::ClosureType>();

    assert(args.size() == 2);

    return world_.closure(closure_type, args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Struct (json desc) {
    auto args = get_arglist(desc["args"]);
    auto struct_type = typetable_.get_type(desc["struct_type"])->as<thorin::StructType>();

    return world_.struct_agg(struct_type, args);
}

const thorin::Def * IRBuilder::build_Tuple (json desc) {
    auto args = get_arglist(desc["args"]);

    return world_.tuple(args);
}

const thorin::Def * IRBuilder::build_Vector (json desc) {
    auto args = get_arglist(desc["args"]);

    return world_.vector(args);
}

const thorin::Def * IRBuilder::build_Filter (json desc) {
    auto args = get_arglist(desc["args"]);

    return world_.filter(args);
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
