/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Stack.hpp"

Variable* StackFrame::MemberStack;
StackFrame* StackFrame::FrameBase;

Engine::Engine() {
    ClassHeap = NULL;
    ObjectHeap = NULL;
}

Engine::~Engine() {}

uint32_t Engine::Ignite(StackFrame* Stack) {
    StackFrame* CurrentFrame = &Stack[0];

    printf("Execution frame %lld, stack begins %lld\n", CurrentFrame - StackFrame::FrameBase, CurrentFrame->Stack - StackFrame::MemberStack);

    // 0x100 = native
    if(CurrentFrame->Method->Access & 0x100) {
        puts("Executing native method");
        //InvokeNative(CurrentFrame);
        puts("Native method returned");
        return 0;
    }

    uint8_t* Code = CurrentFrame->Method->Code->Code + CurrentFrame->ProgramCounter;
    int32_t Error = 0;
    Class* Class = CurrentFrame->Class;
    char* Name;
    Class->GetStringConstant(CurrentFrame->Method->Name, Name);

    printf("Executing %s::%s\n", Class->GetClassName(), Name);
    int32_t Index;
    size_t Long;



    while(true) {
        switch(Code[CurrentFrame->ProgramCounter]) {
            case noop: CurrentFrame->ProgramCounter++; break;

            case _return: return 0;

            default: printf("%x\n", Code[CurrentFrame->ProgramCounter]); break;
        }

        if(Error) break;
    }

    return 0;
    
}