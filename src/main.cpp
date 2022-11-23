#include "main.h"
#include "typetable.h"
#include "irbuilder.h"

#include<iostream>
#include<fstream>

#include<thorin/world.h>
#include<thorin/be/codegen.h>
#include<thorin/be/llvm/llvm.h>
#include<thorin/be/llvm/cpu.h>

#include<thorin/be/json/json.h>

#include<thorin/transform/partial_evaluation.h>

//#define DUMP

struct ProgramOptions {
    std::vector<std::string> files;
    std::string module_name;
    bool exit = false;
    bool no_color = false;
    bool warns_as_errors = false;
    bool enable_all_warns = false;
    bool debug = false;
    bool print_ast = false;
    bool emit_thorin = false;
    bool emit_c_int = false;
    bool emit_c = false;
    bool emit_json = false;
    bool emit_llvm = false;
    std::string host_triple;
    std::string host_cpu;
    std::string host_attr;
    std::string hls_flags;
    bool show_implicit_casts = false;
    unsigned opt_level = 0;
    size_t max_errors = 0;
    size_t tab_width = 2;
    thorin::LogLevel log_level = thorin::LogLevel::Error;

    bool matches(const char* arg, const char* opt) {
        return !strcmp(arg, opt);
    }

    bool matches(const char* arg, const char* opt1, const char* opt2) {
        return !strcmp(arg, opt1) || !strcmp(arg, opt2);
    }

    bool check_arg(int argc, char** argv, int i) {
        if (i + 1 >= argc) {
            return false;
        }
        return true;
    }

    bool parse(int argc, char** argv) {
        if (argc < 2) {
            return false;
        }

        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                if (matches(argv[i], "-h", "--help")) {
                    exit = true;
                    return true;
                } else if (matches(argv[i], "--version")) {
                    exit = true;
                    return true;
                } else if (matches(argv[i], "--no-color")) {
                    no_color = true;
                } else if (matches(argv[i], "-Wall", "--enable-all-warnings")) {
                    enable_all_warns = true;
                } else if (matches(argv[i], "-Werror", "--warnings-as-errors")) {
                    warns_as_errors = true;
                } else if (matches(argv[i], "--max-errors")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    max_errors = std::strtoull(argv[++i], NULL, 10);
                    if (max_errors == 0) {
                        return false;
                    }
                } else if (matches(argv[i], "-g", "--debug")) {
                    debug = true;
                } else if (matches(argv[i], "--print-ast")) {
                    print_ast = true;
                } else if (matches(argv[i], "--show-implicit-casts")) {
                    show_implicit_casts = true;
                } else if (matches(argv[i], "--emit-thorin")) {
                    emit_thorin = true;
                } else if (matches(argv[i], "--emit-json")) {
                    emit_json = true;
                } else if (matches(argv[i], "--emit-c-interface")) {
                    emit_c_int = true;
                } else if (matches(argv[i], "--log-level")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    i++;
                    using namespace std::string_literals;
                    if (argv[i] == "debug"s)
                        log_level = thorin::LogLevel::Debug;
                    else if (argv[i] == "verbose"s)
                        log_level = thorin::LogLevel::Verbose;
                    else if (argv[i] == "info"s)
                        log_level = thorin::LogLevel::Info;
                    else if (argv[i] == "warn"s)
                        log_level = thorin::LogLevel::Warn;
                    else if (argv[i] == "error"s)
                        log_level = thorin::LogLevel::Error;
                    else {
                        return false;
                    }
                } else if (matches(argv[i], "--tab-width")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    tab_width = std::strtoull(argv[++i], NULL, 10);
                } else if (matches(argv[i], "--emit-llvm")) {
                    emit_llvm = true;
                } else if (matches(argv[i], "--emit-c")) {
                    emit_c = true;
                } else if (matches(argv[i], "--host-triple")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    host_triple = argv[++i];
                } else if (matches(argv[i], "--host-cpu")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    host_cpu = argv[++i];
                } else if (matches(argv[i], "--host-attr")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    host_attr = argv[++i];
                } else if (matches(argv[i], "--hls-flags")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    hls_flags = argv[++i];
                } else if (matches(argv[i], "-O0")) {
                    opt_level = 0;
                } else if (matches(argv[i], "-O1")) {
                    opt_level = 1;
                } else if (matches(argv[i], "-O2")) {
                    opt_level = 2;
                } else if (matches(argv[i], "-O3")) {
                    opt_level = 3;
                } else if (matches(argv[i], "-o")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    module_name = argv[++i];
                } else {
                    return false;
                }
            } else
                files.push_back(argv[i]);
        }

        return true;
    }
};

int main (int argc, char** argv) {
    ProgramOptions opts;
    opts.parse(argc, argv);

    auto filename = opts.files[0];

    std::ifstream json_input_file (filename);
    json data = json::parse(json_input_file);

#ifdef DUMP
    std::cout << data["module"].get<std::string>() << std::endl;

    json test = data["defs"];
    for (auto t : test) {
        std::cout << t["name"].get<std::string>() << std::endl;
    }
#endif

    thorin::World world(data["module"].get<std::string>());

    TypeTable table(world);

    for (auto it : data["type_table"]) {
        const thorin::Type* type = table.reconstruct_type(it);
#ifdef DUMP
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
#ifdef DUMP
        std::cerr << "Reconst of " << it["name"]  << ":" << std::endl;
#endif
        const thorin::Def* def = irbuilder.reconstruct_def(it);
#ifdef DUMP
        def->dump();
#endif
    }

#ifdef DUMP
    world.dump();
#endif

    if (opts.emit_thorin) {
        world.cleanup();
        world.dump();
    }

    auto emit_to_file = [&] (thorin::CodeGen& cg) {
        auto name = data["module"].get<std::string>() + cg.file_ext();

        std::ofstream file(name);
        assert(file);

        cg.emit_stream(file);
    };

    if (opts.emit_llvm) {
        world.opt();

        std::string a = "";
        std::string b = "";
        std::string c = "";
        thorin::llvm::CPUCodeGen cg(world, 0, "", a, b, c);

        emit_to_file(cg);
    }

    if (opts.emit_json) {
        thorin::Cont2Config kernel_configs;
        thorin::json::CodeGen cg(world, kernel_configs, false);

        emit_to_file(cg);
    }

    return 0;
}
