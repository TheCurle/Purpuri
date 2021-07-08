# Overview

Purpuri is a custom JVM implementation. It is an interpreter (meaning, no JIT).

Currently, it targets JVMS7 (the JVM Specification for Java 7) and supports only a subset of opcodes. A full supported opcode list is provided [here](spec.md).

Purpuri does not currently provide any form of Native Interface.

Purpuri does not provide, or support, the Java Standard Library. The only classes in `java.*` that are provided is `java.lang.Object`, for it is required by the bytecode specification.

Purpuri does no checking on visibility of objects. Every class, method and field is public by default.

Purpuri has no exceptions, or stack unwinding. Problematic code will attempt to continue to run, possibly to the detriment of the host system.

I feel the need at this point, to specify that this is an education and personal project. Purpuri will never evolve to the point where it can replace Hotspot, or OpenJ9.

The name Purpuri means Purple, in Ido (a fork of Esperanto).

# Getting Started

To use Purpuri, find or [create](https://github.com/TheCurle/Purpuri) a binary. There are no current distributed binaries for Purpuri.

Compile a standard Java class (make sure it does not use any features or opcodes higher than Java 7).

Note that the entry point is not `main([Ljava.lang.String;)V` like traditional VMs, but `EntryPoint()I`.  
This means that in order to be deemed worthy of execution by the VM, your class must contain the function:  
``public static int EntryPoint() {``

Execute the VM:

``./Purpuri exec.class``

Any classes that are referenced by this class are loaded automatically (and classes referenced by those..), so only execute the class containing the `EntryPoint`.

By default, Purpuri will print enhanced debugging messages to the terminal for every opcode executed for every function called. In this configuration, stdout (System.out.println) messages are collected and printed all at once when the program ends.

If you append the flag `-rel` to execution:  
``./Purpuri exec.class -rel``  
then enhanced debugging is stopped, and all that will be printed is what is sent to stdout.


# Developer Features

The interpreted nature of Purpuri poses a potential benefit for people looking to debug or optimise their Java programs;  
Since every opcode executed generates a status message explaining what has been done (outside of rel-mode, at least), it provides some insight into potential manual optimizations that can't easily be visualized with standard JVMs or debuggers.

On the topic of debuggers however, Purpuri does not support any current Java debugger available.

It is planned that it will eventually support the Java Platform Debugger Architecture (JPDA), but this is a monumental task.


# Disclaimer

It was said earlier, but it deserves mention again.

Purpuri ***is an educational project***. It will ***never be a replacement for traditional VMs***. It will ***never have a JIT architecture***.

Use of Purpuri is limited strictly to educational and "fun" ventures due to its nature.

With that out of the way, I'm always accepting bug reports and feature requests (or feature submissions, if you can wrangle the code monster i've created) at [the repository](https://github.com/TheCurle/Purpuri).