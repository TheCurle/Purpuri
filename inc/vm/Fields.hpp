/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <stdint.h>
#include "Common.hpp"

struct FieldData {
    uint16_t Access;
    uint16_t Name;
    uint16_t Descriptor;
    uint16_t AttributeCount;
    struct AttributeData* Attributes;
};