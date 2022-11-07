#include "main.h"
#include "typetable.h"
#include "irbuilder.h"

#include<iostream>
#include<fstream>

#include<thorin/world.h>
#include<thorin/be/codegen.h>
#include<thorin/be/llvm/llvm.h>
#include<thorin/be/llvm/cpu.h>

#define DUMP_TYPE

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

    thorin::World world(data["module"].get<std::string>());

    TypeTable table(world);

    for (auto it : data["type_table"]) {
        const thorin::Type* type = table.reconstruct_type(it);
#ifdef DUMP_TYPE
        type->dump();
        if (auto nominaltype = type->isa<thorin::NominalType>()) {
            for (auto name : nominaltype->op_names()) {
                std::cout << " " << name;
            }
            std::cout << std::endl;
            for (auto op : nominaltype->ops()) {
                op->dump();
            }
            std::cout << "Nominaltype End" << std::endl;
        }
#endif
    }

    IRBuilder irbuilder(world, table);

    for (auto it : data["defs"]) {
        const thorin::Def* def = irbuilder.reconstruct_def(it);
    }

    world.dump();

    world.opt();

    auto emit_to_file = [&] (thorin::CodeGen& cg) {
        auto name = data["module"].get<std::string>() + cg.file_ext();

        std::ofstream file(name);
        assert(file);

        cg.emit_stream(file);
    };

    std::string a = "";
    std::string b = "";
    std::string c = "";

    thorin::llvm::CPUCodeGen cg(world, 0, "", a, b, c);
    emit_to_file(cg);

    return 0;
}
