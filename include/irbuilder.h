#ifndef IRBUILDER_H
#define IRBUILDER_H

#include "typetable.h"

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
        InvalidDef,
        Continuation,
        Constant,
    };

    DefType resolvedef (std::string def_type);

    thorin::Array<const thorin::Def*> get_arglist (json arg_list);

public:
    const thorin::Def * get_def (std::string type_name);
    const thorin::Def * reconstruct_def(json desc);
};

#endif
