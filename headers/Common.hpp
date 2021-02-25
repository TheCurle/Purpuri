/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/
#pragma once
#include <stdint.h>


class Class;
class ClassHeap;

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
 