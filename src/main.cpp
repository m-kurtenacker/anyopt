#include "main.h"

#include<iostream>
#include<fstream>
#include<map>

#include<thorin/world.h>

class TypeTable {
public:
    TypeTable(thorin::World& world) : world_(world) {}

private:
    thorin::World& world_;
public:
    enum class OptiType {
        InvalidType,
        FnType,
        MemType,
    };

    OptiType resolvetype (std::string type_name) {
        static const std::map<std::string, OptiType> TypeMap {
            {"fn", OptiType::FnType},
            {"mem", OptiType::MemType},
        };

        auto it = TypeMap.find(type_name);
        if (it != TypeMap.end())
            return it->second;
        else
            return OptiType::InvalidType;
    }

    //map alias -> thorin::Type
    std::map<std::string, const thorin::Type*> known_types;

    const thorin::Type * reconstruct_type(json desc) {
        const thorin::Type* return_type;
        switch (resolvetype(desc["type"])) {
        case OptiType::FnType: {
            //Get all argument types based on their alias.
            json arg_list = desc["args"];
            thorin::Array<const thorin::Type*> args(arg_list.size());
            for (int argnum = 0; argnum < arg_list.size(); ++argnum) {
                json arg_desc = arg_list[argnum];
                auto it = known_types.find(arg_desc.get<std::string>());
                assert(it != known_types.end() && "Unknown argument type!");
                args[argnum] = it->second;
            }

            return_type = world_.fn_type(args);
            break;
        }
        case OptiType::MemType: {
            return_type = world_.mem_type();
            break;
        }
        default:
            std::cerr << "Type is invalid" << std::endl;
            assert(false);
        }
        return known_types[desc["name"].get<std::string>()] = return_type;
    }
};

int main (int argc, char** argv) {
    assert(argc == 2 && "Exactly one argument needed for now.");

    char * filename = argv[1];

    std::ifstream json_input_file (filename);
    json data = json::parse(json_input_file);

    std::cout << data["module"].get<std::string>() << std::endl;

    json test = data["defs"];

    for (auto t : test) {
        std::cout << t["name"].get<std::string>() << std::endl;
    }

    thorin::World world;

    TypeTable table(world);

    for (auto it : data["type_table"]) {
        const thorin::Type* type = table.reconstruct_type(it);
        type->dump();
    }

    return 0;
}
