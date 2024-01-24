/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <iostream>
#include <cstdio>
#include <cstring>
#include <vm/Stack.hpp>
#include <vm/ZipFile.hpp>

#include <vm/debug/Debug.hpp>
#include <filesystem>
#include <algorithm>
#include "vm/Native.hpp"

/**
 * When the VM is executed without a class name, we need to display a usage hint.
 * This will show the user what's available, including compile-only extras.
 * @param Name the name of the Purpuri executable.
 */
void DisplayUsage(char* Name) {
    fprintf(stderr, "Purpuri VM v1.7a - Gemwire Institute\n");
    fprintf(stderr, "***************************************\n");
    fprintf(stderr, "Usage: %s <class file>\n", Name);
    #ifdef VISUAL_DEBUGGER
        fprintf(stderr, "          -d: Enable Visual Debugger\n");
    #endif
    fprintf(stderr, "          -q: Enable Quiet Mode\n");
    fprintf(stderr, "Compiled on " ifsystem("linux", "windows", "macOS") " with " ifcompiler("gcc", "clang", "MSVC") ".");
    fprintf(stderr, "\n16:50 25/02/21 Curle\n");
}

/**
 * When a valid file is given, execute the code.
 * This will, in order:
 *  - Classload the Object class
 *  - Classload the requested class
 *  - Execute static initializers of every class in the heap all at once
 *  - Create an execution engine context
 *  - Execute the static main method of the given class file
 *
 * The actual execution and interpretation of bytecode is handled in Engine.cpp.
 * See ClassHeap for how classloading is handled.
 * @param MainFile the class file to load and execute.
 */
void StartVM(char* MainFile, char* Executable) {

    // Preinitialization
    std::string ExecutableStr(Executable);
    std::replace(ExecutableStr.begin(), ExecutableStr.end(), '/', '\\');
    std::string ExecutablePath = ExecutableStr.substr(0, ExecutableStr.find_last_of('\\'));

    ClassHeap heap;
    heap.AddToBootstrapClasspath(std::filesystem::current_path().append("stdlib.jar").string());
    heap.AddToClassPath(ExecutablePath);
    heap.AddToClassPath(std::filesystem::current_path().string());

    // Set up the libnative library as a valid target for the Native module.
    Native::LoadLibrary(NATIVES_FILE);

    // -------------------------------------------------------------------------------------------------------------- //

    // First, we initialize the Stack Frame.
    // The Stack Frame is what handles calling methods and returning values.
    // By default, it only has 20 frames, but this is merely a safe guard.
    auto* Stack = new StackFrame[20];

    // The frames need to each be initialized, since we only declared the list.
    for(size_t i = 0; i < 20; i++) {
        Stack[i] = StackFrame();
    }

    // Next, the Object Stack.
    // This is what is actually referred to as the "stack" in Java.
    // When you perform a calculation like 2 + 2, the value 2 is pushed to the stack twice, and then calculated upon.
    // This works similar to an internal Reverse Polish Notation; 2 2 ADD.

    // The Object Stack is implemented as a 100-deep list, that can be traversed up and down while retaining values.
    // This ability to pop and then recover the value with push is important, and is why this is not a traditional
    // stack data structure. An array makes more sense here, just this once.
    size_t StackSize = 100;
    StackFrame::MemberStack = new Variable[StackSize];
    // Like before, we only declared the list, we need to initialize the values.
    for(size_t i = 0; i < StackSize; i++)
        StackFrame::MemberStack[i] = 0;

    StackFrame::ClassloadStack = new Variable[StackSize];
    // Like before, we only declared the list, we need to initialize the values.
    for(size_t i = 0; i < StackSize; i++)
        StackFrame::ClassloadStack[i] = 0;

    // Now we create the Execution Engine itself.
    // This is what actually interprets the bytecode.
    // See Engine.cpp for how it works.
    Engine engine;
    // The Engine needs to know what classes it can access, so we provide the heap here.
    engine._ClassHeap = &heap;

    // -------------------------------------------------------------------------------------------------------------- //

    // Here's where we start loading classes.

    // Initialize the Object class before anything else.
    auto* Object = new Class();
    Object->SetClassHeap(&heap);

    if(!heap.LoadClass((char*)"java/lang/Object", Object, engine.ClassloadingStack, &engine)) {
        printf("Unable to load Object class. Fatal error.\n");
        exit(6);
    }

    // Initialize the class we were asked to
    auto* GivenClass = new Class();
    GivenClass->SetClassHeap(&heap);

    std::string GivenPath(MainFile);

    // We need to figure out where the specified file is, relative to the current working directory.
    // With this, we can load relative classes (ie. src/GivenClass loads OtherClass, we must append "src/" to the
    // file path to be able to load it properly.
    size_t LastInd = GivenPath.find_last_of('/');
    if(LastInd != GivenPath.size())
        heap.ClassPrefix = GivenPath.substr(0, LastInd + 1);

    // With the prefix handled, classload the requested class file.
    if(!heap.LoadClass(MainFile, GivenClass, engine.ClassloadingStack, &engine)) {
        printf("Loading given class failed. Fatal error.\n");
        exit(6);
    }

    // Here we search for the index of the EntryPoint method.
    // The reason we use EntryPoint rather than main will be explained later, but we need to check this NOW,
    // rather than after we do so much extra work on clinit.

    // Search for the EntryPoint method..
    uint32_t EntryPoint = GivenClass->GetMethodFromDescriptor("EntryPoint", "()I", GivenClass->GetClassName().c_str(), GivenClass);

    // If it doesn't exist, we need to stop now.
    // It doesn't make sense to execute any code if it's all going to go to waste anyway.
    if(EntryPoint == std::numeric_limits<uint32_t>::max()) {
        printf("%s does not have an EntryPoint function, unable to execute.\n", MainFile);
        return;
    }

    // -------------------------------------------------------------------------------------------------------------- //

    // At this point, we're ready to start actually executing the class we were asked to.
    // This is done by seeking for the EntryPoint method.
    // In the class, it looks like this:
    //  public static int EntryPoint();

    // Again, this is a deviation from the Java spec, which dictates that code should start executing from
    // the main method. However, we have a reason for doing it like this.

    // Having two entry points - one for Java, one for Purpuri, allows us to implement a testing framework that compares
    // the outputs of Purpuri and Java based on the same low-complexity classes.

    // The EntryPoint has been searched and stored above - before the static initializers, so we can move on to setting
    // the Execution Engine context again.

    // Starting again, so make this index 0.
    int StartFrame = 0;

    Stack[StartFrame]._Class = GivenClass;
    Stack[StartFrame].ProgramCounter = 0; // The Program Counter counts upwards, and indicates the byte we're currently executing.
    Stack[StartFrame]._Method = &GivenClass->Methods[EntryPoint]; // EntryPoint is stored above.
    Stack[StartFrame].Stack = StackFrame::MemberStack; // The stack starts at 0.
    Stack[StartFrame].StackPointer = Stack[StartFrame]._Method->Code->LocalsSize; // The stack pointer starts high and grows towards 0.

    // If the debugger is enabled, now is the time to start its' thread.
    // The Engine will prepare and wait for the Debugger before it will start executing.
    DEBUG(Debugger::SpinOff());

    // -------------------------------------------------------------------------------------------------------------- //

    // All else is ready,
    // This call will start the EntryPoint function of the class the user entered.
    // If a value is returned, it will be lost (but printed, due to the nature of the VM).
    // There is no spin-down required, since the main function is about to be terminated and all memory freed to the OS.

    // However, this may change in the future.

    puts("\n\nStarting Execution\n\n");
    engine.Ignite(&Stack[StartFrame]);

    // If we get here, the EntryPoint function returned successfully.
    // This is an achievement!

    // The debugger lives in a separate thread, so after the Engine is finished executing, we need to wait for that
    // thread to die.
    DEBUG(Debugger::Rejoin());
}

int main(int argc, char* argv[]) {

    // Parse command line arguments.
    // Stolen from Greek Tools' Erythro Compiler.
    int i;
    for(i = 1/*skip 0*/; i < argc; i++) {
        // If we're not a flag, we can skip.
        // We only care about flags in rows.
        // ie. erc >> -v -T -o << test.exe src/main.er
        if(*argv[i] != '-')
            break;
        
        // Once we identify a flag, we need to make sure it's not just a minus in-place.
        for(int j = 1; (*argv[i] == '-') && argv[i][j]; j++) {
            // Finally, identify what option is being invoked.
            switch(argv[i][j]) {
                case 'd':
                    #ifndef VISUAL_DEBUGGER
                        printf("Purpuri was compiled without Debugging features. Recompile with -DDEBUGGER to enable it.\n");
                    #else
                        // Debug Stuff.
                        Debugger::Enabled = true;
                    #endif

                    break;

                case 'q':
                    // Quiet stuff!
                    Engine::QuietMode = true;
                    break;

                default:
                    DisplayUsage(argv[0]);
                    return 0;
            }
        }
    }

    // If we didn't provide anything other than flags, we need to show how to use the program.
    if(i >= argc) {
        DisplayUsage(argv[0]);
        return 0;
    }
    
    StartVM(argv[i], argv[0]);

    return 1;
}