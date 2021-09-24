/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#pragma once
#include <stdint.h>
#include "Common.hpp"

/**
 * When an exception is thrown
 *  between PCStart and PCEnd,
 *  with the type CatchType,
 *  jump to PCHandler.
 * 
 * PC = Program Counter
 */
struct Exception {
    uint16_t PCStart;
    uint16_t PCEnd;
    uint16_t PCHandler;
    uint16_t CatchType;
};

struct CodePoint {
    uint16_t Name;
    uint32_t Length;
    uint16_t StackSize;
    uint16_t LocalsSize;
    uint32_t CodeLength;
    uint8_t* Code;
    uint16_t ExceptionCount;
    struct Exception* Exceptions;
    uint16_t AttributeCount;
    struct AttributeData* Attributes;
};

struct MethodData {
    uint16_t Access;
    uint16_t Name;
    uint16_t Descriptor;
    uint16_t AttributeCount;
    struct AttributeData* Attributes;
};

struct Method : MethodData {
    struct MethodData* Data;
    struct CodePoint* Code;
};
