#ifndef TABLES_OPTPASSES_H
#define TABLES_OPTPASSES_H

#define ThorinWorldAdapter(pass) [](thorin::Thorin& thorin) { pass(thorin.world()); }

#define OptPassesEnum(N) \
N(Cleanup_World, [](thorin::Thorin& thorin){ thorin.cleanup(); }) \
N(PE, pe) \
N(Mark_PE_Done, mark_pe_done) \
N(Flatten_Tuples, flatten_tuples) \
N(Clone_Bodies, ThorinWorldAdapter(clone_bodies)) \
N(Split_Slots, split_slots) \
N(Closure_Conversion, ThorinWorldAdapter(closure_conversion)) \
N(Lift_Builtins, lift_builtins) \
N(Inliner, inliner) \
N(Hoist_Enters, hoist_enters) \
N(Dead_Load_Opt, ThorinWorldAdapter(dead_load_opt)) \
N(Codegen_Prepare, ThorinWorldAdapter(codegen_prepare)) \

#endif
