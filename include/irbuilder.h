#ifndef IRBUILDER_H
#define IRBUILDER_H

#include "typetable.h"

#include<thorin/world.h>
#include<nlohmann/json.hpp>

using json = nlohmann::json;


class IRBuilder {
public:
    IRBuilder(thorin::World& world, TypeTable& typetable) : world_(world), typetable_(typetable) {}

private:
    thorin::World& world_;
    TypeTable& typetable_;

    enum class DefType {
        InvalidDef,
        Continuation,
        Constant,
    };

    DefType resolvedef (std::string def_type);

public:
    const thorin::Def * reconstruct_def(json desc);
};

#endif
