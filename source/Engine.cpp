/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Stack.hpp"
#include <math.h>
#include <stdio.h>
#include <cstring>

Variable* StackFrame::MemberStack;
StackFrame* StackFrame::FrameBase;

Engine::Engine() {
    _ClassHeap = NULL;
    _ObjectHeap = NULL;
}

Engine::~Engine() {}

uint32_t Engine::Ignite(StackFrame* Stack) {
    StackFrame* CurrentFrame = &Stack[0];

    printf("Execution frame %zd, stack begins %zd\n", CurrentFrame - StackFrame::FrameBase, CurrentFrame->Stack - StackFrame::MemberStack);

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
    char* Name;
    Class->GetStringConstant(CurrentFrame->_Method->Name, Name);

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

            case _new:
                New(CurrentFrame);
                CurrentFrame->ProgramCounter += 3;
                printf("Initialized new object\n");
                break;

            case bcdup:
                CurrentFrame->Stack[CurrentFrame->StackPointer + 1] =
                    CurrentFrame->Stack[CurrentFrame->StackPointer];
                CurrentFrame->StackPointer++;
                CurrentFrame->ProgramCounter++;
                printf("Duplicated the last item on the stack\n");
                break;

            case invokespecial:
                InvokeSpecial(CurrentFrame);
                CurrentFrame->ProgramCounter += 3;
                break;

            case invokeinterface:
                InvokeInterface(CurrentFrame);
                CurrentFrame->ProgramCounter += 5;
                break;

            case iconst_0:
            case iconst_5:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].intVal = (uint8_t) Code[CurrentFrame->ProgramCounter] - iconst_0;
                CurrentFrame->ProgramCounter++;
                printf("Pushed int constant %d to the stack\n", Code[CurrentFrame->ProgramCounter - 1] - iconst_0);
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

            case fload_1:
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
                printf("Multiplied the last two values on the stack (result %d)\n", CurrentFrame->Stack[CurrentFrame->StackPointer].intVal);
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

            case dstore_1:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - dstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled double %.6f out of the stack into 1\n", CurrentFrame->Stack[CurrentFrame->StackPointer + 1].doubleVal);
                break;
            
            case fstore_1:
            case fstore_3:
                CurrentFrame->Stack[(uint8_t)Code[CurrentFrame->ProgramCounter] - fstore_0] = CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled float %f out of the stack into local %d\n", CurrentFrame->Stack[CurrentFrame->StackPointer - 1].floatVal, Code[CurrentFrame->ProgramCounter - 1] - fstore_0);
                break;
            

            case dload_1:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] =
                    CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - dload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled %.6f out of local 1 into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].doubleVal);
                break;
            
            case aload_0:
            case aload_1:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer] = CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - aload_0];
                CurrentFrame->ProgramCounter++;
                printf("Pulled object %d out of local %d into the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].object.Type, Code[CurrentFrame->ProgramCounter - 1] - aload_0);
                break;

            case astore_0:
            case astore_1:
                CurrentFrame->Stack[(uint8_t) Code[CurrentFrame->ProgramCounter] - astore_0] = 
                    CurrentFrame->Stack[CurrentFrame->StackPointer--];
                CurrentFrame->ProgramCounter++;
                printf("Pulled the last object on the stack into local %d\n", Code[CurrentFrame->ProgramCounter - 1] - astore_0);
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

            case bipush:
                CurrentFrame->StackPointer++;
                CurrentFrame->Stack[CurrentFrame->StackPointer].charVal = (uint8_t) Code[CurrentFrame->ProgramCounter + 1];
                CurrentFrame->ProgramCounter += 2;
                printf("Pushed char %d to the stack\n", CurrentFrame->Stack[CurrentFrame->StackPointer].charVal);
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
    Object obj;

    switch(Code[0]) {
        case TypeFloat:
        case TypeInteger:
            temp.intVal = ReadIntFromStream(&Code[1]);
            break;
        
        case TypeString:
            shortTemp = ReadShortFromStream(&Code[1]);
            Class->GetStringConstant(shortTemp, StrTmp);
            Str = &StrTmp;
            obj = _ObjectHeap->CreateString(Str, _ClassHeap);
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

    if(!Stack->_Class->CreateObject(Index, this->_ObjectHeap, Stack->Stack[Stack->StackPointer].object))
        return -1;
    return 0;
}

void Engine::InvokeSpecial(StackFrame *Stack) {
    InvokeVirtual(Stack, invokespecial);
}

void Engine::InvokeInterface(StackFrame *Stack) {
    size_t ProgramCounter = Stack[0].ProgramCounter;
    uint8_t* Code = (uint8_t*) Stack[0]._Method->Code->Code;
    uint16_t MethodInd = ReadShortFromStream(&Code[ProgramCounter + 1]);

    printf("Calculating invocation for interface function.\n");
    // Read method data
    uint8_t* ClassConstants = (uint8_t*) Stack[0]._Class->Constants[MethodInd];
    uint16_t ClassNameLocationInd = ReadShortFromStream(&ClassConstants[1]);
    uint16_t ClassNameTypeInd = ReadShortFromStream(&ClassConstants[3]);
    // Read Class data
    ClassConstants = (uint8_t*) Stack[0]._Class->Constants[ClassNameLocationInd];
    uint16_t ClassNameInd = ReadShortFromStream(&ClassConstants[1]);
    char* ClassName;
    Stack[0]._Class->GetStringConstant(ClassNameInd, ClassName);

    printf("\tInvocation is calling into class %s\n", ClassName);

    Class* InvocationClass = _ClassHeap->GetClass(ClassName);
    ClassConstants = (uint8_t*) &InvocationClass->Constants[ClassNameTypeInd];

    uint16_t MethodNameInd = ReadShortFromStream(&ClassConstants[1]);
    uint16_t MethodDescriptorInd = ReadShortFromStream(&ClassConstants[3]);
    char* MethodName, *MethodDescriptor;
    InvocationClass->GetStringConstant(MethodNameInd, MethodName);
    InvocationClass->GetStringConstant(MethodDescriptorInd, MethodDescriptor);

    printf("\tInvocation resolves to method %s%s\n", MethodName, MethodDescriptor);
    for(;;) {}

}

void Engine::InvokeVirtual(StackFrame *Stack, uint16_t Type) {
    uint16_t MethodIndex = ReadShortFromStream(&Stack[0]._Method->Code->Code[Stack[0].ProgramCounter + 1]);

    printf("Calculating invocation for method function.\n");
    printf("Trying to invoke %x.\r\n", Stack[0]._Class->This);
    //Variable ObjectRef = Stack[0].Stack[Stack[0].StackPointer];

    uint8_t* Constants = (uint8_t*) Stack[0]._Class->Constants[MethodIndex];

    uint16_t ClassIndex = ReadShortFromStream(&Constants[1]);
    uint16_t ClassNTIndex = ReadShortFromStream(&Constants[3]);

    Constants = (uint8_t*) Stack[0]._Class->Constants[ClassIndex];
    uint16_t ClassName = ReadShortFromStream(&Constants[1]);
    char* ClassStr;
    Stack[0]._Class->GetStringConstant(ClassName, ClassStr);

    
    printf("\tInvocation is calling into class %s\n", ClassStr);

    Class* Class = _ClassHeap->GetClass(ClassStr);

    Constants = (uint8_t*) Stack[0]._Class->Constants[ClassNTIndex];

    Method MethodToInvoke;
    MethodToInvoke.Name = ReadShortFromStream(&Constants[1]);
    MethodToInvoke.Descriptor = ReadShortFromStream(&Constants[3]);
    MethodToInvoke.Access = 0;

    char* MethodName, *MethodDescriptor;

    Stack[0]._Class->GetStringConstant(MethodToInvoke.Name, MethodName);
    Stack[0]._Class->GetStringConstant(MethodToInvoke.Descriptor, MethodDescriptor);
    
    printf("\tInvocation resolves to method %s%s\n", MethodName, MethodDescriptor);
    class Class* VirtualClass = Class;

    int MethodInClassIndex = Class->GetMethodFromDescriptor(MethodName, MethodDescriptor, VirtualClass);

    Stack[1] = (StackFrame) { 0 };

    Stack[1]._Method = &Class->Methods[MethodInClassIndex];
    MethodToInvoke.Access = ReadShortFromStream((uint8_t*) Stack[1]._Method);

    if(MethodToInvoke.Access & 0x20)
        Stack[1]._Class = VirtualClass->GetSuper();
    else if(MethodToInvoke.Access & 0x200)
        Stack[1]._Class = Stack[0]._Class;
    else
        Stack[1]._Class = VirtualClass;

    int Parameters = GetParameters(MethodDescriptor) + 1;

    if(Type == invokestatic)
        Parameters--;
    
    int DiscardStack = Parameters;
    
    if(!(Stack[1]._Method->Access & 0x100))
        DiscardStack += Stack[1]._Method->Code->LocalsSize;
    
    Stack[1].Stack = &StackFrame::MemberStack[Stack->Stack - StackFrame::MemberStack + Stack[0].StackPointer - Parameters + 1];
    Stack[1].StackPointer = DiscardStack - 1;

    printf("Invoking method %s%s - Last frame at %zd, new frame begins %zd\n", MethodName, MethodDescriptor,
        Stack[0].Stack - StackFrame::MemberStack + Stack[0].StackPointer, 
        Stack[1].Stack - StackFrame::MemberStack);

    this->Ignite(&Stack[1]);
    std::string DescStr(MethodDescriptor);
    if(DescStr.find(")V") != std::string::npos)
        DiscardStack--;
    
    Stack[0].StackPointer -= DiscardStack;
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