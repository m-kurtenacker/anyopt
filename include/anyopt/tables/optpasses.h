#ifndef TABLES_OPTPASSES_H
#define TABLES_OPTPASSES_H

#define ThorinWorldAdapter(pass) [](thorin::Thorin& thorin) { pass(thorin.world()); }

#define OptPassesEnum(N) \
N(Verify, verify, ThorinWorldAdapter(verify)) \
N(Cleanup, cleanup, [](thorin::Thorin& thorin){ thorin.cleanup(); }) \
N(Lower2CFF, lower2cff, lower2cff) \
N(PE, pe, pe) \
N(Mark_PE_Done, mark_pe_done, mark_pe_done) \
N(Flatten_Tuples, flatten_tuples, flatten_tuples) \
N(Split_Slots, split_slots, split_slots) \
N(Closure_Conversion, closure_conversion, ThorinWorldAdapter(closure_conversion)) \
N(Lift_Builtins, lift_builtins, lift_builtins) \
N(Inliner, inliner, inliner) \
N(Hoist_Enters, hoist_enters, hoist_enters) \
N(Dead_Load_Opt, dead_load_opt, ThorinWorldAdapter(dead_load_opt)) \
N(Codegen_Prepare, codegen_prepare, codegen_prepare) \

#endif
