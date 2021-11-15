/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <iostream>
#include <cstdio>
#include <cstring>
#include <vm/Stack.hpp>

#include <vm/debug/Debug.hpp>

void DisplayUsage(char* Name) {
    fprintf(stderr, "Purpuri VM v1.6 - Gemwire Institute\n");
    fprintf(stderr, "***************************************\n");
    fprintf(stderr, "Usage: %s <class file>\n", Name);
    #ifdef VISUAL_DEBUGGER
        fprintf(stderr, "          -d: Enable Visual Debugger\n");
    #endif
    fprintf(stderr, "          -q: Enable Quiet Mode\n");
    fprintf(stderr, "\n16:50 25/02/21 Curle\n");
}

void StartVM(char* MainFile) {

    // Preinitialization

    ClassHeap heap;
    Class* Object = new Class();
    Class* GivenClass = new Class();
    GivenClass->SetClassHeap(&heap);
    Object->SetClassHeap(&heap);

    std::string GivenPath(MainFile);
    size_t LastInd = GivenPath.find_last_of("/");
    if(LastInd != GivenPath.size())
        heap.ClassPrefix = GivenPath.substr(0, LastInd + 1);

    if(!heap.LoadClass((char*)"java/lang/Object", Object)) {
        printf("Unable to load Object class. Fatal error.\n");
        exit(6);
    }

    if(!heap.LoadClass(MainFile, GivenClass)) {
        printf("Loading given class failed. Fatal error.\n");
        exit(6);
    }

    StackFrame* Stack = new StackFrame[20];
    StackFrame::FrameBase = Stack;

    for(size_t i = 0; i < 20; i++) {
        Stack[i] = StackFrame();
    }

    size_t StackSize = 100;
    StackFrame::MemberStack = new Variable[StackSize];
    for(size_t i = 0; i < StackSize; i++) 
        StackFrame::MemberStack[i] = 0;

    Engine engine;
    
    engine._ClassHeap = &heap;

    // All classes loaded - run clinit. Keep quiet.

    bool quiet = Engine::QuietMode;
    bool debug = Debugger::Enabled;
    Engine::QuietMode = true;
    Debugger::Enabled = false;

    for(Class* clazz : heap.GetAllClasses()) {
        int init = clazz->GetMethodFromDescriptor("<clinit>", "()V", clazz->GetClassName().c_str(), clazz);
        // If there's no static initializer, skip
        if(init < 0)
            continue;

        int StartFrame = 0;

        Stack[StartFrame]._Class = clazz;
        Stack[StartFrame]._Method = &clazz->Methods[init];
        Stack[StartFrame].Stack = StackFrame::MemberStack;
        Stack[StartFrame].StackPointer = Stack[StartFrame]._Method->Code->LocalsSize;
        print("Running static initializer for class %s\n", clazz->GetClassName().c_str());

        engine.Ignite(&Stack[StartFrame]);
    }

    Engine::QuietMode = quiet;
    Debugger::Enabled = debug;

    // Invoke the static method.

    int EntryPoint = GivenClass->GetMethodFromDescriptor("EntryPoint", "()I", GivenClass->GetClassName().c_str(), GivenClass);

    if(EntryPoint < 0) {
        printf("%s does not have an EntryPoint function, unable to execute.\n", MainFile);
        return;
    }

    int StartFrame = 0;
    
    StackFrame::MemberStack = new Variable[StackSize];
    for(size_t i = 0; i < StackSize; i++) 
        StackFrame::MemberStack[i] = 0;

    Stack[StartFrame]._Class = GivenClass;
    Stack[StartFrame].ProgramCounter = 0;
    Stack[StartFrame]._Method = &GivenClass->Methods[EntryPoint];
    Stack[StartFrame].Stack = StackFrame::MemberStack;
    Stack[StartFrame].StackPointer = Stack[StartFrame]._Method->Code->LocalsSize;

    puts("*****************");
    puts("\n\nStarting Execution\n\n");
    puts("*****************");

    
    DEBUG(Debugger::SpinOff());

    engine.Ignite(&Stack[StartFrame]);

    
    // Make sure the thread never dies.
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

    
    StartVM(argv[i]);

    return 1;
}