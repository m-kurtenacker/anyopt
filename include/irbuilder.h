#ifndef IRBUILDER_H
#define IRBUILDER_H

#include "typetable.h"

#include "tables/deftype.h"

#include<thorin/world.h>
#include<nlohmann/json.hpp>
#include<map>

using json = nlohmann::json;

class IRBuilder {
public:
    IRBuilder(thorin::World& world, TypeTable& typetable) : world_(world), typetable_(typetable) {}

private:
    thorin::World& world_;
    TypeTable& typetable_;

    std::map<std::string, const thorin::Def*> known_defs;

    enum class DefType {
#define ID(_, A) A,
        DefTypeEnum(ID)
#undef ID
    };

    DefType resolvedef (std::string def_type);
    thorin::ArithOpTag resolve_arithop_tag (std::string arithop_tag);

    thorin::Array<const thorin::Def*> get_arglist (json arg_list);

    //const thorin::Def* build_Constant (json desc);
#define CreateFunction(NAME, CLASS) const thorin::Def* build_##CLASS (json desc);
    DefTypeEnum(CreateFunction)
#undef CreateFunction

public:
    const thorin::Def * get_def (std::string type_name);
    const thorin::Def * reconstruct_def(json desc);
};

#endif
