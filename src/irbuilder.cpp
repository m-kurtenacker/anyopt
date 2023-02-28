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
#define THORIN_F_TYPE(T, M) case thorin::PrimType_##T: { value = thorin::Box((thorin::M)desc["value"].get<double>()); break; }
#include <thorin/tables/primtypetable.h>
    default:
        assert(false && "not implemented");
    }
    const thorin::Def* literal = world().literal(primtype->primtype_tag(), value, {});
    return literal;
}

const thorin::Def * IRBuilder::build_Top (json desc) {
    auto const_type = typetable_.get_type(desc["const_type"]);
    if (auto vector_type = const_type->isa<thorin::VectorType>())
        assert(vector_type->length() == 1);

    return world().top(const_type);
}

const thorin::Def * IRBuilder::build_Bottom (json desc) {
    auto const_type = typetable_.get_type(desc["const_type"]);
    if (auto vector_type = const_type->isa<thorin::VectorType>())
        assert(vector_type->length() == 1);

    return world().bottom(const_type);
}

const thorin::Def * IRBuilder::build_Alloc (json desc) {
    auto args = get_arglist(desc["args"]);
    auto target_type = typetable_.get_type(desc["target_type"]);

    assert(args.size() == 2);

    return world().alloc(target_type, args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Known (json desc) {
    auto def = get_def(desc["def"]);

    return world().known(def);
}

const thorin::Def * IRBuilder::build_Sizeof (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);

    return world().size_of(target_type);
}

const thorin::Def * IRBuilder::build_Alignof (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);

    return world().align_of(target_type);
}

const thorin::Def * IRBuilder::build_Select (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 3);

    return world().select(args[0], args[1], args[2]);
}

const thorin::Def * IRBuilder::build_Continuation (json desc) {
    thorin::Continuation* continuation = nullptr;

    auto forward_decl = known_defs.find(desc["name"]);
    if (forward_decl != known_defs.end()) {
        continuation = forward_decl->second->as_nom<thorin::Continuation>();
    } else {
        if (desc.contains("internal")) {
            continuation = extern_globals_.lookup(desc["internal"]).value_or(nullptr)->as<thorin::Continuation>();
            if (!continuation) {
                auto fn_type = typetable_.get_type(desc["fn_type"])->as<thorin::FnType>();
                continuation = world().continuation(fn_type);
                continuation->set_name(desc["internal"]);
                world().make_external(continuation);
                continuation->attributes().cc = thorin::CC::Internal;
                extern_globals_.emplace(continuation->name(), continuation);
            }
        } else if (desc.contains("intrinsic")) {
            if (desc["intrinsic"].get<std::string>() == "branch") {
                continuation = world().branch();
            } else if (desc["intrinsic"].get<std::string>() == "match") {
                auto variant_type = typetable_.get_type(desc["variant_type"]);
                size_t num_patterns = desc["num_patterns"];
                continuation = world().match(variant_type, num_patterns);
            } else {
                auto fn_type = typetable_.get_type(desc["fn_type"])->as<thorin::FnType>();
                continuation = world().continuation(fn_type);
            }
        } else {
            auto fn_type = typetable_.get_type(desc["fn_type"])->as<thorin::FnType>();
            continuation = world().continuation(fn_type);
        }
    }

    if (desc.contains("filter")) {
        auto filter = get_def(desc["filter"])->as<thorin::Filter>();
        continuation->set_filter(filter);
    }
    
    if (desc.contains("arg_names")) {
        for (size_t i = 0; i < desc["arg_names"].size(); ++i) {
            auto arg = desc["arg_names"][i];
            known_defs[arg] = continuation->param(i);
        }
    }

    if (desc.contains("external")) {
        continuation->set_name(desc["external"]);
        world().make_external(continuation);
    }

    if (desc.contains("device")) {
        continuation->set_name(desc["device"]);
    }

    if (desc.contains("app")) {
        if (extern_globals_.lookup(continuation->name())) {
            world().make_internal(continuation);
        }
        auto args = get_arglist(desc["app"]["args"]);
        auto callee = get_def(desc["app"]["target"]);
        continuation->jump(callee, args);
    }

    if (desc.contains("intrinsic")) {
        if (desc["intrinsic"].get<std::string>() == "branch" || desc["intrinsic"].get<std::string>() == "match") {
            //pass
        } else {
            continuation->set_name(desc["intrinsic"]);
            continuation->set_intrinsic();
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
        assert(false);
}

const thorin::Def * IRBuilder::build_ArithOp (json desc) {
    auto args = get_arglist(desc["args"]);
    auto tag = resolve_arithop_tag(desc["op"]);

    assert(args.size() == 2);

    return world().arithop(tag, args[0], args[1]);
}

thorin::MathOpTag IRBuilder::resolve_mathop_tag (std::string mathop_tag) {
    static const std::map<std::string, thorin::MathOpTag> MathOpMap {
#define THORIN_MATHOP(OP) {#OP, thorin::MathOpTag::MathOp_##OP},
#include <thorin/tables/mathoptable.h>
    };

    auto it = MathOpMap.find(mathop_tag);
    if (it != MathOpMap.end())
        return it->second;
    else
        assert(false);
}

const thorin::Def * IRBuilder::build_MathOp (json desc) {
    auto args = get_arglist(desc["args"]);
    auto tag = resolve_mathop_tag(desc["op"]);

    return world().mathop(tag, args);
}

const thorin::Def * IRBuilder::build_LEA (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 2);

    return world().lea(args[0], args[1], {});
}

const thorin::Def * IRBuilder::build_Load (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 2);

    return world().load(args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Extract (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 2);

    return world().extract(args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Insert (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 3);

    return world().insert(args[0], args[1], args[2]);
}

const thorin::Def * IRBuilder::build_Cast (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);
    auto source = get_def(desc["source"]);

    return world().cast(target_type, source);
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

    return world().cmp(tag, args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Run (json desc) {
    auto target = get_def(desc["target"]);

    return world().run(target);
}

const thorin::Def * IRBuilder::build_Hlt (json desc) {
    auto target = get_def(desc["target"]);

    return world().hlt(target);
}

const thorin::Def * IRBuilder::build_Store (json desc) {
    auto args = get_arglist(desc["args"]);

    assert(args.size() == 3);

    return world().store(args[0], args[1], args[2]);
}

const thorin::Def * IRBuilder::build_Enter (json desc) {
    auto mem = get_def(desc["mem"]);

    return world().enter(mem);
}

const thorin::Def * IRBuilder::build_Slot (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);
    auto frame = get_def(desc["frame"]);

    return world().slot(target_type, frame);
}

const thorin::Def * IRBuilder::build_Bitcast (json desc) {
    auto target_type = typetable_.get_type(desc["target_type"]);
    auto source = get_def(desc["source"]);

    return world().bitcast(target_type, source);
}

const thorin::Def * IRBuilder::build_IndefiniteArray (json desc) {
    auto elem_type = typetable_.get_type(desc["elem_type"]);
    auto dim = get_def(desc["dim"]);

    return world().indefinite_array(elem_type, dim);
}

const thorin::Def * IRBuilder::build_DefiniteArray (json desc) {
    auto elem_type = typetable_.get_type(desc["elem_type"]);
    auto args = get_arglist(desc["args"]);

    return world().definite_array(elem_type, args);
}

const thorin::Def * IRBuilder::build_Global (json desc) {
    bool is_mutable = desc["mutable"];
    auto init = get_def(desc["init"]);

    thorin::Global* def = nullptr;

    if (desc.contains("external")) {
        def = extern_globals_.lookup(desc["external"]).value_or(nullptr)->as<thorin::Global>();
        if (!def) {
            def = const_cast<thorin::Global*>(world().global(init, is_mutable)->as<thorin::Global>());
            def->set_name(desc["external"]);
            world().make_external(def);
            extern_globals_.emplace(def->name(), def);
        } else if (def->init()->isa<thorin::Bottom>() && !init->isa<thorin::Bottom>()) {
            def->set_init(init);
        } else if (!def->init()->isa<thorin::Bottom>() && !init->isa<thorin::Bottom>()) {
            std::cerr << "Warning: multiple definitions of global variable" << std::endl;
            def->set_init(init);
        }
    } else {
        def = const_cast<thorin::Global*>(world().global(init, is_mutable)->as<thorin::Global>());
    }

    return def;
}

const thorin::Def * IRBuilder::build_Closure (json desc) {
    auto args = get_arglist(desc["args"]);
    auto closure_type = typetable_.get_type(desc["closure_type"])->as<thorin::ClosureType>();

    assert(args.size() == 2);

    return world().closure(closure_type, args[0], args[1]);
}

const thorin::Def * IRBuilder::build_Struct (json desc) {
    auto args = get_arglist(desc["args"]);
    auto struct_type = typetable_.get_type(desc["struct_type"])->as<thorin::StructType>();

    return world().struct_agg(struct_type, args);
}

const thorin::Def * IRBuilder::build_VectorLift (json desc) {
    auto args = get_arglist(desc["args"]);
    assert(args.size() == 1);

    auto target_type = typetable_.get_type(desc["vector_type"]);

    auto vector_type = target_type->as<thorin::VectorContainerType>();
    return world().vector_lift(vector_type, args[0]);
}

const thorin::Def * IRBuilder::build_Tuple (json desc) {
    auto args = get_arglist(desc["args"]);

    return world().tuple(args);
}

const thorin::Def * IRBuilder::build_Vector (json desc) {
    auto args = get_arglist(desc["args"]);

    return world().vector(args);
}

const thorin::Def * IRBuilder::build_Filter (json desc) {
    auto args = get_arglist(desc["args"]);

    return world().filter(args);
}

const thorin::Def * IRBuilder::build_Variant (json desc) {
    auto variant_type = typetable_.get_type(desc["variant_type"])->as<thorin::VariantType>();
    auto value = get_def(desc["value"]);
    size_t index = desc["index"];

    return world().variant(variant_type, value, index);
}

const thorin::Def * IRBuilder::build_Assembly (json desc) {
    auto asm_type = typetable_.get_type(desc["asm_type"]);
    auto inputs = get_arglist(desc["inputs"]);
    std::string asm_template = desc["asm_template"];

    thorin::Array<std::string> out_constraints(desc["output_constraints"].size());
    for (size_t i = 0; i < desc["output_constraints"].size(); ++i) {
        out_constraints[i] = desc["output_constraints"][i];
    }
    thorin::Array<std::string> in_constraints(desc["input_constraints"].size());
    for (size_t i = 0; i < desc["input_constraints"].size(); ++i) {
        in_constraints[i] = desc["input_constraints"][i];
    }
    thorin::Array<std::string> clobbers(desc["clobbers"].size());
    for (size_t i = 0; i < desc["clobbers"].size(); ++i) {
        clobbers[i] = desc["clobbers"][i];
    }

    static const std::map<std::string, thorin::Assembly::Flags> FlagsMap {
        {"noflag", thorin::Assembly::Flags::NoFlag},
        {"hassideeffects", thorin::Assembly::Flags::HasSideEffects},
        {"isalignstack", thorin::Assembly::Flags::IsAlignStack},
        {"isinteldialect", thorin::Assembly::Flags::IsIntelDialect}
    };

    auto flags = thorin::Assembly::Flags::NoFlag;
    auto it = FlagsMap.find(desc["flags"]);
    if (it != FlagsMap.end())
        flags = it->second;
    else {
        std::cerr << "Unknown flag type: " << desc["flags"] << std::endl;
        assert(false);
    }

    return world().assembly(asm_type, inputs, asm_template, out_constraints, in_constraints, clobbers, flags);
}

const thorin::Def * IRBuilder::build_VariantExtract (json desc) {
    auto value = get_def(desc["value"]);
    size_t index = desc["index"];

    return world().variant_extract(value, index);
}

const thorin::Def * IRBuilder::build_VariantIndex (json desc) {
    auto value = get_def(desc["value"]);

    return world().variant_index(value);
}

const thorin::Def * IRBuilder::reconstruct_def(json desc) {
    const thorin::Def* return_def = nullptr;
    switch (resolvedef(desc["type"])) {
#define CASE(NAME, CLASS) case DefType::CLASS: { return_def = build_##CLASS(desc); break; }
    DefTypeEnum(CASE)
#undef CASE
    default:
        std::cerr << "Def is invalid" << std::endl;
        std::cerr << desc["name"] << std::endl;
    }
    assert(return_def);
    return known_defs[desc["name"]] = return_def;
}
