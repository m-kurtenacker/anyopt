#include "irbuilder.h"

IRBuilder::DefType IRBuilder::resolvedef (std::string def_type) {
    static const std::map<std::string, DefType> TypeMap {
        {"continuation", DefType::Continuation},
        {"const", DefType::Constant},
    };

    auto it = TypeMap.find(def_type);
    if (it != TypeMap.end())
        return it->second;
    else
        return DefType::InvalidDef;
}

const thorin::Def * IRBuilder::reconstruct_def(json desc) {
    const thorin::Def* return_def = nullptr;
    switch (resolvedef(desc["type"])) {
    case DefType::Continuation: {
        std::cerr << "Building continuation named " << desc["name"] << std::endl;
        break;
    }
    case DefType::Constant: {
        std::cerr << "Building constant named " << desc["name"] << std::endl;
        auto const_type = typetable_.get_type(desc["const_type"].get<std::string>());
        auto primtype = const_type->as<thorin::PrimType>();
        thorin::Box value(desc["value"].get<thorin::s64>());
        const thorin::Def* literal = world_.literal(primtype->primtype_tag(), value, {}, primtype->length());
        literal->dump();
        break;
    }
    default:
        std::cerr << "Def is invalid" << std::endl;
    }
    return nullptr;
}
