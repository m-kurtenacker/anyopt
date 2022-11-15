#ifndef TABLES_DEFTYPE_H
#define TABLES_DEFTYPE_H

#define DefTypeEnum(N) \
N("arithop", ArithOp) \
N("continuation", Continuation) \
N("const", Constant) \
N("cmp", Cmp) \
N("lea", LEA) \
N("load", Load) \
N("extract", Extract) \
N("cast", Cast) \
N("run", Run) \
N("hlt", Hlt) \
N("store", Store) \
N("enter", Enter) \
N("slot", Slot) \

#endif
