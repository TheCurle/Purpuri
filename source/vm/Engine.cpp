/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Stack.hpp>
#include <vm/Class.hpp>
#include <vm/Native.hpp>

#include <vm/debug/Debug.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>

/**
 * This file implements the primary execution engine for Purpuri.
 * In other words, this is the actual interpreter.
 *
 * The interpreter is a static, one-step-at-a-time dumb interpreter.
 * It does no profiling, no JIT, no fancy stuff.
 *
 * It can only handle executing on one thread at a time, due to the usage of static fields all over.
 *
 * The core function here is Ignite (named after the ignition cycle of a 4-stroke engine), which contains the core loop.
 *
 * Instruction decoding is implemented as a large switch statement for the performance benefit of table-switch.
 * It is a single, large method for the performance benefit of not having to call & return.
 *
 * However, the code inside is still horrendously inefficient, so those are very small kickbacks.
 *
 * Call Frame management is also handled here, to the best of my ability to follow the JVM Spec.
 *
 * While the goal of Purpuri is to be as free of "magic" as possible, there is a reasonable limit.
 * To reduce code reuse (and the soul-crushing mind-grating unbearable tediousness of repeating the same code over and over)
 *  there are a few macros i use all over the place.
 *
 * A breakdown:
 *  PEEK: Read the stack at the current stack pointer
 *  UNDER: Read the stack under the current stack pointer
 *  OVER: Read the stack over the current stack pointer
 *  PCPLUS: Increment the Program Counter
 *
 * Additionally, macros in Common.hpp cause all calls to printf and puts to be wrapped in a check for Engine::QuietMode.
 * Therefore, if the -q flag is passed on startup, these calls will not be sent to the console.
 *
 * It would be wise to move more things here into macros, but that is a TODO.
 */

// Fetch a pointer to the item currently on the stack
#define PEEK \
    CurrentFrame->Stack[CurrentFrame->StackPointer]

// Fetch a pointer to the item under the stack pointer
#define UNDER \
    CurrentFrame->Stack[CurrentFrame->StackPointer - 1]

// Fetch a pointer to the item currently above the stack pointer
#define OVER \
    CurrentFrame->Stack[CurrentFrame->StackPointer + 1]

// Increase the program counter by the given amount.
// Use as "PCPLUS 1;".
#define PCPLUS \
    CurrentFrame->ProgramCounter +=

// The Stack of Variables used to emulate the local variables.
// A method will have a pointer to this stack on entry, and entries from the parent method will still be there.
// Thus, this needs to be carefully managed to ensure there is no underflow.
Variable* StackFrame::MemberStack;

Variable* StackFrame::ClassloadStack;

// The Object Heap of everything in the VM. See Objects.cpp for the implementation.
ObjectHeap Engine::_ObjectHeap;

// The static boolean flag that controls the print output level. If true, printf and puts across the code base will be passed to stdout.
bool Engine::QuietMode = false;
bool Engine::QuietFlag = false;

Engine::Engine() {
    _ClassHeap = nullptr;
    auto* Stack = new StackFrame[20];
    ClassloadingStack = Stack;

    // The frames need to each be initialized, since we only declared the list.
    for(size_t i = 0; i < 20; i++) {
        Stack[i] = StackFrame();
    }
}

Engine::~Engine() = default;

/**
 * A wrapper to execute a native (implemented in system-specific code) function.
 * The function will be searched for in a library named "libnative" in a bin folder in the working directory.
 * If the method is not found in the library, the VM will close.
 *
 * TODO: Allow programs to add native libraries to the list of searched.
 * @param Context pre-collected information about the method to be called, the parameters to it, and the class object that called it.
 */
void Engine::InvokeNative(const NativeContext& Context) {
    // First we need to properly obfuscate the name of the method to call.
    std::string FunctionName = Native::EncodeName(Context);
    printf("Function %s%s from class %s wants to call to function %s.\n", Context.MethodName.c_str(), Context.MethodDescriptor.c_str(), Context.ClassName.c_str(), FunctionName.c_str());

    // We then set the static field for the parameters of the current native call.
    // TODO: static field usage
    Native::Parameters = Context.Parameters;

    // The libnative library was loaded in EntryPoint.cpp, so we can just ask the Native module to search for the function for us.
    auto func = Native::LoadSymbol(NATIVES_FILE, FunctionName);

    // If the function wasn't found, we need to print and exit now, otherwise we're about to cause major system issues.
    // Basically every OS API is predicated on us not trying to execute code at NULLPTR with no system context.
    if(func == nullptr) {
        printf("\n\n*************\n*************\n*************\nFunction %s does not exist in any loaded library.\n*************\n*************\n*************\n", FunctionName.c_str());
        exit(7);
    }

    printf("Jumping to native function.\n");

    // LoadSymbol stores a function pointer, so we can jump to it here.
    // The function takes no arguments, it retrieves system context from the PNI API.
    func();
}

/**
 * The core loop, as explained above.
 *
 * Given a method, it will read the bytecode and interpret instructions as it goes.
 * If the method calls another method, it will create a new StackFrame and recurse until all methods return.
 *
 * It does not detect unbounded recursion, this is a TODO.
 */
uint32_t Engine::Ignite(StackFrame* Stack, bool clinit) {
    // If we call a method, Stack is advanced and a new pointer passed, so we need to keep a reference of our base here.
    StackFrame* CurrentFrame = &Stack[0];
    printf("New execution frame generated.\n");

    this->clinit = clinit;

    // We're going to be executing bytecode, so it's a good idea to fetch the current state of the code here.
    uint8_t* Code = CurrentFrame->_Method->Code->Code + CurrentFrame->ProgramCounter;
    // Error is used to keep track of things that go wrong during execution. Invariably non-fatal, and it's fun to see how wrong it can go from a simple error.
    //int32_t Error = 0;

    // Some metadata for debug printing.
    Class* Class = CurrentFrame->_Class;
    std::string Name = Class->GetStringConstant(CurrentFrame->_Method->Name);

    printf("Executing %s::%s\n", Class->GetClassName().c_str(), Name.c_str());

    // We need to refer to these a lot, and switch cases can't redefine variables, so they're all up here.
    int32_t Index;
    size_t Long;

    // This loop can only be broken by a method return.
    while(true) {

        // Before every instruction, we should notify the debugger what's happening.
        // The DEBUG macro only executes the containing code if the Visual Debugger is selected for compilation.
        DEBUG({
            // Set the stack (including the program counter)
            Debugger::SetStack(CurrentFrame);
            // Create a new lock on the debugger so that we can transfer the data
            std::unique_lock<std::mutex> lock(Debugger::Locker);
            // Wait for the data to change (user input, basically) before progressing
            Debugger::Notifier.wait(lock, []{return Debugger::ShouldStep;});
            // Reset to 0 so that we can wait for more data later
            Debugger::ShouldStep = false;
        })

        printf("%d: ", Code[CurrentFrame->ProgramCounter]);

        switch(Code[CurrentFrame->ProgramCounter]) {
            // No-op; skip the instruction.
            case Instruction::noop: PCPLUS 1; break;

            // return; pass execution back to the caller method. Typically used in void methods.
            // Purpuri: pass data on the stack back to the caller method.
            case Instruction::_return:
                puts("Function returns"); return 0;

            // ireturn; pass an int on the stack back to the caller method.
            // Purpuri: pass data on the stack back to the caller method.
            case Instruction::ireturn:
                fprintf(stderr, "\n******************\n\nFunction returned value %d\n\n", PEEK.intVal);
                return 0;

            // new: create a new object instance.
            // Purpuri: see the New function in this file for more details.
            case Instruction::_new:
                if(!New(CurrentFrame)) {
                    printf("Creating new object failed.\r\n");
                    exit(5);
                }

                PCPLUS 3;
                printf("Initialized new object\n");
                break;

            // arraylength: push the size in entries of the array currently on the stack
            // Purpuri: push(pop().size())
            case Instruction::arraylength:
                PEEK.pointerVal
                    = _ObjectHeap.GetArraySize(PEEK.object);
                printf("Got the size " PrtSizeT " from the array just pushed.\n", PEEK.pointerVal);
                PCPLUS 1;
                break;

            // newarray: allocate and push a new array of the given type.
            // Purpuri: see the NewArray function in this file for more details.
            case Instruction::newarray:
                printf("New array initializing:\n");
                NewArray(CurrentFrame);
                PCPLUS 2;
                printf("Initialized new array\n");
                break;

            // anewarray: allocate and push a new array of references.
            // Purpuri: see the ANewArray function in this file for more details.
            case Instruction::anewarray:
                ANewArray(CurrentFrame);
                PCPLUS 2;
                printf("Initialized new a-array\n");
                break;

            // bcdup: duplicate the reference on the stack.
            // Purpuri: push(peek())
            case Instruction::bcdup:
                OVER = PEEK;
                // The compiler may re-organize these two calls; this ensures there's always some sort of ordering.
                PEEK.object = OVER.object;

                CurrentFrame->StackPointer++;
                PCPLUS 1;
                printf("Duplicated the last item on the stack\n");
                break;

            // invokespecial: call methods without dynamic binding, for e.g. constructors and superclass methods.
            case Instruction::invokespecial:
                Invoke(CurrentFrame, Instruction::invokespecial);
                PCPLUS 3;
                break;

            // invokevirtual: standard method invoke.
            case Instruction::invokevirtual:
                Invoke(CurrentFrame, Instruction::invokevirtual);
                PCPLUS 3;
                break;

            // invokeinterface: call a method from an interface; either on the class, a parent class, or the interface itself.
            case Instruction::invokeinterface:
                Invoke(CurrentFrame, Instruction::invokeinterface);
                PCPLUS 5;
                break;

            // invokestatic: call a static method.
            case Instruction::invokestatic:
                Invoke(CurrentFrame, Instruction::invokestatic);
                PCPLUS 3;
                break;

            // putstatic: place data into a static field.
            case Instruction::putstatic:
                PutStatic(CurrentFrame);
                CurrentFrame->StackPointer--;
                PCPLUS 3;
                break;

            // putfield: place data into an instance field.
            case Instruction::putfield:
                PutField(CurrentFrame);
                CurrentFrame->StackPointer -= 2;
                PCPLUS 3;
                break;

            // getstatic: read data from a static field
            case Instruction::getstatic:
                GetStatic(CurrentFrame);
                CurrentFrame->StackPointer++;
                PCPLUS 3;
                printf("Got value " PrtSizeT ".\n", Stack->Stack[Stack->StackPointer].pointerVal);
                break;

            // getfield: read data from an instance field
            case Instruction::getfield:
                GetField(CurrentFrame);
                PCPLUS 3;
                printf("Retrieved value " PrtSizeT " from field.\r\n", PEEK.pointerVal);
                break;

            // istore, lstore: store an int or long into a local variable
            case Instruction::istore:
            case Instruction::lstore:
			    CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter + 1]] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                printf("Stored value " PrtSizeT " in local %d.\n", PEEK.pointerVal, Code[CurrentFrame->ProgramCounter + 1]);
                PCPLUS 2;
			    break;

            // fstore, dstore: store a float or double to a local variable
            case Instruction::fstore:
            case Instruction::dstore:
			    CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter + 1]] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                printf("Stored value %.6f in local %d.\n", PEEK.doubleVal, Code[CurrentFrame->ProgramCounter + 1]);
		    	CurrentFrame->ProgramCounter += 2;
			    break;

            // iconst_x: push integer constant x to the stack. m1 = minus 1 (-1)
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

            // istore_x: store the int on the top of the stack into local x.
            // Purpuri: locals[x] = (int) pop()
            case Instruction::istore_0:
            case Instruction::istore_1:
            case Instruction::istore_2:
            case Instruction::istore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - Instruction::istore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                PCPLUS 1;
                printf("Pulled int %d out of the stack into local %d\n", OVER.intVal, (uint8_t)Code[CurrentFrame->ProgramCounter - 1] - Instruction::istore_0);
                break;

            // iload_x: store the int in local x on the stack.
            // Purpuri: push((int) locals[x])
            case Instruction::iload_0:
            case Instruction::iload_1:
            case Instruction::iload_2:
            case Instruction::iload_3:
                CurrentFrame->StackPointer++;
                PEEK = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::iload_0];
                PCPLUS 1;
                printf("Pulled int %d out of local %d into the stack\n", PEEK.intVal, Code[CurrentFrame->ProgramCounter - 1] - Instruction::iload_0);
                break;

            // xaload: read the index and array from the stack in that order, and interpret the value of the array at that index as the type x.
            // Purpuri: int idx = pop(); push(pop()[idx])
            // Purpuri: a = object b = byte c = char s = short i = int l = long f = float d = double
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
                PCPLUS 1;
			    break;

            // fload_x: store the float in local x on the stack.
            // Purpuri: push((float) locals[x])
            case Instruction::fload_0:
            case Instruction::fload_1:
            case Instruction::fload_2:
            case Instruction::fload_3:
                CurrentFrame->StackPointer++;
                PEEK = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::fload_0];
                PCPLUS 1;
                printf("Pulled float %.6f out of local %d into the stack\n", PEEK.floatVal, Code[CurrentFrame->ProgramCounter - 1] - Instruction::fload_0);
                break;

            // imul: multiply the two integers on the stack
            // Purpuri: push(pop() * pop())
            case Instruction::imul:
                UNDER.intVal = 
                    UNDER.intVal
                    * PEEK.intVal;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Multiplied the last two integers on the stack (result %d)\n", PEEK.intVal);
                break;

            // iadd: add the two integers on the stack
            // Purpuri: push(pop() + pop())
            case Instruction::iadd:
                UNDER.intVal = 
                    UNDER.intVal
                    + PEEK.intVal;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Added the last two integers on the stack (%d + %d = %d)\r\n", UNDER.intVal, PEEK.intVal - UNDER.intVal, PEEK.intVal);
                break;

            // isub: subtract the two integers on the stack
            // Purpuri: push(pop() - pop())
            case Instruction::isub:
                UNDER.intVal = 
                    PEEK.intVal
                    - UNDER.intVal;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Subtracted the last two integers on the stack (%d - %d = %d)\r\n", 
                    OVER.intVal, 
                    OVER.intVal - PEEK.intVal, 
                    PEEK.intVal);
                break;

            // irem: push the remainder left after dividing the two integers on the stack
            // Purpuri: push(pop() % pop())
            case Instruction::irem:
                UNDER.intVal = 
                    UNDER.intVal
                    % PEEK.intVal;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Modulo'd the last two integers on the stack (result %d)\r\n",
                    PEEK.intVal);
                break;

            // i2c: convert int to char
            // Purpuri: (char) pop()
            case Instruction::i2c:
                PCPLUS 1;
                printf("Int " PrtSizeT " converted to char.\n", PEEK.pointerVal);
                break;

            // i2d: convert int to double
            // Purpuri: (double) pop()
            case Instruction::i2d:
                Long = PEEK.intVal;
                PEEK.doubleVal = (double) Long;
                PCPLUS 1;
                printf("Convert int " PrtSizeT " to double %.6f\n", Long, PEEK.doubleVal);
                break;

            // ldc: load constant onto stack
            // Purpuri: push(this.class.constants[this.code[PC + 1]])
            case Instruction::ldc:
                CurrentFrame->Stack[++CurrentFrame->StackPointer] = GetConstant(CurrentFrame->_Class, (uint8_t) Code[CurrentFrame->ProgramCounter + 1]);
                PCPLUS 2;
                printf("Pushed constant %d (0x" PrtHex64 " / %.6f) to the stack. Below = " PrtSizeT "\n", Code[CurrentFrame->ProgramCounter - 1], PEEK.pointerVal, PEEK.floatVal, UNDER.pointerVal);
                break;

            // ldc2_w: load double-wide constant onto stack
            // Purpuri: push(this.class.constants[this.code[PC + 1]])
            case Instruction::ldc2_w:
                Index = ReadShortFromStream(&Code[CurrentFrame->ProgramCounter + 1]);
                CurrentFrame->StackPointer++;
                PEEK.pointerVal = ReadLongFromStream(&((char *)Class->Constants[Index])[1]);
                PCPLUS 3;

                printf("Pushed constant %d of type %d, value 0x" PrtHex64 " / %.6f onto the stack\n", Index, Class->Constants[Index]->Tag, PEEK.pointerVal, PEEK.doubleVal);
                break;

            // ddiv: divide the two doubles on the stack.
            case Instruction::ddiv: {
                double topVal = PEEK.doubleVal;
                double underVal = UNDER.doubleVal;
                double res = topVal / underVal;
                UNDER.doubleVal = res;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Divided the last two doubles on the stack (%.6f / %.6f = %.6f)\n", topVal, underVal, res);
                break;
            }

            // fdiv: divide the two floats on the stack.
            case Instruction::_fdiv: {
                float topVal = PEEK.floatVal;
                float underVal = UNDER.floatVal;
                float res = topVal / underVal;
                UNDER.floatVal = res;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Divided the last two floats on the stack (%.6f / %.6f = %.6f)\n", topVal, underVal, res);
                break;
            }

            // dstore_x: store the double on the stack into local x.
            case Instruction::dstore_0:
            case Instruction::dstore_1:
            case Instruction::dstore_2:
            case Instruction::dstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - Instruction::dstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                PCPLUS 1;
                printf("Pulled double %.6f out of the stack into %d\n", OVER.doubleVal, (uint8_t)Code[CurrentFrame->ProgramCounter - 1] - Instruction::dstore_0);
                break;

            // fstore_x: store the float on the stack into local x.
            case Instruction::fstore_0:
            case Instruction::fstore_1:
            case Instruction::fstore_2:
            case Instruction::fstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - Instruction::fstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                PCPLUS 1;
                printf("Pulled float %f out of the stack into local %d\n", UNDER.floatVal, Code[CurrentFrame->ProgramCounter - 1] - Instruction::fstore_0);
                break;

            // dload_x: push double local x onto the stack.
            case Instruction::dload_0:
            case Instruction::dload_1:
            case Instruction::dload_2:
            case Instruction::dload_3:
                CurrentFrame->StackPointer++;
                PEEK =
                    CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::dload_0];
                PCPLUS 1;
                printf("Pulled %.6f out of local 1 into the stack\n", PEEK.doubleVal);
                break;

            // aload_x: push reference local x onto the stack.
            case Instruction::aload_0:
            case Instruction::aload_1:
            case Instruction::aload_2:
            case Instruction::aload_3:
                CurrentFrame->StackPointer++;
                PEEK = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::aload_0];
                PCPLUS 1;
                printf("Pulled object " PrtSizeT " out of local %d into the stack\n", PEEK.object.Heap, Code[CurrentFrame->ProgramCounter - 1] - Instruction::aload_0);
                break;

            // astore_x: store the reference on the stack into local x.
            case Instruction::astore_0:
            case Instruction::astore_1:
            case Instruction::astore_2:
            case Instruction::astore_3:
                CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - Instruction::astore_0] = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                PCPLUS 1;
                printf("Pulled the last object on the stack into local %d\n", Code[CurrentFrame->ProgramCounter - 1] - Instruction::astore_0);
                break;


            // aastore: store the reference object on the stack into the array on the stack.
		    case Instruction::aastore:
			    _ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [UNDER.intVal + 1] =
                        PEEK;
                printf("Stored reference %d into the " PrtSizeT "th entry of array object " PrtSizeT ".\n", PEEK.intVal, (size_t) UNDER.intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    PCPLUS 1;
			    break;

            // [ialsbc]astore: store the integer data on the stack into the array on the stack.
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
			    PCPLUS 1;
			    break;

            // [fd]astore: store the floating point number on the stack into the array on the stack.
            case Instruction::fastore:
            case Instruction::dastore:
                _ObjectHeap.GetObjectPtr(CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object)
                    [UNDER.intVal + 1] =
                        PEEK;
                printf("Stored number (%.6f) into the " PrtSizeT "th entry of array object " PrtSizeT ".\n", PEEK.doubleVal, (size_t) UNDER.intVal + 1, CurrentFrame->Stack[CurrentFrame->StackPointer - 2].object.Heap);
			    CurrentFrame->StackPointer -= 3;
			    PCPLUS 1;
			    break;

            // drem: push the remainder of the division of the two doubles on the stack.
            case Instruction::_drem: {
                double topVal = PEEK.doubleVal;
                double underVal = UNDER.doubleVal;

                double res = fmod(topVal, underVal);
                UNDER.doubleVal = res;

                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Took the remainder of the last two doubles (%.6f and %.6f), result %.6f\n", topVal, underVal, res);
                break;
            }

            // d2f: convert the double on the stack into a float.
            case Instruction::d2f: {
                double val = PEEK.doubleVal;
                auto floatVal = (float) val;
                PEEK.floatVal = floatVal;
                PCPLUS 1;
                printf("Converted double %.6f to float %.6f\n", val, floatVal);
                break;
            }

            // d2i: convert the double on the stack into an int
            case Instruction::d2i: {
                double val = PEEK.doubleVal;
                auto intVal = (int) val;
                PEEK.intVal = intVal;
                PCPLUS 1;
                printf("Converted double %.6f to int %d\n", val, intVal);
                break;
            }

            // f2d: convert the float on the stack into a double
            case Instruction::f2d: {
                float val = PEEK.floatVal;
                auto doubleVal = (double) val;
                PEEK.doubleVal = doubleVal;
                PCPLUS 1;

                printf("Converted float %.6f to double %.6f\n", val, doubleVal);
                break;
            }

            // f2i: convert the float on the stack into an int
            case Instruction::f2i: {
                float val = PEEK.floatVal;
                auto intVal = (int) val;
                PEEK.intVal = intVal;
                PCPLUS 1;
                printf("Converted float %.6f to int %d\n", val, intVal);
                break;
            }

            // fmul: multiply the two floats on the stack
            case Instruction::_fmul: {
                float val = PEEK.floatVal;
                float underVal =  UNDER.floatVal;
                float res = val * underVal;
                UNDER.floatVal = res;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Multiplied the last two floats on the stack (%.6f * %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            // dmul: multiply the two doubles on the stack
            case Instruction::dmul: {
                double val = PEEK.doubleVal;
                double underVal =  UNDER.doubleVal;
                double res = val * underVal;
                UNDER.doubleVal = res;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Multiplied the last two doubles on the stack (%.6f * %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            // dadd: add the two doubles on the stack
            case Instruction::dadd: {
                double val = PEEK.doubleVal;
                double underVal =  UNDER.doubleVal;
                double res = val + underVal;
                UNDER.doubleVal = res;
                CurrentFrame->StackPointer--;
                PCPLUS 1;
                printf("Added the last two doubles on the stack (%.6f + %.6f = %.6f)\n", val, underVal, res);
                break;
            }

            // fconst_x: push the constant x in floating point representation to the stack.
            case Instruction::fconst_0:
            case Instruction::fconst_1:
            case Instruction::fconst_2:
                OVER.floatVal = (float) 2.0F;
                CurrentFrame->StackPointer++;
                PCPLUS 1;
                printf("Pushed 2.0F to the stack\n");
                break;

            // dconst_x: push the constant x in double-precision floating point representation to the stack.
            case Instruction::dconst_0:
            case Instruction::dconst_1:
                OVER.doubleVal = (double) 1.0F;
                CurrentFrame->StackPointer++;
                PCPLUS 1;
                printf("Pushed 1.0D to the stack\n");
                break;

            // bipush: push the integer type "byte" of a certain value to the stack.
            case Instruction::bipush:
                CurrentFrame->StackPointer++;
                PEEK.charVal = (uint8_t) Code[CurrentFrame->ProgramCounter + 1];
                PCPLUS 2;
                printf("Pushed char %d to the stack\n", PEEK.charVal);
                break;

            // sipush: push the integer type "short" of a certain value to the stack.
            case Instruction::sipush:
                CurrentFrame->StackPointer++;
                PEEK.shortVal = ReadShortFromStream(Code + (CurrentFrame->ProgramCounter + 1));
                PCPLUS 3;
                printf("Pushed short %d to the stack\n", PEEK.shortVal);
                break;

            // ifne: if value on stack not equal to 0, jump to specified location
            case Instruction::ifne: {
                bool NotEqual = PEEK.pointerVal != 0;
                printf("Comparing: " PrtInt64 " != 0\n", PEEK.pointerVal);
                printf("Integer equality comparison returned %s\n", NotEqual ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                // Bytecode stores our destination, but just skip past it if we want to continue on our code path
                if(NotEqual) {
                    int Offset = (Code[CurrentFrame->ProgramCounter + 1] << 8) | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    PCPLUS Offset;
                    CurrentFrame->ProgramCounter += Offset;
                } else {
                    PCPLUS 3;
                }
                break;
            }

            // if_icmpeq: if the integer comparison of the stack determines they're equal, jump to specified location
            case Instruction::if_icmpeq: {
                bool Equal = UNDER.pointerVal == PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " == " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
                printf("Integer equality comparison returned %s\n", Equal ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                // Bytecode stores our destination, but just skip past it if we want to continue on our code path
                if(Equal) {
                    int Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    PCPLUS Offset;
                } else {
                    PCPLUS 3;
                }
                break;
            }

            // if_icmpne: if the integer comparison of the stack determines they're not equal, jump to specified location
            case Instruction::if_icmpne: {
                bool NotEqual = UNDER.pointerVal != PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " != " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
                printf("Integer inequality comparison returned %s\n", NotEqual ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                // Bytecode stores our destination, but just skip past it if we want to continue on our code path
                if(NotEqual) {
                    int Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    PCPLUS Offset;
                } else {
                    PCPLUS 3;
                }
                break;
            }

            // if_icmpgt: if integer under the stack is greater than the integer on the stack, jump to specified location
            case Instruction::if_icmpgt: {
                bool GreaterThan = UNDER.pointerVal > PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " > " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
                printf("Integer greater-than comparison returned %s\n", GreaterThan ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                // Bytecode stores our destination, but just skip past it if we want to continue on our code path
                if(GreaterThan) {
                    int Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    PCPLUS Offset;
                } else {
                    PCPLUS 3;
                }
                break;
            }

            // if_icmplt: if integer under the stack is less than the integer on the stack, jump to specified location
            case Instruction::if_icmplt: {
                bool LessThan = UNDER.pointerVal < PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " < " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
                printf("Integer less-than comparison returned %s\n", LessThan ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                // Bytecode stores our destination, but just skip past it if we want to continue on our code path
                if(LessThan) {
                    int Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    PCPLUS Offset;
                } else {
                    PCPLUS 3;
                }
                break;
            }

            // if_icmpge: if integer under the stack is greater or equal to than the integer on the stack, jump to specified location
            case Instruction::if_icmpge: {
                bool GreaterEqual = UNDER.pointerVal >= PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " >= " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
                printf("Integer greater-than-or-equal comparison returned %s\n", GreaterEqual ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                // Bytecode stores our destination, but just skip past it if we want to continue on our code path
                if(GreaterEqual) {
                    int Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    PCPLUS Offset;
                } else {
                    PCPLUS 3;
                }
                break;
            }

            // if_icmplt: if integer under the stack is less than or equal to the integer on the stack, jump to specified location
            case Instruction::if_icmple: {
                bool LessEqual = UNDER.pointerVal <= PEEK.pointerVal;
                printf("Comparing: " PrtInt64 " <= " PrtInt64 "\n", UNDER.pointerVal, PEEK.pointerVal);
                printf("Integer less-than-or-equal comparison returned %s\n", LessEqual ? "true" : "false");

                CurrentFrame->StackPointer -= 2;

                // Bytecode stores our destination, but just skip past it if we want to continue on our code path
                if(LessEqual) {
                    int Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                    printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                    PCPLUS Offset;
                } else {
                    PCPLUS 3;
                }
                break;
            }

            // iinc: increment a local variable by a specified amount.
            case Instruction::iinc: {
                uint8_t LocalIndex = Code[CurrentFrame->ProgramCounter + 1];
                uint8_t Offset = Code[CurrentFrame->ProgramCounter + 2];

                printf("Incrementing local %d by %d.\n", LocalIndex, Offset);
                CurrentFrame->Stack[LocalIndex].pointerVal += Offset;
                
                PCPLUS 3;
                break;
            }

            // goto: jump to a specified location in bytecode and continue executing from there
            case Instruction::_goto: {
                int Offset = (Code[CurrentFrame->ProgramCounter + 1]) << 8 | (Code[CurrentFrame->ProgramCounter + 2]);
                printf("Jumping to (%d + %d) = %d\n", CurrentFrame->ProgramCounter, Offset, CurrentFrame->ProgramCounter + Offset);
                PCPLUS Offset;
                break;
            }

            default: printf("\nUnhandled opcode 0x%x\n", Code[CurrentFrame->ProgramCounter]); PCPLUS 1; return false;
        }
    }
}

/**
 * A simple wrapper to fetch the Variable representation of a Constant Pool entry.
 *
 * This lives here, because Classes should be transparent to the inner workings of the interpreter, such as things like Variables.
 * It has handling for a few edge-cases regarding C++ union handling.
 * Namely, we need to make sure to match the input byte width with the data we expected to read.
 *
 * Other than that, a Variable is passed out of here as one would expect.
 * @param Class the class to read the constant from
 * @param Index the index into the class' Constant Pool.
 * @return the Variable representation of the constant.
 */
Variable Engine::GetConstant(Class *Class, uint8_t Index) const {
    Variable temp;
    temp.pointerVal = 0;

    char* Code = (char*) Class->Constants[Index];
    uint16_t shortTemp;
    Object obj {};

    switch(Code[0]) {
        // Floats and Integers are single-wide.
        case TypeFloat:
        case TypeInteger:
            temp.intVal = ReadIntFromStream(&Code[1]);
            break;

        // Strings are arbitrary width
        case TypeString:
            shortTemp = ReadShortFromStream(&Code[1]);
            obj = _ObjectHeap.CreateString(Class->GetStringConstant(shortTemp), _ClassHeap);
            temp.pointerVal = obj.Heap;
            break;

        // Doubles and longs are double-wide
        case TypeDouble:
        case TypeLong:
            temp.pointerVal = ReadLongFromStream(&Code[1]);
            break;
    }

    return temp;
}

/**
 * Create a new instance of an object and push it to the stack.
 * The object to be created is referenced by the Constants Pool.
 *
 * The new instruction is followed by two bytes for the Constant Pool ID.
 * These can be used to reference the constant for construction of the new Object.
 *
 * Once the Object Pool has dealt with the allocation and construction of the new Object, it is emplaced on the stack
 * above  the current Stack Pointer.
 *
 * @param Stack the StackFrame of the currently executing method.
 * @return 0 if something went wrong, 1 otherwise
 */
int Engine::New(StackFrame *Stack) {
    auto Index = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    Object newObj = Stack->_Class->CreateObject(Index, &_ObjectHeap);
    if(newObj == ObjectHeap::Null)
        return false;

    printf("New object is in ObjectHeap position " PrtSizeT ".\n", newObj.Heap);
    Stack->Stack[++Stack->StackPointer].object = newObj;
    return true;
}

/**
 * Create a new array of primitives and push it to the stack.
 * The length of the array is stored on the stack.
 * The type of the array is stored in the bytecode.
 *
 * The newarray instruction is followed by one byte for the array type.
 *
 * Once the Object Pool has dealt with the allocation and construction of the new array, it is emplaced on the stack
 * at the current Stack Pointer.
 *
 * @param Stack the StackFrame of the currently executing method.
 * @return 0 if something went wrong, 1 otherwise
 */
void Engine::NewArray(StackFrame* Stack) {
    // Reading this is equivalent to stack.pop()
    size_t ArrayLength = Stack->Stack[Stack->StackPointer].intVal;
    uint8_t Type = Stack->_Method->Code->Code[Stack->ProgramCounter + 1];
    // Setting this is equivalent to stack.push()
    Stack->Stack[Stack->StackPointer].object = _ObjectHeap.CreateArray(Type, ArrayLength);
    printf("Initialized a " PrtSizeT "-wide array of type %d.\n", ArrayLength, Type);
}

/**
 * Create a new array of Object references and push it to the stack.
 * The length of the array is stored on the stack.
 * The type of the array is stored in the bytecode.
 *
 * The anewarray instruction is followed by two bytes for the array type.
 * These can be used to reference the constant for construction of the new Object.
 *
 * Once the Object Pool has dealt with the allocation and construction of the new array, it is emplaced on the stack
 * at the current Stack Pointer.
 *
 * @param Stack the StackFrame of the currently executing method.
 * @return 0 if something went wrong, 1 otherwise
 */
void Engine::ANewArray(StackFrame* Stack) {
    auto Index = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);
    uint32_t Count = Stack->Stack[Stack->StackPointer].intVal; // pop

    if(!Stack->_Class->CreateObjectArray(Index, Count, _ObjectHeap, Stack->Stack[++Stack->StackPointer].object)) // push
        // TODO: ERROR
        printf("Initializing array failed.");
    printf("Initialized a %d-wide array of objects.\n", Count);
}

/**
 * Easily the single most complex function in the VM.
 * Handles calling other methods from within methods, preserving state in the process.
 *
 * It is a monolithic function, and handles all invoke types (special, virtual, interface, static).
 *
 * The method to invoke is indicated by a Constant Pool ID stored in the bytecode immediately after the invoke instruction.
 * The method contains the name and class of origin of the method.
 *
 * This is made tricky by the fact that interface methods choose their interface as the origin, even if there's no default implementation.
 * Therefore, we still have to do some searching ourselves.
 *
 * @param Stack Reference information about the Stack Frame currently executing
 * @param Type the Type of the method call (an index into Instructions)
 */
void Engine::Invoke(StackFrame *Stack, uint16_t Type) {
    auto MethodIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    printf("Invoking a function.. index is %d.\r\n", MethodIndex);
    auto* Constants = (uint8_t*) Stack->_Class->Constants[MethodIndex];

    // This next block is for sanity checking and debug output.
    // Since there's no sane way to switch-add a string to a log without duplicating or using a std::string wrapper, we kill two birds with one stone here.
    char* TypeName = nullptr;
    switch(Type) {
        // Interface invokes must point to an Interface Method.
        case Instruction::invokeinterface:
            if(Constants[0] != TypeInterfaceMethod)
                exit(4);
            TypeName = (char*) "interface";
            break;
        // Virtual and Special invokes must point to a regular method.
        case Instruction::invokevirtual:
        case Instruction::invokespecial:
            if(Constants[0] != TypeMethod)
                exit(4);
            TypeName = (char*) " instance";
            break;
        // Static invokes must also point to a regular method, but there's special handling for later.
        case Instruction::invokestatic:
            if(Constants[0] != TypeMethod)
                exit(4);
            TypeName = (char*) "   static";
            break;
        default:
            printf("\tUnknown invocation: %d as %d\r\n", Constants[0], Type);
            break;
    }

    printf("\tResolving invocation for %s function.\n", TypeName);

    auto ClassIndex = ReadShortFromStream(&Constants[1]);
    auto ClassNTIndex = ReadShortFromStream(&Constants[3]);

    // Now that we have the index of the class and name type that we want to invoke, we can resolve them to strings.
    std::string ClassName = Stack->_Class->GetStringConstant(ClassIndex);
    printf("\tInvocation is calling into class %s\n", ClassName.c_str());

    // Swap over to the NameAndType of the method and continue.
    Constants = (uint8_t*) Stack->_Class->Constants[ClassNTIndex];

    Method MethodToInvoke {};
    MethodToInvoke.Name = ReadShortFromStream(&Constants[1]);
    MethodToInvoke.Descriptor = ReadShortFromStream(&Constants[3]);
    MethodToInvoke.Access = 0; // Access requires more processing later.

    std::string MethodName = Stack->_Class->GetStringConstant(MethodToInvoke.Name);
    std::string MethodDesc = Stack->_Class->GetStringConstant(MethodToInvoke.Descriptor);
    printf("\tInvocation resolves to method %s%s\n", MethodName.c_str(), MethodDesc.c_str());

    // Grab return type
    std::string Substr;
    // NAME(PARAMPARAM)RETURN
    // Ljava/lang/String; [C B D L Z J I S F V
    Substr = MethodDesc.substr(MethodDesc.find(')') + 1);
    std::string ReturnType;
    switch (Substr.at(0)) {
        case 'L': {
            int i = 1;
            while (Substr.at(i) != ';') {
                char byte = Substr.at(i);
                ReturnType.append(&byte);
                i++;
            }
            break;
        }
        default:
            break;
    }

    printf("Method returns a %s, type is %s\r\n", Substr.c_str(), ReturnType.c_str());
    if (!ReturnType.empty() && !_ClassHeap->ClassExists(ReturnType)) {
        printf("Classloading return type.\r\n");
        auto* c = new Class();
        QUIET(
            _ClassHeap->LoadClass(ReturnType.c_str(), c, ClassloadingStack, this);
        )
    }

    // 5 -> (Ljava/lang/Object;Ljava/lang/String;)V

    // Grab Parameter Types
    Substr = MethodDesc.substr(MethodDesc.find('(') + 1);
    std::vector<std::string> types;
    int i = 0;
    char c;
    std::string type;
    while ((c = Substr.at(i)) != ')') {
        if (c == 'L') {
            i++;
            while (Substr.at(i) != ';') {
                char byte = Substr.at(i);
                type.append(&byte);
                i++;
            }
            types.emplace_back(type);
        }
        i++;
    }

    for (const std::string& t : types) {
        printf("Parameter has type %s\r\n", t.c_str());
        if (!t.empty() && !_ClassHeap->ClassExists(t.c_str())) {
            printf("Classloading parameter type.\r\n");
            auto* cl = new Class();
            _ClassHeap->LoadClass(t.c_str(), cl);
        }
    }

    // The biggest pain here is that we need to synchronize parameters between the caller and callee.
    // Prepare now by counting the parameters in the descriptor.
    size_t Parameters = GetParameters(MethodDesc.c_str());

    printf("\tMethod has " PrtSizeT " parameters, skipping ahead..\r\n", Parameters);
    
    // Sometimes the stack can be corrupted on the edge between caller and callee return.
    // By storing the value under the stack pointer (for non-void methods) we can hope to restore the proper
    // value to them. This is not a guarantee, and is indeed an indicator of a larger logic fault at hand, but I've
    // been unable to find said larger fault yet.
    // This is a hack, but it works, and it solves a really annoying problem.
    Variable UnderStack;
    if(Stack->StackPointer <= (Parameters + 1))
        UnderStack = 0;
    else
        UnderStack = Stack->Stack[Stack->StackPointer - (Parameters + 1)];
    printf("\tSaved value " PrtSizeT " (object " PrtSizeT ") for restoration\r\n", UnderStack.pointerVal, UnderStack.object.Heap);

    // Now that we know how many parameters there are, we can peel them from the stack and put them in this list.
    // We do this because the stack has an inverted order relative to how the method wants them, and the difference is
    // irreconcilable.
    std::vector<Variable> ParamList;
    for(size_t i = 0; i < Parameters; i++) {
        printf("\t\tParameter " PrtSizeT " = " PrtSizeT "\r\n", i, Stack->Stack[Stack->StackPointer - i].pointerVal);
        ParamList.push_back(Stack->Stack[Stack->StackPointer - i]);
    }

    // Of course, we need to know any necessary data about the class we're going to jump into. We take that here.
    Variable ClassInStack = Stack->Stack[Stack->StackPointer - Parameters];
    printf("\tClass to invoke is object #" PrtSizeT ".\r\n", ClassInStack.object.Heap);

    // With detail about the object we're going to invoke, we can resolve it to a Class instance.
    Variable* ObjectFromHeap = _ObjectHeap.GetObjectPtr(ClassInStack.object);
    printf("\tClass at 0x" PrtHex64 ".\r\n", ObjectFromHeap->pointerVal);
    auto* VirtualClass = (class Class*) ObjectFromHeap->pointerVal;

    // Here's where we search the class for the method we want to call (since it may want to be in an interface, etc.)
    // If this class does not exist, something has gone horribly wrong with the compiler.
    uint32_t MethodInClassIndex = VirtualClass->GetMethodFromDescriptor(MethodName.c_str(), MethodDesc.c_str(), ClassName.c_str(), VirtualClass);

    // Now, we know which class has the code, we know what method we want to call, we know the parameters of the call, and we know
    // what object of the class it was called on.
    // However, we first need to check something.
    // 0x100 means "native method", where the code to execute is NOT java bytecode, and cannot be simply jumped to.
    // It means that we need to search an externally loaded native library and execute real machine code to evaluate it.
    //
    // The native method will not return normally, since it is a void method.
    // Instead, it will throw a "NativeReturn" exception so that it can pass a JVM Object back to us if necessary.
    // Exceptions as control flow is typically awful, but it comes in really handy here.
    // It means that we are in full control of when and how native code executes, and what we do with the data it returns.
    //
    if(VirtualClass->Methods[MethodInClassIndex].Access & 0x100) {
        try {
            puts("Executing native method");
            NativeContext Context = { .InvocationMethod = Type, .ClassName = ClassName, .MethodName = MethodName, .MethodDescriptor = MethodDesc, .Parameters = ParamList, .ClassInstance = &ClassInStack.object};
            InvokeNative(Context);
        } catch (NativeReturn& e) {
            printf("Native: %s " PrtSizeT "\n", e.what(), e.Value.pointerVal);

            // We need to decrement the stack pointer since a method was called.
            Stack->StackPointer -= ParamList.size();

            // Don't set return value if the function returned void.
            if(MethodDesc.find(")V") == std::string::npos)
                Stack->Stack[Stack->StackPointer] = e.Value;

            // And now we can return to the caller method.
            return;
        }

        // The return; above means that, if we get here, the native method did not return a value.
        // By design of the PNI, this is an invalid state, and we must close the VM now before we execute any other bad native code.
        puts("Native method execution failed to return a value. This is invalid.");
        exit(5);
    }

    /*
     * With the native edge-case out of the way, this is where it gets juicy.
     * As a basic primer to the workings of Stack Frames (see the Purpuri documentation for more detail here)
     * They are the fundamental container of interpreter state.
     * Every method that is executed gets its own Frame.
     * It therefore has its own Stack, its own Stack Pointer, its own Program Counter, etc.
     * By this method, it is possible to jump between Stack Frames at will.
     *
     * It's very helpful for Java especially, since methods tend to call other methods and return.
     * Calling a method is equivalent to creating a new Stack Frame in the middle of executing another,
     * and returning is equivalent to destroying a Stack Frame and returning to a lower.
     *
     * Eventually, all Stack Frames will return to the Entry Point (unless there's infinite recursion, of course),
     * and all memory can be handled.
     *
     * Stack Frames are also relatively light, containing only pointers to other, already loaded, data.
     * The Stack and Code pointers stored are the fundamental reason that Purpuri was even possible for me to develop.
    */

    // We create a new Stack Frame here and prepare to fill it with data.
    // It's Stack[1] because we want it to be linked to (and accessible from) the lower frame.
    Stack[1] = StackFrame();
    Stack[1]._Method = &VirtualClass->Methods[MethodInClassIndex];

    printf("\tMethod has access 0x%x.\r\n", Stack[1]._Method->Access);
    // 0x8 means "static". Non-static methods implicitly have the object they were called on as the first parameter, so we can skip it if it's static.
    if(!(Stack[1]._Method->Access & 0x8))
        Parameters++;

    Stack[1]._Class = VirtualClass;

    // The dual usage of "Stack" here may be confusing.
    // The first Stack (the one we're indexing [1] into) represents the Call Stack; the hierarchy of calls made by a given program.
    // The second Stack (the one we're setting) represents the Value Stack; the things that are pushed and popped by the Java program to carry data around.
    // The logic for calculating the new position is basically "the lowest possible with the ability to contain the parameters of the method".
    Stack[1].Stack = clinit ?
            &StackFrame::ClassloadStack[Stack->Stack - StackFrame::ClassloadStack + Stack->StackPointer - Parameters] :
            &StackFrame::MemberStack[Stack->Stack - StackFrame::MemberStack + Stack->StackPointer - Parameters];

    size_t ParameterCount = 0;
    printf("\tSetting locals..\r\n");
    // If non-static, set "this"
    // This check is separate from the one immediately above, because we needed to calculate the Parameters additional length
    // to be able to set the Member Stack size, and we need to use the Member Stack Size to be able to set the instance of the Object.
    // It's a chicken-and-egg scenario. The simplest solution was two checks.
    if(!(Stack[1]._Method->Access & 0x8)) {
        printf("\tSetting \"this\" to " PrtSizeT ".\r\n", ClassInStack.object.Heap);
        Stack[1].Stack[ParameterCount] = ClassInStack;
        ParameterCount++;
    }

    // The rest of the parameters are on-stack, so just zip through them.
    for(size_t i = ParameterCount; i < Parameters; i++) {
        Stack[1].Stack[i] = Stack->Stack[Stack->StackPointer - (i - 1)];
        printf("\tSetting local " PrtSizeT " to " PrtSizeT ".\r\n", i, Stack[1].Stack[i].pointerVal);
    }

    printf("\tFunction's parameters start at " PrtSizeT ".\r\n", Parameters);
    // Don't want to accidentally immediately overwrite a parameter, so set the stack at the earliest empty value.
    Stack[1].StackPointer = Parameters;

    printf("Invoking method %s%s\n", MethodName.c_str(), MethodDesc.c_str());

    // Ignite is the main interpreter function above.
    // We pass Stack[1] so that it becomes the new "reference", and if another invoke is called, it will simply
    // add another StackFrame to the array and execute again.
    this->Ignite(&Stack[1]);

    // Because of the mechanisms of the standard return, the value is left on the new frame's stack.
    // However, it's trivial to retrieve, since we also have its stack pointer.
    Variable ReturnValue = Stack[1].Stack[Stack[1].StackPointer];

    // If the method was void, we need to adjust positively (this is about to be used in a -= so - is positive)
    // to ensure that we don't overwrite whatever was on the stack before this.
    if(MethodDesc.find(")V") != std::string::npos)
        Parameters--;

    // Now we can deal with removing the now unused parameters.
    Stack->StackPointer -= Parameters;
    printf("Shrinking the stack by " PrtSizeT " positions.\r\n", Parameters);

    // If the method was NOT void, then we now need to push the return value to the stack (since we didn't do that).
    int Offset = 0;
    if(MethodDesc.find(")V") == std::string::npos) {
        Stack->Stack[Stack->StackPointer] = ReturnValue;
        printf("Pushing function return value, " PrtSizeT "\r\n", ReturnValue.pointerVal);
        Offset--; // Make Offset -1, so that the line below works with return.
    }

    // As mentioned earlier, some edge cases occur on the interface between caller and callee.
    // By forcibly setting the value that used to be there into the same location, we can ensure that nothing changes or is corrupted.
    // Again, this problem is an indication of a bigger fault, but it's an enormous pain to debug, so I'll live with it and so should you.
    Stack->Stack[Stack->StackPointer + Offset] = UnderStack;
    printf("Restoring the value " PrtSizeT " under the function, just in case.\r\n", UnderStack.pointerVal);

}

/**
 * A simple wrapper to read the Descriptor of a method and extract the width of its parameters.
 *
 * J (long) and D (double) count for two, everything else counts for one.
 * Object references are notated by L followed by the class name followed by ;
 * These also count for one.
 *
 * @param Descriptor the descriptor "(Z)V" of the method to extract the parameters from
 * @return the amount of Variables it would take to store all the parameters.
 */
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
