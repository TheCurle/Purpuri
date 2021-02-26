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

            case _return: 
                puts("Function returns"); return 0;

            case iconst_5:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = (uint32_t) Code[CurrentFrame->ProgramCounter] - iconst_0;
                CurrentFrame->ProgramCounter++;
                printf("Pushed %d to the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case istore_0:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - istore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled %d out of the stack into 0\n", CurrentFrame->Stack[CurrentFrame->StackPointer + 1].intVal);
                break;

            case iload_0:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - iload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled %d out of 0 into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case imul:
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal
                    * CurrentFrame->Stack[CurrentFrame->StackPointer].intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Multiplied the last two values on the stack (result %d)\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case i2d:
                Long = CurrentFrame->Stack[CurrentFrame->StackPointer].intVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal = (double) Long;
                CurrentFrame->ProgramCounter++;
                printf("Convert int %zu to double %.6f\n", Long, CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal);
                break;
            
            case ldc2_w:
                Index = ReadShortFromStream(&Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = ReadIntFromStream(&((char *)Class->Constants[Index])[1]);
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = ReadIntFromStream(&((char *)Class->Constants[Index])[5]);
                CurrentFrame->ProgramCounter += 3;

                Long = (size_t) CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal << 32 | CurrentFrame->Stack[CurrentFrame->StackPointer].intVal;
                printf("Pushed constant %d of type %d, value %llu / %.6f onto the stack\n", Index, Class->Constants[Index]->Tag, Long, *reinterpret_cast<double*>(&Long));
                break;

            case ddiv: {
                double topVal = CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal;
                double underVal = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal;
                double res = topVal / underVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Divided the last two doubles on the stack (%.6f / %.6f = %.6f)\n", topVal, underVal, res);
                break;            
            }
            

            default: printf("\nUnhandled opcode %x\n", Code[CurrentFrame->ProgramCounter]); CurrentFrame->ProgramCounter++; return false;
        }

        if(Error) break;
    }

    return 0;
    
}