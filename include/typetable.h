#ifndef TYPE_TABLE_H
#define TYPE_TABLE_H

#include "tables/typetable.h"

#include<thorin/world.h>
#include<nlohmann/json.hpp>
#include<map>

using json = nlohmann::json;

class TypeTable {
public:
    TypeTable(thorin::Thorin& thorin) : thorin_(thorin) {}

private:
    thorin::Thorin& thorin_;

    std::map<std::string, const thorin::Type*> known_types;

    enum class OptiType {
#define ID(_, A) A,
        TypeTableEnum(ID)
#undef ID
    };

    OptiType resolvetype (std::string type_name);
    thorin::PrimTypeTag resolvetag (std::string type_tag);
    thorin::AddrSpace resolveaddrspace (std::string addr_space);
    thorin::Array<const thorin::Type*> get_arglist (json arg_list);

    thorin::World& world() { return thorin_.world(); }

#define CreateFunction(NAME, CLASS) const thorin::Type* build_##CLASS (json desc);
    TypeTableEnum(CreateFunction)
#undef CreateFunction

public:
    const thorin::Type * get_type(std::string type_name);
    const thorin::Type * reconstruct_type(json desc);
};

#endif
