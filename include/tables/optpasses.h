#ifndef TABLES_OPTPASSES_H
#define TABLES_OPTPASSES_H

#define OptPassesEnum(N) \
N(Cleanup_World, cleanup_world) \
N(PE, pe) \
N(Flatten_Tuples, flatten_tuples) \
N(Clone_Bodies, clone_bodies) \
N(Split_Slots, split_slots) \
N(Closure_Conversion, closure_conversion) \
N(Lift_Builtins, lift_builtins) \
N(Inliner, inliner) \
N(Hoist_Enters, hoist_enters) \
N(Dead_Load_Opt, dead_load_opt) \
N(Codegen_Prepare, codegen_prepare) \

#endif
