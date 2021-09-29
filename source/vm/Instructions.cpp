/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Common.hpp>

std::map<size_t, std::string> Instruction::InstrToName = {
        { noop, "noop" },
        { aconst_null, "aconst_null" },
        { iconst_m1, "iconst_m1" },
        { iconst_0, "iconst_0" },
        { iconst_1, "iconst_1" },
        { iconst_2, "iconst_2" },
        { iconst_3, "iconst_3" },
        { iconst_4, "iconst_4" },
        { iconst_5, "iconst_5" },
        
        { lconst_0, "lconst_0" },
        { lconst_1, "lconst_1" },
        
        { fconst_0, "fconst_0" },
        { fconst_1, "fconst_1" },
        { fconst_2, "fconst_2" },
        
        { dconst_0, "dconst_0" },
        { dconst_1, "dconst_1" },
        
        { bipush, "bipush" },
        { sipush, "sipush" },
        
        { ldc, "ldc" },
        { ldc_w, "ldc_w" },
        { ldc2_w, "ldc2_w" },
        
        { iload, "iload" },
        { lload, "lload" },
        { fload, "fload" },
        { dload, "dload" },
        { aload, "aload" },

        { iload_0, "iload_0" },
        { iload_1, "iload_1" },
        { iload_2, "iload_2" },
        { iload_3, "iload_3" },
        
        { lload_0, "lload_0" },
        { lload_1, "lload_1" },
        { lload_2, "lload_2" },
        { lload_3, "lload_3" },
        
        { fload_0, "fload_0" },
        { fload_1, "fload_1" },
        { fload_2, "fload_2" },
        { fload_3, "fload_3" },
        
        { dload_0, "dload_0" },
        { dload_1, "dload_1" },
        { dload_2, "dload_2" },
        { dload_3, "dload_3" },
        
        { aload_0, "aload_0" },
        { aload_1, "aload_1" },
        { aload_2, "aload_2" },
        { aload_3, "aload_3" },
        
        { iaload, "iaload" },
        { laload, "laload" },
        { faload, "faload" },
        { daload, "daload" },
        { aaload, "aaload" },
        { baload, "baload" },
        { caload, "caload" },
        { saload, "saload" },
        
        { istore, "istore" },
        { lstore, "lstore" },
        { fstore, "fstore" },
        { dstore, "dstore" },
        { astore, "astore" },
        
        { istore_0, "istore_0" },
        { istore_1, "istore_1" },
        { istore_2, "istore_2" },
        { istore_3, "istore_3" },
        
        { lstore_0, "lstore_0" },
        { lstore_1, "lstore_1" },
        { lstore_2, "lstore_2" },
        { lstore_3, "lstore_3" },
        
        { fstore_0, "fstore_0" },
        { fstore_1, "fstore_1" },
        { fstore_2, "fstore_2" },
        { fstore_3, "fstore_3" },
        
        { dstore_0, "dstore_0" },
        { dstore_1, "dstore_1" },
        { dstore_2, "dstore_2" },
        { dstore_3, "dstore_3" },
        
        { astore_0, "astore_0" },
        { astore_1, "astore_1" },
        { astore_2, "astore_2" },
        { astore_3, "astore_3" },
        
        { iastore, "iastore" },
        { lastore, "lastore" },
        { fastore, "fastore" },
        { dastore, "dastore" },
        { aastore, "aastore" },
        { bastore, "bastore" },
        { castore, "castore" },
        { sastore, "sastore" },
        
        { pop, "pop" },
        { pop2, "pop2" },
        
        { bcdup, "dup" },
        { dup_x1, "dup_x1" },
        { dup_x2, "dup_x2" },

        { dup2, "dup2" },
        { dup2_x1, "dup2_x1" },
        { dup2_x2, "dup2_x2" },

        { swap, "swap" },

        { iadd, "iadd" },
        { ladd, "ladd" },
        { _fadd, "fadd" },
        { dadd, "dadd" },

        { isub, "isub" },
        { lsub, "lsub" },
        { _fsub, "fsub" },
        { dsub, "dsub" },
        
        { imul, "imul" },
        { lmul, "lmul" },
        { _fmul, "fmul" },
        { dmul, "dmul" },
        
        { _idiv, "idiv" },
        { _ldiv, "ldiv" },
        { _fdiv, "fdiv" },
        { ddiv, "ddiv" },

        { irem, "irem" },
        { lrem, "lrem" },
        { frem, "frem" },
        { _drem, "drem" },
        
        { ineg, "ineg" },
        { lneg, "lneg" },
        { fneg, "fneg" },
        { dneg, "dneg" },

        { ishl, "ishl" },
        { lshl, "lshl" },
        
        { ishr, "ishr" },
        { lshr, "lshr" },
        
        { iushr, "iushr" },
        { lushr, "lushr" },

        { iand, "iand" },
        { land, "land" },
        
        { ior, "ior" },
        { lor, "lor" },
        
        { ixor, "ixor" },
        { lxor, "lxor" },
        
        { iinc, "iinc" },

        { i2l, "i2l" },
        { i2f, "i2f" },
        { i2d, "i2d" },
        { l2i, "l2i" },
        { l2f, "l2f" },
        { l2d, "l2d" },

        { f2i, "f2i" },
        { f2l, "f2l" },
        { f2d, "f2d" },
        
        { d2i, "d2i" },
        { d2l, "d2l" },
        { d2f, "d2f" },
        
        { i2b, "i2b" },
        { i2c, "i2c" },
        { i2s, "i2s" },

        { lcmp, "lcmp" },
        { fcmpl, "fcmpl" },
        { fcmpg, "fcmpg" },
        { dcmpl, "dcmpl" },
        { dcmpg, "dcmpg" },

        { ifeq, "ifeq" },
        { ifne, "ifne" },
        { iflt, "iflt" },
        { ifge, "ifge" },
        { ifgt, "ifgt" },
        { ifle, "ifle" },
        
        { if_icmpeq, "if_icmpeq" },
        { if_icmpne, "if_icmpne" },
        { if_icmplt, "if_icmplt" },
        { if_icmpge, "if_icmpge" },
        { if_icmpgt, "if_icmpgt" },
        { if_icmple, "if_icmple" },
        
        { if_acmpeq, "if_acmpeq" },
        { if_acmpne, "if_acmpne" },

        { _goto, "goto" },

        { jsr, "jsr" },
        { ret, "ret" },

        { tableswitch, "tableswitch" },
        { lookupswitch, "lookupswitch" },

        { ireturn, "ireturn" },
        { lreturn, "lreturn" },
        { freturn, "freturn" },
        { dreturn, "dreturn" },
        { areturn, "areturn" },

        { _return, "return" },

        { getstatic, "getstatic" },
        { putstatic, "putstatic" },

        { getfield, "getfield" },
        { putfield, "putfield" },

        { invokevirtual, "invokevirtual" },
        { invokespecial, "invokespecial" },
        { invokestatic, "invokestatic" },
        { invokeinterface, "invokeinterface" },
        { invokedynamic, "invokedynamic" },

        { _new, "new" },

        { newarray, "newarray" },
        { anewarray, "anewarray" },

        { arraylength, "arraylength" },

        { athrow, "athrow" },

        { checkcast, "checkcast" },

        { instanceof, "instanceof" },

        { monitorenter, "monitorenter" },
        { monitorexit, "monitorexit" },

        { wide, "wide" },

        { multinewarray, "multinewarray" },

        { ifnull, "ifnull" },
        { ifnonnull, "ifnonnull" },

        { goto_w, "goto_w" },
        { jsr_w, "jsr_w" },

        { breakpoint, "breakpoint"}  
};

std::map<size_t, size_t> Instruction::InstrLengths = {
        { noop, 1 },
        { aconst_null, 1 }, { iconst_m1, 1 }, { iconst_0, 1 }, { iconst_1, 1 }, { iconst_2, 1 }, { iconst_3, 1 }, { iconst_4, 1 }, { iconst_5, 1 },
        { lconst_0, 1 }, { lconst_1, 1 }, { fconst_0, 1 }, { fconst_1, 1 }, { fconst_2, 1 }, { dconst_0, 1 }, { dconst_1, 1 },
        { bipush, 2 }, { sipush, 3 },
        { ldc, 2 }, { ldc_w, 3 }, { ldc2_w, 3 },
        { iload, 2 }, { lload, 2 }, { fload, 2 }, { dload, 2 }, { aload, 2 },
        { iload_0, 1 }, { iload_1, 1 }, { iload_2, 1 }, { iload_3, 1 }, { lload_0, 1 }, { lload_1, 1 }, { lload_2, 1 }, { lload_3, 1 },
        { fload_0, 1 }, { fload_1, 1 }, { fload_2, 1 }, { fload_3, 1 }, { dload_0, 1 }, { dload_1, 1 }, { dload_2, 1 }, { dload_3, 1 },
        { aload_0, 1 }, { aload_1, 1 }, { aload_2, 1 }, { aload_3, 1 },
        { iaload, 1 }, { laload, 1 }, { faload, 1 }, { daload, 1 }, { aaload, 1 }, { baload, 1 }, { caload, 1 }, { saload, 1 },
        { istore, 2 }, { lstore, 2 }, { fstore, 2 }, { dstore, 2 }, { astore, 2 },
        { istore_0, 1 }, { istore_1, 1 }, { istore_2, 1 }, { istore_3, 1 },  { lstore_0, 1 }, { lstore_1, 1 }, { lstore_2, 1 }, { lstore_3, 1 },
        { fstore_0, 1 }, { fstore_1, 1 }, { fstore_2, 1 }, { fstore_3, 1 }, { dstore_0, 1 }, { dstore_1, 1 }, { dstore_2, 1 }, { dstore_3, 1 },
        { astore_0, 1 }, { astore_1, 1 }, { astore_2, 1 }, { astore_3, 1 },
        { iastore, 1 }, { lastore, 1 }, { fastore, 1 }, { dastore, 1 }, { aastore, 1 }, { bastore, 1 }, { castore, 1 }, { sastore, 1 },
        { pop, 1}, { pop2, 1 }, { bcdup, 1 }, { dup_x1, 1 }, { dup_x2, 1 }, { dup2, 1 }, { dup2_x1, 1 }, { dup2_x2, 1 },
        { swap, 1 },
        { iadd, 1 }, { ladd, 1 }, { _fadd, 1 }, { dadd, 1 }, { isub, 1 }, { lsub, 1 }, { _fsub, 1 }, { dsub, 1 },
        { imul, 1 }, { lmul, 1 }, { _fmul, 1 }, { dmul, 1 }, { _idiv, 1 }, { _ldiv, 1 }, { _fdiv, 1 }, { ddiv, 1 },
        { irem, 1 }, { lrem, 1 }, { frem, 1 }, { _drem, 1 },
        { ineg, 1 }, { lneg, 1 }, { fneg, 1 }, { dneg, 1 },
        { ishl, 1 }, { lshl, 1 }, { ishr, 1 }, { lshr, 1 }, { iushr, 1 }, { lushr, 1 },
        { iand, 1 }, { land, 1 }, { ior, 1 }, { lor, 1 }, { ixor, 1 }, { lxor, 1 },
        { iinc, 3 },
        { i2l, 1 }, { i2f, 1 }, { i2d, 1 }, { l2i, 1 }, { l2f, 1 }, { l2d, 1 }, { f2i, 1 }, { f2l, 1 }, { f2d, 1 },
        { d2i, 1 }, { d2l, 1 }, { d2f, 1 }, { i2b, 1 }, { i2c, 1 }, { i2s, 1 },
        { lcmp, 1 }, { fcmpl, 1 }, { fcmpg, 1 }, { dcmpl, 1 }, { dcmpg, 1 },
        { ifeq, 3 }, { ifne, 3 }, { iflt, 3 }, { ifge, 3 }, { ifgt, 3 }, { ifle, 3 },
        { if_icmpeq, 3 }, { if_icmpne, 3 }, { if_icmplt, 3 }, { if_icmpge, 3 }, { if_icmpgt, 3 }, { if_icmple, 3 },
        { if_acmpeq, 3 }, { if_acmpne, 3 },
        { _goto, 3 }, { jsr, 3 }, { ret, 2 },

        { tableswitch, 1 }, // TODO: fix
        { lookupswitch, 1 }, // TODO: fix
        { ireturn, 1 }, { lreturn, 1 }, { freturn, 1 }, { dreturn, 1 }, { areturn, 1 },
        { _return, 1 },
        { getstatic, 3 }, { putstatic, 3 }, { getfield, 3 }, { putfield, 3 },
        { invokevirtual, 3 }, { invokespecial, 3 }, { invokestatic, 3 },
        { invokeinterface, 5 }, { invokedynamic, 5 },
        { _new, 3 },
        { newarray, 2 }, { anewarray, 3 },
        { arraylength, 1 }, { athrow, 1 },
        { checkcast, 3 }, { instanceof, 3 },
        { monitorenter, 1 }, { monitorexit, 1 },
        { wide, 5 },
        { multinewarray, 4 },
        { ifnull, 3 }, { ifnonnull, 3 },
        { goto_w, 5 }, { jsr_w, 5 },
        { breakpoint, 1 }
    };