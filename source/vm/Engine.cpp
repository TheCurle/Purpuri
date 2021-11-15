/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Stack.hpp>
#include <vm/Class.hpp>
#include <vm/Native.hpp>

#include <vm/debug/Debug.hpp>

#include <math.h>
#include <stdio.h>
#include <cstring>

template<class T>
using ptr = T *;

#define JUMP_TO(x) \
    CurrentFrame->ProgramCounter = x;

#define NATIVES \
    "./bin/"

#define PEEK \
    CurrentFrame->Stack[CurrentFrame->StackPointer]

#define UNDER \
    CurrentFrame->Stack[CurrentFrame->StackPointer - 1]

#define OVER \
    CurrentFrame->Stack[CurrentFrame->StackPointer + 1]


Variable* StackFrame::MemberStack;
StackFrame* StackFrame::FrameBase;
ObjectHeap Engine::_ObjectHeap;
bool Engine::QuietMode = false;

Engine::Engine() {
    _ClassHeap = NULL;
}

Engine::~Engine() {}

void Engine::InvokeNative(NativeContext Context) {

    std::string FunctionName = Native::EncodeName(Context);
    printf("Function %s%s from class %s wants to call to function %s.\n", Context.MethodName.c_str(), Context.MethodDescriptor.c_str(), Context.ClassName.c_str(), FunctionName.c_str());

    Native::Parameters = Context.Parameters;
    #ifdef WIN32
        #define EXT ".dll"
    #elif linux
        #define EXT ".so"
    #endif

    Native::LoadLibrary(NATIVES "\\libnative" EXT);
    
    auto func = Native::LoadSymbol(NATIVES "\\libnative" EXT, FunctionName.c_str());

    if(func == nullptr) {
        printf("\n\n*************\n*************\n*************\nFunction %s does not exist in any loaded library.\n*************\n*************\n*************\n", FunctionName.c_str());
        exit(7);
    }

    printf("Jumping to native function.\n");
    func();
}

uint32_t Engine::Ignite(StackFrame* Stack) {
    StackFrame* CurrentFrame = &Stack[0];

    printf("New execution frame generated.\n");

    uint8_t* Code = CurrentFrame->_Method->Code->Code + CurrentFrame->ProgramCounter;
    int32_t Error = 0;
    Class* Class = CurrentFrame->_Class;
    std::string Name = Class->GetStringConstant(CurrentFrame->_Method->Name);

    printf("Executing %s::%s\n", Class->GetClassName().c_str(), Name.c_str());
    int32_t Index;
    size_t Long;


    while(true) {
        // Before every instruction, we should notify the debugger what's happening.
        DEBUG({
            Debugger::SetStack(CurrentFrame);
            std::unique_lock<std::mutex> lock(Debugger::Locker);
            Debugger::Notifier.wait(lock, []{return Debugger::ShouldStep;});
            Debugger::ShouldStep = false;
        });

        printf("%d: ", Code[CurrentFrame->ProgramCounter]);

        switch(Code[CurrentFrame->ProgramCounter]) {
            case Instruction::noop: CurrentFrame->ProgramCounter++; break;

            case Instruction::_return:
                puts("Function returns"); return 0;

            case Instruction::ireturn:
                fprintf(stderr, "\n******************\n\nFunction returned value %d\n\n", PEEK.intVal);
                return 0;

            case Instruction::_new:
                if(!New(CurrentFrame)) {
                    printf("Creating new object failed.\r\n");
                    exit(5);
                }
                CurrentFrame->ProgramCounter += 3;
                printf("Initialized new object\n");
                break;

            case Instruction::arraylength:
                PEEK.pointerVal
                    = _ObjectHeap.GetArraySize(PEEK.object);
                printf("Got the size " PrtSizeT " from the array just pushed.\n", PEEK.pointerVal);
                CurrentFrame->ProgramCounter++;
                break;

            case Instruction::newarray:
                printf("New array initializing:\n");
                NewArray(CurrentFrame);
                CurrentFrame->ProgramCounter += 2;
                printf("Initialized new array\n");
                break;

            case Instruction::anewarray:
                ANewArray(CurrentFrame);
                CurrentFrame->ProgramCounter += 2;
                printf("Initialized new a-array\n");
                break;

            case Instruction::bcdup:
                OVER =
                    PEEK;
                // pre-emptive strike on weirdness
                PEEK.object =
                    OVER.object;
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Duplicated the last item on the stack\n");
                break;

            case Instruction::invokespecial:
                Invoke(CurrentFrame, Instruction::invokespecial);
                CurrentFrame->ProgramCounter += 3;
                break;

            case Instruction::invokevirtual:
                Invoke(CurrentFrame, Instruction::invokevirtual);
                CurrentFrame->ProgramCounter += 3;
                break;

            case Instruction::invokeinterface:
                Invoke(CurrentFrame, Instruction::invokeinterface);
                CurrentFrame->ProgramCounter += 5;
                break;

            case Instruction::invokestatic:
                Invoke(CurrentFrame, Instruction::invokestatic);
                CurrentFrame->ProgramCounter += 3;
                break;

            case Instruction::putstatic:
                PutStatic(CurrentFrame);
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter += 3;
                break;

            case Instruction::putfield:
                PutField(CurrentFrame);
                CurrentFrame->StackPointer -= 2;
                CurrentFrame->ProgramCounter += 3;
                break;

            case Instruction::getstatic:
                GetStatic(CurrentFrame);
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter += 3;
                printf("Got value " PrtSizeT ".\n", Stack->Stack[Stack->StackPointer].pointerVal);
                break;

            case Instruction::getfield:
                GetField(CurrentFrame);
                CurrentFrame->ProgramCounter += 3;
                printf("Retrieved value " PrtSizeT " from field.\r\n", PEEK.pointerVal);
                break;

            case Instruction::istore:
            case Instruction::lstore:
			    CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter + 1]] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                printf("Stored value " PrtSizeT " in local %d.\n", PEEK.pointerVal, Code[CurrentFrame->ProgramCounter + 1]);
		    	CurrentFrame->ProgramCounter += 2;
			    break;

            case Instruction::fstore:
            case Instruction::dstore:
			    CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter + 1]] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                printf("Stored value %.6f in local %d.\n", PEEK.doubleVal, Code[CurrentFrame->ProgramCounter + 1]);
		    	CurrentFrame->ProgramCounter += 2;
			    break;

            case Instruction::iconst_m1:
            case Instruction::iconst_0:
            case Instruction::iconst_1:
            case Instruction::iconst_2:
            case Instruction::iconst_3:
            case Instruction::iconst_4:
            case Instruction::iconst_5:
                CurrentFrame->StackPointer++;
                PEEK.intVal = (uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::iconst_0;
                CurrentFrame->ProgramCounter++;
                printf("Pushed int constant %d to the stack\n", PEEK.intVal);
                break;

            case Instruction::istore_0:
            case Instruction::istore_1:
            case Instruction::istore_2:
            case Instruction::istore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - Instruction::istore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled int %d out of the stack into local %d\n", OVER.intVal, (uint8_t)Code[CurrentFrame->ProgramCounter - 1] - Instruction::istore_0);
                break;

            case Instruction::iload_0:
            case Instruction::iload_1:
            case Instruction::iload_2:
            case Instruction::iload_3:
                CurrentFrame->StackPointer++;
                PEEK = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::iload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled int %d out of local %d into the stack\n", PEEK.intVal, Code[CurrentFrame->ProgramCounter - 1] - Instruction::iload_0);
                break;

            case Instruction::aaload:
            case Instruction::baload:
            case Instruction::caload:
            case Instruction::saload:
            case Instruction::iaload:
            case Instruction::laload:
            case Instruction::faload:
            case Instruction::daload:
                UNDER =
                    _ObjectHeap.GetObjectPtr(UNDER.object)
                        [PEEK.intVal + 1];
                printf("Pulled value " PrtSizeT " (%.6f) out of the " PrtSizeT "th entry of array object " PrtSizeT "\n", UNDER.pointerVal, UNDER.floatVal, (size_t) PEEK.intVal + 1, UNDER.object.Heap);
			    CurrentFrame->StackPointer--;
			    CurrentFrame->ProgramCounter++;
			    break;

            case Instruction::fload_0:
            case Instruction::fload_1:
            case Instruction::fload_2:
            case Instruction::fload_3:
                CurrentFrame->StackPointer++;
                PEEK = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::fload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled float %.6f out of local %d into the stack\n", PEEK.floatVal, Code[CurrentFrame->ProgramCounter - 1] - Instruction::fload_0);
                break;

            case Instruction::imul:
                UNDER.intVal = 
                    UNDER.intVal
                    * PEEK.intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Multiplied the last two integers on the stack (result %d)\n", PEEK.intVal);
                break;

            case Instruction::iadd:
                UNDER.intVal = 
                    UNDER.intVal
                    + PEEK.intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Added the last two integers on the stack (%d + %d = %d)\r\n", UNDER.intVal, PEEK.intVal - UNDER.intVal, PEEK.intVal);
                break;

            case Instruction::isub:
                UNDER.intVal = 
                    PEEK.intVal
                    - UNDER.intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Subtracted the last two integers on the stack (%d - %d = %d)\r\n", 
                    OVER.intVal, 
                    OVER.intVal - PEEK.intVal, 
                    PEEK.intVal);
                break;

            case Instruction::irem:
                UNDER.intVal = 
                    UNDER.intVal
                    % PEEK.intVal;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Modulo'd the last two integers on the stack (result %d)\r\n",
                    PEEK.intVal);
                break;

            case Instruction::i2c:
                CurrentFrame->ProgramCounter++;
                printf("Int " PrtSizeT " converted to char.\n", PEEK.pointerVal);
                break;

            case Instruction::i2d:
                Long = PEEK.intVal;
                PEEK.doubleVal = (double) Long;
                CurrentFrame->ProgramCounter++;
                printf("Convert int " PrtSizeT " to double %.6f\n", Long, PEEK.doubleVal);
                break;

            case Instruction::ldc:
                CurrentFrame->Stack[++CurrentFrame->StackPointer] = GetConstant(CurrentFrame->_Class, (uint8_t) Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->ProgramCounter += 2;
                printf("Pushed constant %d (0x" PrtHex64 " / %.6f) to the stack. Below = " PrtSizeT "\n", Code[CurrentFrame->ProgramCounter - 1], PEEK.pointerVal, PEEK.floatVal, UNDER.pointerVal);
                break;

            case Instruction::ldc2_w:
                Index = ReadShortFromStream(&Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->StackPointer++;
                PEEK.pointerVal = ReadLongFromStream(&((char *)Class->Constants[Index])[1]);
                CurrentFrame->ProgramCounter += 3;

                printf("Pushed constant %d of type %d, value 0x" PrtHex64 " / %.6f onto the stack\n", Index, Class->Constants[Index]->Tag, PEEK.pointerVal, PEEK.doubleVal);
                break;

            case Instruction::ddiv: {
                double topVal = PEEK.doubleVal;
                double underVal = UNDER.doubleVal;
                double res = topVal / underVal;
                UNDER.doubleVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Divided the last two doubles on the stack (%.6f / %.6f = %.6f)\n", topVal, underVal, res);
                break;
            }

            case Instruction::_fdiv: {
                float topVal = PEEK.floatVal;
                float underVal = UNDER.floatVal;
                float res = topVal / underVal;
                UNDER.floatVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Divided the last two floats on the stack (%.6f / %.6f = %.6f)\n", topVal, underVal, res);
                break;
            }

            case Instruction::dstore_0:
            case Instruction::dstore_1:
            case Instruction::dstore_2:
            case Instruction::dstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - Instruction::dstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled double %.6f out of the stack into 1\n", OVER.doubleVal);
                break;

            case Instruction::fstore_0:
            case Instruction::fstore_1:
            case Instruction::fstore_2:
            case Instruction::fstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - Instruction::fstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled float %f out of the stack into local %d\n", UNDER.floatVal, Code[CurrentFrame->ProgramCounter - 1] - Instruction::fstore_0);
                break;

            case Instruction::dload_0:
            case Instruction::dload_1:
            case Instruction::dload_2:
            case Instruction::dload_3:
                CurrentFrame->StackPointer++;
                PEEK =
                    CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::dload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled %.6f out of local 1 into the stack\n", PEEK.doubleVal);
                break;

            case Instruction::aload_0:
            case Instruction::aload_1:
            case Instruction::aload_2:
            case Instruction::aload_3:
                CurrentFrame->StackPointer++;
                PEEK = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::aload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled object " PrtSizeT " out of local %d into the stack\n", PEEK.object.Heap, Code[CurrentFrame->ProgramCounter - 1] - Instruction::aload_0);
                break;

            case Instruction::astore_0:
            case Instruction::astore_1:
            case Instruction::astore_2:
            case Instruction::astore_3:
                CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::astore_0] = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled the last object on the stack into local %d\n", Code[CurrentFrame->ProgramCounter - 1] - Instruction::astore_0);
                break;


		    case Instruction::aastore:
			    _ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [UNDER.intVal + 1] =
                        PEEK;
                printf("Stored reference %d into the " PrtSizeT "th entry of array object " PrtSizeT ".\n", PEEK.intVal, (size_t) UNDER.intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    CurrentFrame->ProgramCounter++;
			    break;

            case Instruction::iastore:
            case Instruction::lastore:
            case Instruction::sastore:
            case Instruction::bastore:
            case Instruction::castore:
            	_ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [UNDER.intVal + 1] =
                        PEEK;
                printf("Stored number %d into the " PrtSizeT "th entry of array object " PrtSizeT ".\n", PEEK.intVal, (size_t) UNDER.intVal, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    CurrentFrame->ProgramCounter++;
			    break;

            case Instruction::fastore:
            case Instruction::dastore:
                _ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [UNDER.intVal + 1] =
                        PEEK;
                printf("Stored number (%.6f) into the " PrtSizeT "th entry of array object " PrtSizeT ".\n", PEEK.doubleVal, (size_t) UNDER.intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    CurrentFrame->ProgramCounter++;
			    break;

            case Instruction::_drem: {
                double topVal = PEEK.doubleVal;
                double underVal = UNDER.doubleVal;

                double res = fmod(topVal, underVal);
                UNDER.doubleVal = res;

                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Took the remainder of the last two doubles (%.6f and %.6f), result %.6f\n", topVal, underVal, res);
                break;
            }

            case Instruction::d2f: {
                double val = PEEK.doubleVal;
                float floatVal = (float) val;
                PEEK.floatVal = floatVal;
                CurrentFrame->ProgramCounter++;
                printf("Converted double %.6f to float %.6f\n", val, floatVal);
                break;
            }

            case Instruction::d2i: {
                double val = PEEK.doubleVal;
                int32_t intVal = (int) val; //*reinterpret_cast<int32_t*>(&val);
                PEEK.intVal = intVal;
                CurrentFrame->ProgramCounter++;
                printf("Converted double %.6f to int %d\n", val, intVal);
                break;
            }

            case Instruction::f2d: {
                float val = PEEK.floatVal;
                double doubleVal = (double) val;
                PEEK.doubleVal = doubleVal;
                CurrentFrame->ProgramCounter++;

                printf("Converted float %.6f to double %.6f\n", val, doubleVal);
                break;
            }

            case Instruction::f2i: {
                float val = PEEK.floatVal;
                int32_t intVal = (int) val; //*reinterpret_cast<int32_t*>(&val);
                PEEK.intVal = intVal;
                CurrentFrame->ProgramCounter++;
                printf("Converted float %.6f to int %d\n", val, intVal);
                break;
            }

            case Instruction::_fmul: {
                float val = PEEK.floatVal;
                float underVal =  UNDER.floatVal;
                float res = val * underVal;
                UNDER.floatVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Multiplied the last two floats on the stack (%.6f * %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            case Instruction::dmul: {
                double val = PEEK.doubleVal;
                double underVal =  UNDER.doubleVal;
                double res = val * underVal;
                UNDER.doubleVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Multiplied the last two doubles on the stack (%.6f * %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            case Instruction::dadd: {
                double val = PEEK.doubleVal;
                double underVal =  UNDER.doubleVal;
                double res = val + underVal;
                UNDER.doubleVal = res;
                CurrentFrame->StackPointer--;
                CurrentFrame->ProgramCounter++;
                printf("Added the last two doubles on the stack (%.6f + %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            case Instruction::fconst_0: // TODO
            case Instruction::fconst_1:
            case Instruction::fconst_2:
                OVER.floatVal = (float) 2.0F;
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Pushed 2.0F to the stack\n");
                break;

            case Instruction::dconst_0: // TODO
            case Instruction::dconst_1:
                OVER.doubleVal = (double) 1.0F;
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Pushed 1.0D to the stack\n");
                break;

            case Instruction::bipush:
                CurrentFrame->StackPointer++;
                PEEK.charVal = (uint8_t) Code[CurrentFrame->ProgramCounter + 1];
                CurrentFrame->ProgramCounter += 2;
                printf("Pushed char %d to the stack\n", PEEK.charVal);
                break;

            case Instruction::sipush:
                CurrentFrame->StackPointer++;
                PEEK.shortVal = ReadShortFromStream(Code + (CurrentFrame->ProgramCounter + 1));
                CurrentFrame->ProgramCounter += 3;
                printf("Pushed short %d to the stack\n", PEEK.shortVal);
                break;

            case Instruction::ifne: {
                bool NotEqual = PEEK.pointerVal != 0;
                printf("Comparing: " PrtInt64 " != 0\n", PEEK.pointerVal);
                printf("Integer equality comparison returned %s\n", NotEqual ? "true" : "false");

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

            case Instruction::if_icmpeq: {
                bool Equal = UNDER.pointerVal == PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " == " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
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

            case Instruction::if_icmpne: {
                bool NotEqual = UNDER.pointerVal != PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " != " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
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

            case Instruction::if_icmpgt: {
                bool GreaterThan = UNDER.pointerVal > PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " > " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
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

            case Instruction::if_icmplt: {
                bool LessThan = UNDER.pointerVal < PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " < " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
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

            case Instruction::if_icmpge: {
                bool GreaterEqual = UNDER.pointerVal >= PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " >= " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
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

            case Instruction::if_icmple: {
                bool LessEqual = UNDER.pointerVal <= PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " <= " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
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

            case Instruction::iinc: {
                uint8_t Index = Code[CurrentFrame->ProgramCounter + 1];
                uint8_t Offset = Code[CurrentFrame->ProgramCounter + 2];

                printf("Incrementing local %d by %d.\n", Index, Offset);
                CurrentFrame->Stack[Index].pointerVal += Offset;
                
                CurrentFrame->ProgramCounter += 3;
                break;
            }

            case Instruction::_goto: {
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

    Object newObj = Stack->_Class->CreateObject(Index, &_ObjectHeap);
    if(newObj == ObjectHeap::Null)
        return false;

    printf("New object is in ObjectHeap position " PrtSizeT ".\n", newObj.Heap);
    Stack->Stack[Stack->StackPointer].object = newObj;
    return true;
}

void Engine::NewArray(StackFrame* Stack) {
    size_t ArrayLength = Stack->Stack[Stack->StackPointer].intVal;
    uint8_t Type = Stack->_Method->Code->Code[Stack->ProgramCounter + 1];
    Stack->Stack[Stack->StackPointer].object = _ObjectHeap.CreateArray(Type, ArrayLength);
    printf("Initialized a " PrtSizeT "-wide array of type %d.\n", ArrayLength, Type);
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
    printf("ptr %d, stack - 1 = " PrtSizeT "\n", Stack->StackPointer, Stack->Stack[Stack->StackPointer - 1].pointerVal);

    uint16_t MethodIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    printf("Invoking a function.. index is %d.\r\n", MethodIndex);

    uint8_t* Constants = (uint8_t*) Stack->_Class->Constants[MethodIndex];

    char* TypeName = NULL;
    switch(Type) {
        case Instruction::invokeinterface:
            if(!(Constants[0] == TypeInterfaceMethod))
                exit(4);
            TypeName = (char*) "interface";
            break;
        case Instruction::invokevirtual:
        case Instruction::invokespecial:
            if(!(Constants[0] == TypeMethod))
                exit(4);
            TypeName = (char*) " instance";
            break;
        case Instruction::invokestatic:
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

    printf("\tMethod has " PrtSizeT " parameters, skipping ahead..\r\n", Parameters);
    
    // Make sure there's no weirdness if all we do is call a function
    Variable UnderStack;
    if(Stack->StackPointer <= (Parameters + 1))
        UnderStack = 0;
    else
        UnderStack = Stack->Stack[Stack->StackPointer - (Parameters + 1)];

    printf("\tSaved value " PrtSizeT " (object " PrtSizeT ") for restoration\r\n", UnderStack.pointerVal, UnderStack.object.Heap);

    std::vector<Variable> ParamList;

    for(size_t i = 0; i < Parameters; i++) {
        printf("\t\tParameter " PrtSizeT " = " PrtSizeT "\r\n", i, Stack->Stack[Stack->StackPointer - i].pointerVal);
        ParamList.push_back(Stack->Stack[Stack->StackPointer - i]);
    }

    Variable ClassInStack = Stack->Stack[Stack->StackPointer - Parameters];
    printf("\tClass to invoke is object #" PrtSizeT ".\r\n", ClassInStack.object.Heap);

    Variable* ObjectFromHeap = _ObjectHeap.GetObjectPtr(ClassInStack.object);
    printf("\tClass at 0x" PrtHex64 ".\r\n", ObjectFromHeap->pointerVal);
    class Class* VirtualClass = (class Class*) ObjectFromHeap->pointerVal;

    int MethodInClassIndex = VirtualClass->GetMethodFromDescriptor(MethodName.c_str(), MethodDesc.c_str(), ClassName.c_str(), VirtualClass);

    // 0x100 = native
    if(VirtualClass->Methods[MethodInClassIndex].Access & 0x100) {
        try {
            puts("Executing native method");
            NativeContext Context = { .InvocationMethod = Type, .ClassName = ClassName, .MethodName = MethodName, .MethodDescriptor = MethodDesc, .Parameters = ParamList, .ClassInstance = &ClassInStack.object};
            InvokeNative(Context);
        } catch (NativeReturn& e) {
            printf("Native: %s " PrtSizeT "\n", e.what(), e.Value.pointerVal);

            Stack->StackPointer -= ParamList.size();

            // Don't set return value if the function returned void.
            if(MethodDesc.find(")V") == std::string::npos)
                Stack->Stack[Stack->StackPointer] = e.Value;

            return;
        }

        puts("Impossible position. Investigate immediately.\n********************\n********************\n********************\n********************\n********************\n********************\n");
        exit(5);
    }

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
        printf("\tSetting \"this\" to " PrtSizeT ".\r\n", ClassInStack.object.Heap);
        Stack[1].Stack[ParameterCount] = ClassInStack;
        ParameterCount++;
    }

    for(size_t i = ParameterCount; i < Parameters; i++) {
        Stack[1].Stack[i] = Stack->Stack[Stack->StackPointer - (i - 1)];
        printf("\tSetting local " PrtSizeT " to " PrtSizeT ".\r\n", i, Stack[1].Stack[i].pointerVal);
    }

    printf("\tFunction's parameters start at " PrtSizeT ".\r\n", Parameters);

    Stack[1].StackPointer = Parameters;

    printf("Invoking method %s%s\n", MethodName.c_str(), MethodDesc.c_str());

    this->Ignite(&Stack[1]);
    Variable ReturnValue = Stack[1].Stack[Stack[1].StackPointer];

    if(MethodDesc.find(")V") != std::string::npos)
        Parameters--;

    Stack->StackPointer -= Parameters;

    printf("Shrinking the stack by " PrtSizeT " positions.\r\n", Parameters);

    int Offset = 0;
    if(MethodDesc.find(")V") == std::string::npos) {
        Stack->Stack[Stack->StackPointer] = ReturnValue;
        printf("Pushing function return value, " PrtSizeT "\r\n", ReturnValue.pointerVal);
        Offset--; // Make Offset -1, so that the line below works with return.
    }
    Stack->Stack[Stack->StackPointer + Offset] = UnderStack;
    printf("Restoring the value " PrtSizeT " under the function, just in case.\r\n", UnderStack.pointerVal);

}

uint16_t Engine::GetParameters(const char *Descriptor) {
    uint16_t Count = 0;
    size_t Length = strlen(Descriptor);

    for(size_t i = 1; i < Length; i++) {
        if(Descriptor[i] == '[')
            continue;

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
