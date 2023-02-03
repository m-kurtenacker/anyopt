#ifndef TABLES_TYPETABLE_H
#define TABLES_TYPETABLE_H

#define TypeTableEnum(N) \
N("def_array", DefiniteArrayType) \
N("indef_array", IndefiniteArrayType) \
N("bottom", BottomType) \
N("function", FnType) \
N("closure", ClosureType) \
N("frame", FrameType) \
N("mem", MemType) \
N("struct", StructType) \
N("variant", VariantType) \
N("tuple", TupleType) \
N("prim", PrimType) \
N("ptr", PtrType) \
N("vector_container", VectorContainerType) \

#define AddrSpaceEnum(N) \
N("generic", Generic) \
N("global", Global) \
N("texture", Texture) \
N("shared", Shared) \
N("constant", Constant) \
N("private", Private) \

#endif
