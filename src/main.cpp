#include "anyopt/main.h"
#include "anyopt/typetable.h"
#include "anyopt/irbuilder.h"
#include "anyopt/tables/optpasses.h"

#include "anyopt/analysis.h"

#include<iostream>
#include<fstream>

#include<thorin/world.h>
#include<thorin/be/codegen.h>
#include<thorin/be/llvm/llvm.h>
#include<thorin/be/llvm/cpu.h>

#include<thorin/be/c/c.h>
#include<thorin/be/json/json.h>

#include<thorin/analyses/verify.h>

#include<thorin/transform/closure_conversion.h>
#include<thorin/transform/codegen_prepare.h>
#include<thorin/transform/dead_load_opt.h>
#include<thorin/transform/flatten_tuples.h>
#include<thorin/transform/hoist_enters.h>
#include<thorin/transform/inliner.h>
#include<thorin/transform/lift_builtins.h>
#include<thorin/transform/partial_evaluation.h>
#include<thorin/transform/plugin_execute.h>
#include<thorin/transform/split_slots.h>

using namespace anyopt;

static void usage() {
    std::cout << "usage: anyopt [options] files...\n"
                "options:\n"
                "  -h     --help                 Displays this message\n"
                "         --version              Displays the version number\n"
                "         --no-color             Disables colors in error messages\n"
                "         --emit-thorin          Prints the Thorin IR after code generation\n"
                "         --emit-json            Emits Thorin IR in the output file\n"
                "         --emit-c-interface     Emits C interface for exported functions and imported types\n"
                "         --log-level <lvl>      Changes the log level in Thorin (lvl = debug, verbose, info, warn, or error, defaults to error)\n"
                "         --tab-width <n>        Sets the width of the TAB character in error messages or when printing the AST (in spaces, defaults to 2)\n"
                "         --emit-c               Emits C code in the output file\n"
                "         --emit-llvm            Emits LLVM IR in the output file\n"
                "         --plugin <file>        Load additional code transformations as plugins\n"
                "  -On                           Sets the optimization level (n = 0, 1, 2, or 3, defaults to 0)\n"
                "  -p     --pass                 Manually supply passes that are going to be executed. Passes are:\n"
#define MAP(CLASS, ALIAS, PASS) "                                   " #ALIAS "\n"
            OptPassesEnum(MAP)
#undef MAP
                "         --remove-interns       Turn all internal continuations into normal thorin continuations.\n"
                "         --keep-intern <name>   Keep a specific intern by name. This implies --remove-interns.\n"
                "  -s     --scope                Compute scope of a given continuation and print the names of all definitions that belong to it.\n"
                "         --passes               Displays the normal optimization pass chain\n"
                "  -o <name>                     Sets the module name (defaults to the first file name without its extension)\n"
                ;
}

static void version() {
    static const char day[] = { __DATE__[4] == ' ' ? '0' : __DATE__[4], __DATE__[5], 0 };
    static const char* month =
        (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n') ? "01" :
        (__DATE__[0] == 'F' && __DATE__[1] == 'e' && __DATE__[2] == 'b') ? "02" :
        (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ? "03" :
        (__DATE__[0] == 'A' && __DATE__[1] == 'p' && __DATE__[2] == 'r') ? "04" :
        (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ? "05" :
        (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ? "06" :
        (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ? "07" :
        (__DATE__[0] == 'A' && __DATE__[1] == 'u' && __DATE__[2] == 'g') ? "08" :
        (__DATE__[0] == 'S' && __DATE__[1] == 'e' && __DATE__[2] == 'p') ? "09" :
        (__DATE__[0] == 'O' && __DATE__[1] == 'c' && __DATE__[2] == 't') ? "10" :
        (__DATE__[0] == 'N' && __DATE__[1] == 'o' && __DATE__[2] == 'v') ? "11" :
        (__DATE__[0] == 'D' && __DATE__[1] == 'e' && __DATE__[2] == 'c') ? "12" :
        "??";
    static const char year[] = { __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10], 0 };
#ifdef NDEBUG
    static const char* build = "Release";
#else
    static const char* build = "Debug";
#endif
    std::cout << "anyopt " << ANYOPT_VERSION_MAJOR << "." << ANYOPT_VERSION_MINOR << " "
             << year << "-" << month << "-" << day
             <<  " (" << build << ")\n";
}

static void passes() {
    std::cout << "--pass cleanup "
              << "--pass lower2cff "
              << "--pass flatten_tuples "
              << "--pass split_slots "
              << "--pass closure_conversion "
              << "--pass lift_builtins "
              << "--pass inliner "
              << "--pass hoist_enters "
              << "--pass dead_load_opt "
              << "--pass cleanup "
              << "--pass codegen_prepare"
              << "\n";
}

enum OptimizerPass {
#define MAP(CLASS, ALIAS, PASS) CLASS,
OptPassesEnum(MAP)
#undef MAP
};

struct ProgramOptions {
    std::vector<std::string> files;
    std::vector<OptimizerPass> optimizer_passes;

    bool remove_interns = false;
    std::set<std::string> keep_interns;

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
    std::string compute_scope;
    bool show_implicit_casts = false;
    unsigned opt_level = 0;
    size_t max_errors = 0;
    size_t tab_width = 2;
    thorin::LogLevel log_level = thorin::LogLevel::Error;
    std::vector<std::string> plugin_files;

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
                    usage();
                    exit = true;
                    return true;
                } else if (matches(argv[i], "--passes")) {
                    passes();
                    exit = true;
                    return true;
                } else if (matches(argv[i], "--version")) {
                    version();
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
                } else if (matches(argv[i], "--plugin")) {
                    plugin_files.push_back(argv[++i]);
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
                } else if (matches(argv[i], "-p", "--pass")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    i++;
                    using namespace std::string_literals;
                    if (false) {}
#define MAP(CLASS, ALIAS, PASS) else if (argv[i] == #ALIAS##s) { optimizer_passes.push_back(CLASS); }
                    OptPassesEnum(MAP)
#undef MAP
                    else {
                        std::cerr << "Did not recognize pass \"" << argv[i] << "\"" << std::endl;
                        return false;
                    }
                } else if (matches(argv[i], "--remove-interns")) {
                    remove_interns = true;
                } else if (matches(argv[i], "--keep-intern")) {
                    remove_interns = true;
                    if (!check_arg(argc, argv, i))
                        return false;
                    keep_interns.insert(argv[++i]);
                } else if (matches(argv[i], "-s", "--scope")) {
                    if (!check_arg(argc, argv, i))
                        return false;
                    compute_scope = argv[++i];
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

void pe (thorin::Thorin& thorin) {
    while(partial_evaluation(thorin, false));
}

void lower2cff (thorin::Thorin& thorin) {
    while(partial_evaluation(thorin, true));
}

void mark_pe_done (thorin::Thorin& thorin) {
    thorin.world().mark_pe_done();
}

int main (int argc, char** argv) {
    ProgramOptions opts;
    if (!opts.parse(argc, argv))
        return EXIT_FAILURE;
    if (opts.exit)
        return EXIT_SUCCESS;

    if (opts.files.empty()) {
        std::cerr << "no input files" << std::endl;
        return EXIT_FAILURE;
    }

    if (opts.module_name == "") {
        std::ifstream json_input_file(opts.files[0]);
        json data = json::parse(json_input_file);
        opts.module_name = data["module"].get<std::string>();
    }

    thorin::Thorin thorin(opts.module_name);
    thorin.world().set(opts.log_level);
    thorin.world().set(std::make_shared<thorin::Stream>(std::cerr));

#ifdef THORIN_ENABLE_RLIMITS
    thorin.ensure_stack_size(512L * 1024L * 1024L);
#endif

    thorin::World::Externals extern_globals;
    std::map<std::string, const thorin::Type*> global_variant_types;

    for (auto filename : opts.files) {
        std::ifstream json_input_file(filename);
        json data = json::parse(json_input_file);

        if (data.contains("host_triple")) {
            if (opts.host_triple == "") {
                opts.host_cpu = data["host_triple"];
            } else if (opts.host_triple != data["host_triple"]) {
                std::cerr << "Warning: Previously supplied host triple is different from the one in " << filename << std::endl;
                opts.host_cpu = data["host_triple"];
            }
        }
        if (data.contains("host_cpu")) {
            if (opts.host_cpu == "") {
                opts.host_cpu = data["host_cpu"];
            } else if (opts.host_cpu != data["host_cpu"]) {
                std::cerr << "Warning: Previously supplied host cpu is different from the one in " << filename << std::endl;
                opts.host_cpu = data["host_cpu"];
            }
        }
        if (data.contains("host_attr")) {
            if (opts.host_attr == "") {
                opts.host_cpu = data["host_attr"];
            } else if (opts.host_attr != data["host_attr"]) {
                std::cerr << "Warning: Previously supplied host attributes is different from the one in " << filename << std::endl;
                opts.host_cpu = data["host_attr"];
            }
        }

        if (opts.module_name == "")
            opts.module_name = data["module"].get<std::string>();

        TypeTable table(thorin, global_variant_types);
        for (auto it : data["type_table"])
            table.reconstruct_type(it);

        IRBuilder irbuilder(thorin, table, extern_globals, opts.remove_interns, opts.keep_interns);
        for (auto it : data["defs"])
            irbuilder.reconstruct_def(it);

        if (opts.compute_scope != "") {
            print_scope_analysis(irbuilder, opts.compute_scope);
        }
    }

    for (auto plugin_to_load : opts.plugin_files) {
        thorin.register_plugin(plugin_to_load.c_str());
    }

    for (auto pass : opts.optimizer_passes) {
        switch (pass) {
#define MAP(CLASS, ALIAS, PASS) case CLASS: std::cerr << #ALIAS << std::endl; PASS(thorin); break;
            OptPassesEnum(MAP)
#undef MAP
        }
    }

    if (opts.optimizer_passes.empty() && opts.opt_level == 1)
        thorin.cleanup();
    if (opts.emit_c_int) {
        auto name = opts.module_name + ".h";
        std::ofstream file(name);
        if (!file)
            std::cerr << "cannot open '" << name << "' for writing" << std::endl;
        else {
            thorin::Stream stream(file);
            thorin::c::emit_c_int(thorin, stream);
        }
    }

    if (opts.optimizer_passes.empty() && (opts.opt_level > 1 || opts.emit_c || opts.emit_llvm))
        thorin.opt();
    if (opts.emit_thorin)
        thorin.world().dump_scoped();

    if (opts.emit_json || opts.emit_c || opts.emit_llvm) {
        auto emit_to_file = [&] (thorin::CodeGen& cg) {
            auto name = opts.module_name + cg.file_ext();
            std::ofstream file(name);
            if (!file)
                std::cerr << "cannot open '" << name << "' for writing" << std::endl;
            else
                cg.emit_stream(file);
        };
        if (opts.emit_json) {
            thorin::json::CodeGen cg(thorin, opts.debug, opts.host_triple, opts.host_cpu, opts.host_attr);
            emit_to_file(cg);
        }
        if (opts.emit_c || opts.emit_llvm) {
            thorin::DeviceBackends backends(thorin.world(), opts.opt_level, opts.debug, opts.hls_flags);
            if (opts.emit_c) {
                thorin::Cont2Config kernel_configs;
                thorin::c::CodeGen cg(thorin, kernel_configs, thorin::c::Lang::C99, opts.debug, opts.hls_flags);
                emit_to_file(cg);
            }
            if (opts.emit_llvm) {
                thorin::llvm::CPUCodeGen cg(thorin, opts.opt_level, opts.debug, opts.host_triple, opts.host_cpu, opts.host_attr);
                emit_to_file(cg);
            }
            for (auto& cg : backends.cgs) {
                if (cg) {
                    std::cerr << "AnyOpt Codegen " << cg->file_ext() << std::endl;
                    emit_to_file(*cg);
                }
            }
        }
    }

    return 0;
}
