#ifndef TYPE_TABLE_H
#define TYPE_TABLE_H

#include<thorin/world.h>
#include<nlohmann/json.hpp>
#include<map>

using json = nlohmann::json;


class TypeTable {
public:
    TypeTable(thorin::World& world) : world_(world) {}

private:
    thorin::World& world_;

    //map alias -> thorin::Type
    std::map<std::string, const thorin::Type*> known_types;

    enum class OptiType {
        InvalidType,
        DefiniteArrayType,
        IndefiniteArrayType,
        BottomType,
        FnType,
        ClosureType,
        FrameType,
        MemType,
        StructType,
        VariantType,
        TupleType,
        PrimType,
        PtrType,
    };

    OptiType resolvetype (std::string type_name);
    thorin::PrimTypeTag resolvetag (std::string type_tag);
    thorin::Array<const thorin::Type*> get_arglist (json arg_list);

public:
    const thorin::Type * get_type(std::string type_name);
    const thorin::Type * reconstruct_type(json desc);
};

#endif
