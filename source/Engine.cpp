/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Stack.hpp"
#include <math.h>
#include <stdio.h>

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

            case ireturn:
                printf("\n******************\n\nFunction returned value %d\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                return 0;

            case iconst_5:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = (uint32_t) Code[CurrentFrame->ProgramCounter] - iconst_0;
                CurrentFrame->ProgramCounter++;
                puts("Pushed 5 to the stack");
                break;

            case istore_0:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - istore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled int %d out of the stack into 0\n", CurrentFrame->Stack[CurrentFrame->StackPointer + 1].intVal);
                break;

            case iload_0:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - iload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled int %d out of local 0 into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case fload_3:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - fload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled float %.6f out of local 3 into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal);
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

            case ldc:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1] = GetConstant(CurrentFrame->Class, (uint8_t) Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter += 2;
                printf("Pushed constant %d (%zd / %.6f) to the stack\n", Code[CurrentFrame->ProgramCounter - 1], CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal, *reinterpret_cast<float*>(&CurrentFrame->Stack[CurrentFrame->StackPointer]));
                break;
            
            case ldc2_w:
                Index = ReadShortFromStream(&Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal = ReadLongFromStream(&((char *)Class->Constants[Index])[1]);
                CurrentFrame->ProgramCounter += 3;

                printf("Pushed constant %d of type %d, value %llu / %.6f onto the stack\n", Index, Class->Constants[Index]->Tag, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal, *reinterpret_cast<double*>(&CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal));
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

            case fdiv: { 
                float topVal = CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal;
                float underVal = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal;
                float res = topVal / underVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Divided the last two floats on the stack (%.6f / %.6f = %.6f)\n", topVal, underVal, res);
                break; 
            }

            case dstore_1:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - dstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled double %.6f out of the stack into 1\n", CurrentFrame->Stack[CurrentFrame->StackPointer + 1].doubleVal);
                break;
            
            case fstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - fstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled float %.6f out of the stack into 3\n", CurrentFrame->Stack[CurrentFrame->StackPointer + 1].doubleVal);
                break;
            

            case dload_1:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] =
                    CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - dload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled %.6f out of local 1 into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal);
                break;

            case drem: {
                double topVal = CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal;
                double underVal = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal;

                double res = fmod(topVal, underVal);
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal = res;

                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Took the remainder of the last two doubles (%.6f and %.6f), result %.6f\n", topVal, underVal, res);
                break;
            }

            case d2f: {
                double val = CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal;
                float floatVal = (float) val;
                CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal = floatVal;
                CurrentFrame->ProgramCounter++;
                printf("Converted double %.6f to float %.6f\n", val, floatVal);
                break;
            }

            case d2i: {
                double val = CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal;
                int32_t intVal = (int) val; //*reinterpret_cast<int32_t*>(&val);
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = intVal;
                CurrentFrame->ProgramCounter++;
                printf("Converted double %.6f to int %d\n", val, intVal);
                break;
            }

            case f2d: {
                float val = CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal;
                double doubleVal = (double) val;
                CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal = doubleVal;
                CurrentFrame->ProgramCounter++;

                printf("Converted float %.6f to double %.6f\n", val, doubleVal);
                break;
            }

            case fmul: {
                float val = CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal;
                float underVal =  CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal;
                float res = val * underVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Multiplied the last two floats on the stack (%.6f * %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            case dmul: {
                double val = CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal;
                double underVal =  CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal;
                double res = val * underVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Multiplied the last two doubles on the stack (%.6f * %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            case dadd: {
                double val = CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal;
                double underVal =  CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal;
                double res = val + underVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].doubleVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Added the last two doubles on the stack (%.6f + %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            case fconst_2:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1].floatVal = (float) 2.0F;
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Pushed 2.0F to the stack\n");
                break;

            case dconst_1:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1].doubleVal = (double) 1.0F;
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Pushed 1.0D to the stack\n");
                break;

            default: printf("\nUnhandled opcode %x\n", Code[CurrentFrame->ProgramCounter]); CurrentFrame->ProgramCounter++; return false;
        }

        if(Error) break;
    }

    return 0;
    
}

Variable Engine::GetConstant(Class *Class, uint8_t Index) {
    Variable temp;
    temp.pointerVal = 0;


    char** Str = NULL, *StrTmp;

    char* Code = (char*) Class->Constants[Index];
    uint16_t shortTemp;
    uint32_t intTemp;
    float floatTemp;
    double doubleTemp;
    size_t longTemp;
    Object obj;

    switch(Code[0]) {
        case TypeInteger:
            temp.intVal = ReadIntFromStream(&Code[1]);
            break;
        
        case TypeFloat:
            intTemp = ReadIntFromStream(&Code[1]);
            floatTemp = *reinterpret_cast<float*>(&intTemp);
            temp.floatVal = floatTemp;
            break;
        
        case TypeString:
            shortTemp = ReadShortFromStream(&Code[1]);
            Class->GetStringConstant(shortTemp, StrTmp);
            Str = &StrTmp;
            obj = ObjectHeap->CreateString(Str, ClassHeap);
            temp.pointerVal = obj.Heap;
            break;
        
        case TypeDouble:
            longTemp = ReadLongFromStream(&Code[1]);
            doubleTemp = *reinterpret_cast<double*>(&longTemp);
            temp.doubleVal = doubleTemp;
            break;
        
        case TypeLong:
            temp.pointerVal = ReadLongFromStream(&Code[1]);
            break;
    }

    return temp;
}