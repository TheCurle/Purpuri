/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/
#pragma once
#include <stdint.h>

class Class;
class ClassHeap;
class StackFrame;
class ObjectHeap;

struct AttributeData {
    uint16_t AttributeName;
    uint32_t AttributeLength;
    uint8_t* AttributeInfo;
};

class Object {
    public:
        size_t Heap;
        uint8_t Type;
};

union Variable {
    uint8_t charVal;
    uint16_t shortVal;
    uint32_t intVal;
    uint32_t floatVal;
    size_t pointerVal;
    Object object;
};
 

class Engine {
    public:
        ClassHeap* ClassHeap;
        ObjectHeap* ObjectHeap;

        Engine();
        virtual ~Engine();
        virtual uint32_t Ignite(StackFrame* Stack);

        void InvokeSpecial(StackFrame* Stack);
        void InvokeVirtual(StackFrame* Stack, uint16_t Type);
        void InvokeNative(StackFrame* Stack);

        void PutField(StackFrame* Stack);
        void GetField(StackFrame* Stack);

        void GetConstant(Class* Class, uint8_t Index);

        uint16_t GetParameters(const char* Descriptor);
        uint16_t GetParametersStack(const char* Descriptor);

        void New(StackFrame* Stack);
        void NewArray(StackFrame* Stack);
        void ANewArray(StackFrame* Stack);

        Variable CreateObject(Class* Class);
        Variable* CreateArray(uint8_t Type, int32_t Count);
        
        void DumpObject(Object Object);
};

enum {
    noop,               // 0
    aconst_null,        // 1
    iconst_m1,          // 2
    iconst_0,           // 3
    iconst_1,           // 4
    iconst_2,           // 5
    iconst_3,           // 6
    iconst_4,           // 7
    iconst_5,           // 8
    
    lconst_0,           // 9
    lconst_1,           // 10

    bipush = 16,        // 16
    sipush,             // 17

    ldc,                // 18

    ldc2_w = 20,        // 20
    
    iload,              // 21
    lload,              // 22

    aload = 25,         // 25

    iload_0,            // 26
    iload_1,            // 27
    iload_2,            // 28
    iload_3,            // 29

    lload_0,            // 30
    lload_1,            // 31
    lload_2,            // 32
    lload_3,            // 33

    fload_0,            // 34
    fload_1,            // 35
    fload_2,            // 36
    fload_3,            // 37

    aload_0 = 42,       // 42
    aload_1,            // 43
    aload_2,            // 44
    aload_3,            // 45

    iaload,             // 46

    aaload = 50,        // 50
    istore = 54,        // 54
    astore = 58,        // 58

    istore_0,           // 59
    istore_1,           // 60
    istore_2,           // 61
    istore_3,           // 62

    lstore_0,           // 63
    lstore_1,           // 64
    lstore_2,           // 65
    lstore_3,           // 66

    fstore_0,           // 67
    fstore_1,           // 68
    fstore_2,           // 69
    fstore_3,           // 70

    astore_0 = 75,      // 75
    astore_1,           // 76
    astore_2,           // 77
    astore_3,           // 78

    iastore,            // 79

    aastore = 83,       // 83

    bcdup = 89,           // 89
    dup_x1,             // 90
    dup_x2,             // 91

    iadd = 96,          // 96
    ladd,               // 97

    isub = 100,         // 100
    imul = 104,         // 104

    iinc = 132,         // 132

    ifeq = 153,         // 153
    ifne,               // 154
    iflt,               // 155
    ifge,               // 156
    ifgt,               // 157
    ifle,               // 158

    if_icmpeq,          // 159
    if_icmpne,          // 160
    if_icmplt,          // 161
    if_icmpge,          // 162
    if_icmpgt,          // 163
    if_icmple,          // 164

    _goto = 167,        // 167

    ireturn = 172,      // 172

    _return = 177,      // 177

    getfield = 180,     // 180
    putfield,           // 181

    invokevirtual,      // 182
    invokespecial,      // 183
    invokestatic,       // 184

    _new = 187,         // 187

    newarray,           // 188
    anewarray,          // 189

    athrow = 191,       // 191
    checkcast,          // 192
    instanceof,         // 193
    monitorenter,       // 194
    monitorexit,        // 195
};