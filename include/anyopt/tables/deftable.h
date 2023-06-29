#ifndef TABLES_DEFTABLE_H
#define TABLES_DEFTABLE_H

#define DefTypeEnum(N) \
N("arithop", ArithOp) \
N("mathop", MathOp) \
N("continuation", Continuation) \
N("const", Constant) \
N("top", Top) \
N("bottom", Bottom) \
N("cmp", Cmp) \
N("lea", LEA) \
N("load", Load) \
N("extract", Extract) \
N("insert", Insert) \
N("cast", Cast) \
N("bitcast", Bitcast) \
N("run", Run) \
N("hlt", Hlt) \
N("store", Store) \
N("enter", Enter) \
N("slot", Slot) \
N("def_array", DefiniteArray) \
N("indef_array", IndefiniteArray) \
N("global", Global) \
N("closure", Closure) \
N("struct", Struct) \
N("tuple", Tuple) \
N("vector", Vector) \
N("alloc", Alloc) \
N("release", Release) \
N("known", Known) \
N("sizeof", Sizeof) \
N("alignof", Alignof) \
N("select", Select) \
N("filter", Filter) \
N("variant", Variant) \
N("variant_extract", VariantExtract) \
N("variant_index", VariantIndex) \
N("assembly", Assembly) \

#endif
