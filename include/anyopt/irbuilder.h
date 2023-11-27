#ifndef IRBUILDER_H
#define IRBUILDER_H

#include "typetable.h"

#include "anyopt/tables/deftable.h"

#include<thorin/world.h>
#include<nlohmann/json.hpp>
#include<map>
#include<set>

using json = nlohmann::json;

namespace anyopt {

class IRBuilder {
public:
    IRBuilder(thorin::Thorin& thorin, TypeTable& typetable, thorin::World::Externals& extern_globals, bool remove_interns, std::set<std::string>& keep_interns) : thorin_(thorin), typetable_(typetable), extern_globals_(extern_globals), remove_interns_(remove_interns), keep_interns_(keep_interns) {}

private:
    thorin::Thorin& thorin_;
    TypeTable& typetable_;
    thorin::World::Externals& extern_globals_;

    bool remove_interns_;
    std::set<std::string>& keep_interns_;

    std::map<std::string, const thorin::Def*> known_defs;

    enum class DefType {
#define ID(_, A) A,
        DefTypeEnum(ID)
#undef ID
    };

    DefType resolvedef (std::string def_type);
    thorin::ArithOpTag resolve_arithop_tag (std::string arithop_tag);
    thorin::MathOpTag resolve_mathop_tag (std::string mathop_tag);
    thorin::CmpTag resolve_cmp_tag (std::string cmp_tag);

    thorin::Array<const thorin::Def*> get_arglist (json arg_list);

    thorin::World& world() { return thorin_.world(); }

#define CreateFunction(NAME, CLASS) const thorin::Def* build_##CLASS (json desc);
    DefTypeEnum(CreateFunction)
#undef CreateFunction

public:
    std::map<std::string, const thorin::Def*>::iterator begin() { return known_defs.begin(); }
    std::map<std::string, const thorin::Def*>::iterator end() { return known_defs.end(); }

    const thorin::Def * get_def (std::string type_name);
    const thorin::Def * reconstruct_def(json desc);
};

}

#endif
