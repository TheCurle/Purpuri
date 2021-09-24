/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Stack.hpp"
#include "../headers/Class.hpp"
#include <math.h>
#include <stdio.h>
#include <cstring>

#define JUMP_TO(x) \
    CurrentFrame->ProgramCounter = x;

Variable* StackFrame::MemberStack;
StackFrame* StackFrame::FrameBase;

Engine::Engine() {
    _ClassHeap = NULL;
}

Engine::~Engine() {}

uint32_t Engine::Ignite(StackFrame* Stack) {
    StackFrame* CurrentFrame = &Stack[0];

    printf("New execution frame generated.\n");

    // 0x100 = native
    if(CurrentFrame->_Method->Access & 0x100) {
        puts("Executing native method");
        //InvokeNative(CurrentFrame);
        puts("Native method returned");
        return 0;
    }

    uint8_t* Code = CurrentFrame->_Method->Code->Code + CurrentFrame->ProgramCounter;
    int32_t Error = 0;
    Class* Class = CurrentFrame->_Class;
    std::string Name = Class->GetStringConstant(CurrentFrame->_Method->Name);

    printf("Executing %s::%s\n", Class->GetClassName().c_str(), Name.c_str());
    int32_t Index;
    size_t Long;


    while(true) {
        switch(Code[CurrentFrame->ProgramCounter]) {
            case noop: CurrentFrame->ProgramCounter++; break;

            case _return:
                puts("Function returns"); return 0;

            case ireturn:
                printf("\n******************\n\nFunction returned value %d\n\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                return 0;

            case _new:
                if(!New(CurrentFrame)) {
                    printf("Creating new object failed.\r\n");
                    exit(5);
                }
                CurrentFrame->ProgramCounter += 3;
                printf("Initialized new object\n");
                break;

            case arraylength:
                CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal
                    = _ObjectHeap.GetArraySize(CurrentFrame->Stack[CurrentFrame->StackPointer].object);
                CurrentFrame->ProgramCounter += 2;
                break;

            case newarray:
                NewArray(CurrentFrame);
                CurrentFrame->ProgramCounter += 2;
                printf("Initialized new array\n");
                break;

            case anewarray:
                ANewArray(CurrentFrame);
                CurrentFrame->ProgramCounter += 2;
                printf("Initialized new a-array\n");
                break;

            case bcdup:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer];
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Duplicated the last item on the stack\n");
                break;

            case invokespecial:
                Invoke(CurrentFrame, invokespecial);
                CurrentFrame->ProgramCounter += 3;
                break;

            case invokevirtual:
                Invoke(CurrentFrame, invokevirtual);
                CurrentFrame->ProgramCounter += 3;
                break;

            case invokeinterface:
                Invoke(CurrentFrame, invokeinterface);
                CurrentFrame->ProgramCounter += 5;
                break;
            case invokestatic:
                Invoke(CurrentFrame, invokestatic);
                CurrentFrame->ProgramCounter += 3;
                break;

            case putfield:
                PutField(CurrentFrame);
                CurrentFrame->StackPointer -= 2;
                CurrentFrame->ProgramCounter += 3;
                break;

            case getfield:
                GetField(CurrentFrame);
                CurrentFrame->ProgramCounter += 3;
                printf("Retrieved value %zu from field.\r\n", CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal);
                break;

            case istore:
            case lstore:
			    CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter + 1]] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                printf("Stored value %zu in local %d.\n", CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal, Code[CurrentFrame->ProgramCounter + 1]);
		    	CurrentFrame->ProgramCounter += 2;
			    break;

            case fstore:
            case dstore:
			    CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter + 1]] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                printf("Stored value %.6f in local %d.\n", CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal, Code[CurrentFrame->ProgramCounter + 1]);
		    	CurrentFrame->ProgramCounter += 2;
			    break;

            case iconst_m1:
            case iconst_0:
            case iconst_1:
            case iconst_2:
            case iconst_3:
            case iconst_4:
            case iconst_5:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = (uint8_t) Code[CurrentFrame->ProgramCounter] - iconst_0;
                CurrentFrame->ProgramCounter++;
                printf("Pushed int constant %d to the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case istore_0:
            case istore_1:
            case istore_2:
            case istore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - istore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled int %d out of the stack into 0\n", CurrentFrame->Stack[CurrentFrame->StackPointer + 1].intVal);
                break;

            case iload_0:
            case iload_1:
            case iload_2:
            case iload_3:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - iload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled int %d out of local %d into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal, Code[CurrentFrame->ProgramCounter - 1] - iload_0);
                break;

            case aaload:
            case iaload:
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1] =
                    _ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 1].object)
                        [CurrentFrame->Stack[CurrentFrame->StackPointer].intVal + 1];
                printf("Pulled value %d out of the %zuth entry of array object %zu\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal, (size_t) CurrentFrame->Stack[CurrentFrame->StackPointer].intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 1].object.Heap);
			    CurrentFrame->StackPointer--;
			    CurrentFrame->ProgramCounter++;
			    break;

            case fload_0:
            case fload_1:
            case fload_2:
            case fload_3:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - fload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled float %.6f out of local %d into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal, Code[CurrentFrame->ProgramCounter - 1] - fload_0);
                break;

            case imul:
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal
                    * CurrentFrame->Stack[CurrentFrame->StackPointer].intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Multiplied the last two integers on the stack (result %d)\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case iadd:
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal
                    + CurrentFrame->Stack[CurrentFrame->StackPointer].intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Added the last two integers on the stack (%d + %d = %d)\r\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal, CurrentFrame->Stack[CurrentFrame->StackPointer].intVal - CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal, CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case isub:
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer].intVal
                    - CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Subtracted the last two integers on the stack (%d - %d = %d)\r\n", 
                    CurrentFrame->Stack[CurrentFrame->StackPointer + 1].intVal, 
                    CurrentFrame->Stack[CurrentFrame->StackPointer + 1].intVal - CurrentFrame->Stack[CurrentFrame->StackPointer].intVal, 
                    CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
                break;

            case i2d:
                Long = CurrentFrame->Stack[CurrentFrame->StackPointer].intVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal = (double) Long;
                CurrentFrame->ProgramCounter++;
                printf("Convert int %zu to double %.6f\n", Long, CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal);
                break;

            case ldc:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1] = GetConstant(CurrentFrame->_Class, (uint8_t) Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter += 2;
                printf("Pushed constant %d (0x%zx / %.6f) to the stack\n", Code[CurrentFrame->ProgramCounter - 1], CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal);
                break;

            case ldc2_w:
                Index = ReadShortFromStream(&Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal = ReadLongFromStream(&((char *)Class->Constants[Index])[1]);
                CurrentFrame->ProgramCounter += 3;

                printf("Pushed constant %d of type %d, value 0x%zx / %.6f onto the stack\n", Index, Class->Constants[Index]->Tag, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal);
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

            case _fdiv: {
                float topVal = CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal;
                float underVal = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal;
                float res = topVal / underVal;
                CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Divided the last two floats on the stack (%.6f / %.6f = %.6f)\n", topVal, underVal, res);
                break;
            }

            case dstore_0:
            case dstore_1:
            case dstore_2:
            case dstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - dstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled double %.6f out of the stack into 1\n", CurrentFrame->Stack[CurrentFrame->StackPointer + 1].doubleVal);
                break;

            case fstore_0:
            case fstore_1:
            case fstore_2:
            case fstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - fstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled float %f out of the stack into local %d\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal, Code[CurrentFrame->ProgramCounter - 1] - fstore_0);
                break;

            case dload_0:
            case dload_1:
            case dload_2:
            case dload_3:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] =
                    CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - dload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled %.6f out of local 1 into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal);
                break;

            case aload_0:
            case aload_1:
            case aload_2:
            case aload_3:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - aload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled object %zu out of local %d into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].object.Heap, Code[CurrentFrame->ProgramCounter - 1] - aload_0);
                break;

            case astore_0:
            case astore_1:
            case astore_2:
            case astore_3:
                CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - astore_0] = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled the last object on the stack into local %d\n", Code[CurrentFrame->ProgramCounter - 1] - astore_0);
                break;


		    case aastore:
			    _ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal + 1] =
                        CurrentFrame->Stack[CurrentFrame->StackPointer];
                printf("Stored reference %d into the %zuth entry of array object %zu.\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal, (size_t) CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    CurrentFrame->ProgramCounter++;
			    break;

            case iastore:
            case lastore:
            case sastore:
            case bastore:
            case castore:
            	_ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal + 1] =
                        CurrentFrame->Stack[CurrentFrame->StackPointer];
                printf("Stored number %d into the %zuth entry of array object %zu.\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal, (size_t) CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    CurrentFrame->ProgramCounter++;
			    break;

            case fastore:
            case dastore:
                _ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal + 1] =
                        CurrentFrame->Stack[CurrentFrame->StackPointer];
                printf("Stored number (%.6f) into the %zuth entry of array object %zu.\n", CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal, (size_t) CurrentFrame->Stack[CurrentFrame->StackPointer - 1].intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    CurrentFrame->ProgramCounter++;
			    break;

            case _drem: {
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

            case f2i: {
                float val = CurrentFrame->Stack[CurrentFrame->StackPointer].floatVal;
                int32_t intVal = (int) val; //*reinterpret_cast<int32_t*>(&val);
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = intVal;
                CurrentFrame->ProgramCounter++;
                printf("Converted float %.6f to int %d\n", val, intVal);
                break;
            }

            case _fmul: {
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

            case fconst_0:
            case fconst_1:
            case fconst_2:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1].floatVal = (float) 2.0F;
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Pushed 2.0F to the stack\n");
                break;

            case dconst_0:
            case dconst_1:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1].doubleVal = (double) 1.0F;
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Pushed 1.0D to the stack\n");
                break;

            case bipush:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].charVal = (uint8_t) Code[CurrentFrame->ProgramCounter + 1];
                CurrentFrame->ProgramCounter += 2;
                printf("Pushed char %d to the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].charVal);
                break;

            case sipush:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].shortVal = ReadShortFromStream(Code + (CurrentFrame->ProgramCounter + 1));
                CurrentFrame->ProgramCounter += 3;
                printf("Pushed short %d to the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].shortVal);
                break;

            case if_icmpeq: {
                bool Equal = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal == CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal;
                printf("Comparing: %zd == %zd\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal);
                printf("Integer equality comparison returned %s\n", Equal ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                if(Equal) {
                    short Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %hd) = %hd\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    CurrentFrame->ProgramCounter += Offset;
                } else {
                    CurrentFrame->ProgramCounter += 3;
                }
                break;
            }

            case if_icmpne: {
                bool NotEqual = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal != CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal;
                printf("Comparing: %zd != %zd\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal);
                printf("Integer inequality comparison returned %s\n", NotEqual ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                if(NotEqual) {
                    short Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %hd) = %hd\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    CurrentFrame->ProgramCounter += Offset;
                } else {
                    CurrentFrame->ProgramCounter += 3;
                }
                break;
            }

            case if_icmpgt: {
                bool GreaterThan = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal > CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal;
                printf("Comparing: %zu > %zu\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal);
                printf("Integer greater-than comparison returned %s\n", GreaterThan ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                if(GreaterThan) {
                    short Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %hd) = %hd\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    CurrentFrame->ProgramCounter += Offset;
                } else {
                    CurrentFrame->ProgramCounter += 3;
                }
                break;
            }

            case if_icmplt: {
                bool LessThan = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal < CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal;
                printf("Comparing: %zu < %zu\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal);
                printf("Integer less-than comparison returned %s\n", LessThan ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                if(LessThan) {
                    short Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %hd) = %hd\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    CurrentFrame->ProgramCounter += Offset;
                } else {
                    CurrentFrame->ProgramCounter += 3;
                }
                break;
            }

            case if_icmpge: {
                bool GreaterEqual = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal >= CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal;
                printf("Comparing: %zu >= %zu\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal);
                printf("Integer greater-than-or-equal comparison returned %s\n", GreaterEqual ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                if(GreaterEqual) {
                    short Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %hd) = %hd\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    CurrentFrame->ProgramCounter += Offset;
                } else {
                    CurrentFrame->ProgramCounter += 3;
                }
                break;
            }

            case if_icmple: {
                bool LessEqual = CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal <= CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal;
                printf("Comparing: %zu <= %zu\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].pointerVal, CurrentFrame->Stack[CurrentFrame->StackPointer].pointerVal);
                printf("Integer less-than-or-equal comparison returned %s\n", LessEqual ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                if(LessEqual) {
                    short Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %hd) = %hd\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    CurrentFrame->ProgramCounter += Offset;
                } else {
                    CurrentFrame->ProgramCounter += 3;
                }
                break;
            }

            case iinc: {
                uint8_t Index = Code[CurrentFrame->ProgramCounter + 1];
                uint8_t Offset = Code[CurrentFrame->ProgramCounter + 2];

                printf("Incrementing local %d by %d.\n", Index, Offset);
                CurrentFrame->Stack[Index].pointerVal += Offset;
                
                CurrentFrame->ProgramCounter += 3;
                break;
            }

            case _goto: {
                short Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                printf("Jumping to (%d + %hd) = %hd\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                CurrentFrame->ProgramCounter += Offset;
                break;
            }


            default: printf("\nUnhandled opcode 0x%x\n", Code[CurrentFrame->ProgramCounter]); CurrentFrame->ProgramCounter++; return false;
        }

        if(Error) break;
    }

    return 0;
}

Variable Engine::GetConstant(Class *Class, uint8_t Index) {
    Variable temp;
    temp.pointerVal = 0;

    char* Code = (char*) Class->Constants[Index];
    uint16_t shortTemp;
    Object obj;

    switch(Code[0]) {
        case TypeFloat:
        case TypeInteger:
            temp.intVal = ReadIntFromStream(&Code[1]);
            break;

        case TypeString:
            shortTemp = ReadShortFromStream(&Code[1]);
            obj = _ObjectHeap.CreateString(Class->GetStringConstant(shortTemp).c_str(), _ClassHeap);
            temp.pointerVal = obj.Heap;
            break;

        case TypeDouble:
        case TypeLong:
            temp.pointerVal = ReadLongFromStream(&Code[1]);
            break;
    }

    return temp;
}

int Engine::New(StackFrame *Stack) {
    Stack->StackPointer++;
    uint8_t* Code = Stack->_Method->Code->Code;
    uint16_t Index = ReadShortFromStream(&Code[Stack->ProgramCounter + 1]);

    if(!Stack->_Class->CreateObject(Index, &_ObjectHeap, Stack->Stack[Stack->StackPointer].object))
        return -1;
    return 1;
}

void Engine::NewArray(StackFrame* Stack) {
    uint8_t Type = Stack->_Method->Code->Code[Stack->ProgramCounter + 1];
    Stack->Stack[Stack->StackPointer + 1].object = _ObjectHeap.CreateArray(Type, Stack->Stack[Stack->StackPointer].intVal);
    Stack->StackPointer++;
    printf("Initialized a %d-wide array of type %d.\n", Stack->Stack[Stack->StackPointer - 1].intVal, Type);
}

void Engine::ANewArray(StackFrame* Stack) {
    uint16_t Index = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);
    uint32_t Count = Stack->Stack[Stack->StackPointer].intVal;
    Stack->StackPointer++;

    if(!Stack->_Class->CreateObjectArray(Index, Count, _ObjectHeap, Stack->Stack[Stack->StackPointer].object))
        // TODO: ERROR
        printf("Intializing array failed.");
    printf("Initialized a %d-wide array of objects.\n", Count);
}

void Engine::Invoke(StackFrame *Stack, uint16_t Type) {

    uint16_t MethodIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    printf("Invoking a function.. index is %d.\r\n", MethodIndex);

    uint8_t* Constants = (uint8_t*) Stack->_Class->Constants[MethodIndex];

    char* TypeName = NULL;
    switch(Type) {
        case invokeinterface:
            if(!(Constants[0] == TypeInterfaceMethod))
                exit(4);
            TypeName = (char*) "interface";
            break;
        case invokevirtual:
        case invokespecial:
            if(!(Constants[0] == TypeMethod))
                exit(4);
            TypeName = (char*) " instance";
            break;
        case invokestatic:
            if(!(Constants[0] == TypeMethod))
                exit(4);
            TypeName = (char*) "   static";
            break;
        default:
            printf("\tUnknown invocation: %d as %d\r\n", Constants[0], Type);
            break;
    }

    printf("\tCalculating invocation for %s function.\n", TypeName);

    uint16_t ClassIndex = ReadShortFromStream(&Constants[1]);
    uint16_t ClassNTIndex = ReadShortFromStream(&Constants[3]);

    Constants = (uint8_t*) Stack->_Class->Constants[ClassIndex];
    std::string ClassName = Stack->_Class->GetStringConstant(ClassIndex);

    printf("\tInvocation is calling into class %s\n", ClassName.c_str());

    Constants = (uint8_t*) Stack->_Class->Constants[ClassNTIndex];

    Method MethodToInvoke;
    MethodToInvoke.Name = ReadShortFromStream(&Constants[1]);
    MethodToInvoke.Descriptor = ReadShortFromStream(&Constants[3]);
    MethodToInvoke.Access = 0;

    std::string MethodName = Stack->_Class->GetStringConstant(MethodToInvoke.Name);
    std::string MethodDesc = Stack->_Class->GetStringConstant(MethodToInvoke.Descriptor);

    printf("\tInvocation resolves to method %s%s\n", MethodName.c_str(), MethodDesc.c_str());
    size_t Parameters = GetParameters(MethodDesc.c_str());

    printf("\tMethod has %zu parameters, skipping ahead..\r\n", Parameters);
    Variable UnderStack = Stack->Stack[Stack->StackPointer - (Parameters + 1)];

    for(size_t i = 0; i < Parameters; i++) {
        printf("\t\tParameter %zu = %zu\r\n", i, Stack->Stack[Stack->StackPointer - i].pointerVal);
    }

    Variable ClassInStack = Stack->Stack[Stack->StackPointer - Parameters];
    printf("\tClass to invoke is object #%zu.\r\n", ClassInStack.object.Heap);

    Variable* ObjectFromHeap = _ObjectHeap.GetObjectPtr(ClassInStack.object);
    printf("\tClass at 0x%zx.\r\n", ObjectFromHeap->pointerVal);
    class Class* VirtualClass = (class Class*) ObjectFromHeap->pointerVal;

    int MethodInClassIndex = VirtualClass->GetMethodFromDescriptor(MethodName.c_str(), MethodDesc.c_str(), ClassName.c_str(), VirtualClass);


    Stack[1] = StackFrame();

    Stack[1]._Method = &VirtualClass->Methods[MethodInClassIndex];
    printf("\tMethod has access 0x%x.\r\n", Stack[1]._Method->Access);
    if(!(Stack[1]._Method->Access & 0x8))
        Parameters++;

    Stack[1]._Class = VirtualClass;

    Stack[1].Stack = &StackFrame::MemberStack[Stack->Stack - StackFrame::MemberStack + Stack->StackPointer - Parameters];
    size_t ParameterCount = 0;
    printf("\tSetting locals..\r\n");
    // If non static, set "this"
    if(!(Stack[1]._Method->Access & 0x8)) {
        printf("\tSetting \"this\" to %zu.\r\n", ClassInStack.object.Heap);
        Stack[1].Stack[ParameterCount] = ClassInStack;
        ParameterCount++;
    }

    for(size_t i = ParameterCount; i < Parameters; i++) {
        Stack[1].Stack[i] = Stack->Stack[Stack->StackPointer - (i - 1)];
        printf("\tSetting local %zu to %zu.\r\n", i, Stack[1].Stack[i].pointerVal);
    }

    printf("\tFunction's parameters start at %zu.\r\n", Parameters);

    Stack[1].StackPointer = Parameters;

    printf("Invoking method %s%s\n", MethodName.c_str(), MethodDesc.c_str());

    this->Ignite(&Stack[1]);
    Variable ReturnValue = Stack[1].Stack[Stack[1].StackPointer];

    if(MethodDesc.find(")V") != std::string::npos)
        Parameters--;

    Stack->StackPointer -= Parameters;

    printf("Shrinking the stack by %zu positions.\r\n", Parameters);

    int Offset = 0;
    if(MethodDesc.find(")V") == std::string::npos) {
        Stack->Stack[Stack->StackPointer] = ReturnValue;
        printf("Pushing function return value, %zu\r\n", ReturnValue.pointerVal);
        Offset--; // Make Offset -1, so that the line below works with return.
    }
    Stack->Stack[Stack->StackPointer + Offset] = UnderStack;
    printf("Restoring the value %zu under the function, just in case.\r\n", UnderStack.pointerVal);

}

uint16_t Engine::GetParameters(const char *Descriptor) {
    uint16_t Count = 0;
    size_t Length = strlen(Descriptor);

    for(size_t i = 1; i < Length; i++) {
        if(Descriptor[i] == 'L')
            while(Descriptor[i] != ';')
                i++;

        if(Descriptor[i] == ')')
            break;

        if(Descriptor[i] == 'J' || Descriptor[i] == 'D')
            Count++;
        Count++;
    }

    return Count;
}
