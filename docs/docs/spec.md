# Supported Features

Purpuri only supports a subset of Java 7 features.  
An exhaustive list would be exhausting to produce, so here's a list of unsupported features instead:

* Every class in the Java Standard Library (`java.*`, `javax.*`)
* Native methods
* Visibility
* Exceptions
* Stack unwinding
* Debugging

This means it does not support any features higher than Java 7:

* Lambdas
* Modules
* Static, private or default interface methods
* Method References
* Direct execution of Java Source files.
* Dynamic class-file constants
* Switch expressions
* `instanceof` pattern matching.
* Records
* Sealed classes

# Supported opcodes

* noop
* return, ireturn
* new
* arraylength
* newarray, anewarray
* bcdup
* invokespecial, invokevirtual, invokeinterface, invokestatic
* putfield, getfield
* [ilfd]store
* iconst_m1, iconst_[0-5]
* fconst_[0-2]
* dconst_[0-1]
* istore_[0-3]
* iload_[0-3]
* dstore_[0-3]
* dload_[0-3]
* fstore_[0-3]
* aload_[0-3]
* astore_[0-3]
* aaload, iaload
* aastore, [ilsbcfd]astore
* fload_[0-3]
* imul, iadd, isub,
* fmul, fdiv
* dmul, dadd, ddiv
* i2d, d2f, d2i, f2d, f2i
* ldc, ldc2_w
* drem
* bipush
* if_icmpeq, if_icmpge, if_icmpgt, if_icmple, if_icmplt, if_icmpne
* goto

# TODO opcodes

* dup, dup_x1, dup_x2
* dup2, dup2_x1, dup2_x2
* putstatic, getstatic
* goto_w, ret
* pop, pop2
* [fd]cmpg, [fd]cmpl, [fd]neg, dsub
* frem, fadd, fsub
* d2l, f2l, i2b, i2c, i2f, i2l, i2s, l2d, l2f, l2i
* ladd, lcmp, ldiv, lmul, lneg, lor, lrem, lreturn, lshl, lshr, lsub, lushr, lxor
* iand, idiv, iinc, ineg, ior, irem, ireturn, ishl, ishr, iushr, ixor
* if_acmpeq, if_acmpne, ifeq, ifge, ifgt, ifle, iflt, ifne, ifnonnul, ifnull
* sipush
* impdep1, impdep2
* jsr, jsr_w
* [adf]return,
* [adfil]load, [a]store
* [bcdfls]aload
* wide
* swap
* breakpoint
* checkcast
* instanceof
* lookupswitch, tableswitch
* monitorenter, monitorexit
* multianewarray

# Unsupported opcodes

* athrow


