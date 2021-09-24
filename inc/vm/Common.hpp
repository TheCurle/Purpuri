/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/
#pragma once
#include <stdint.h>
#include <cstddef>
#include <list>
#include <string>

class Class;
class ClassHeap;
class StackFrame;
class ObjectHeap;

#define ReadLongFromStream(Stream) \
        (size_t) ( (((size_t) ReadIntFromStream(Stream) << 32) & 0xFFFFFFFF00000000) | ((size_t) ReadIntFromStream(Stream + 4) & 0x00000000FFFFFFFF))

#define ReadIntFromStream(Stream) \
        (uint32_t)( ((uint32_t)((Stream)[0]) <<24 & 0xFF000000) | ((uint32_t)((Stream)[1]) << 16 & 0x00FF0000) \
        | ((uint32_t)((Stream)[2]) << 8 & 0x0000FF00) | ((uint32_t)((Stream)[3]) & 0x000000FF) )

#define ReadShortFromStream(Stream) \
        (uint16_t) ( ((uint16_t)((Stream)[0]) << 8 & 0xFF00) | (((uint16_t)(Stream)[1]) & 0x00FF) )

#define SHUTUPUNUSED(X) ((void)X)

#define COMPARE_MATCH 0

void PrintList(std::list<std::string> &list);

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
    float floatVal;
    double doubleVal;
    size_t pointerVal;
    Object object;
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

    fconst_0,           // 11
    fconst_1,           // 12
    fconst_2,           // 13

    dconst_0,           // 14
    dconst_1,           // 15

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

    dload_0,            // 38
    dload_1,            // 39
    dload_2,            // 40
    dload_3,            // 41

    aload_0,            // 42
    aload_1,            // 43
    aload_2,            // 44
    aload_3,            // 45

    iaload,             // 46

    aaload = 50,        // 50
    istore = 54,        // 54
    lstore = 55,        // 55
    fstore = 56,        // 56
    dstore = 57,        // 57
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

    dstore_0,           // 71
    dstore_1,           // 72
    dstore_2,           // 73
    dstore_3,           // 74

    astore_0,           // 75
    astore_1,           // 76
    astore_2,           // 77
    astore_3,           // 78

    iastore,            // 79
    lastore,            // 80
    fastore,            // 81
    dastore,            // 82
    aastore,            // 83
    bastore,            // 84
    castore,            // 85
    sastore,            // 86

    bcdup = 89,         // 89
    dup_x1,             // 90
    dup_x2,             // 91

    iadd = 96,          // 96
    ladd,               // 97
    _fadd,               // 98
    dadd,               // 99

    isub,               // 100
    lsub,               // 101
    _fsub,              // 102
    dsub,               // 103
    imul,               // 104
    lmul,               // 105
    _fmul,              // 106
    dmul,               // 107

    _idiv,              // 108
    _ldiv,              // 109
    _fdiv,              // 110

    ddiv,               // 111
    irem,               // 112
    lrem,               // 113
    frem,               // 114
    _drem,              // 115

    iinc = 132,         // 132

    i2d = 135,          // 135
    l2i,                // 136
    l2f,                // 137
    l2d,                // 138

    f2i,                // 139
    f2l,                // 140
    f2d,                // 141
    
    d2i,                // 142
    d2l,                // 143
    d2f,                // 144

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
    invokeinterface,    // 185
    invokedynamic,      // 186

    _new,               // 187

    newarray,           // 188
    anewarray,          // 189

    arraylength,        // 190

    athrow,             // 191
    checkcast,          // 192
    instanceof,         // 193
    monitorenter,       // 194
    monitorexit,        // 195
};

enum {
    NONE,
    TypeUtf8,
    NONE_,
    TypeInteger,
    TypeFloat,
    TypeLong,
    TypeDouble,
    TypeClass,
    TypeString,
    TypeField,
    TypeMethod,
    TypeInterfaceMethod,
    TypeNamed
};