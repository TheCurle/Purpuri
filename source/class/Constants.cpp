/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../../headers/Class.hpp"
#include <string>

bool Class::ParseConstants(const char *&Code) {
    Constants = new ConstantPoolEntry* [ConstantCount - 1];

    puts("Reading constants..");

    if(Constants == NULL) return false;

    for(int i = 1; i < ConstantCount; i++) {
        Constants[i] = (ConstantPoolEntry*) Code;

        int Size = GetConstantsCount(Code);
        Code += Size;

        printf("Constant %d has type %d\n", i, (char)Constants[i]->Tag);

        // Long and Double types increase the constant offset by two
        if(Constants[i]->Tag == 5 || Constants[i]->Tag == 6) {
            Constants[i + 1] = NULL;
            i++;
        }
    }
    
    return true;
}

bool Class::GetStringConstant(uint32_t index, char *&String) {
    if(index < 1 || index >= ConstantCount)
        return false;

    // 1 = string
    if(Constants[index]->Tag != 1)
        return false;
    
    char* Entry = (char*)Constants[index];

    uint16_t Length = ((&Entry[1])[0] & 0xFF) << 8 | (&Entry[1])[1] & 0xFF;

    char* Buffer = new char[Length + 1];
    Buffer[Length] = 0;

    memcpy(Buffer, &(Entry[3]), Length);

    //printf("Retrieving constant %s from ID %d\n", Buffer, index);
    
    String = Buffer;
    return true;
}

uint32_t Class::GetConstantsCount(const char* Code) {
    ConstantPoolEntry* Pool = (ConstantPoolEntry*) Code;

    switch(Pool->Tag) {
        case 1: return 3 + ((Code + 1)[0] << 8 | (Code + 1)[1]);
        case 3: return 5;
        case 4: return 5;
        case 5: return 9;
        case 6: return 9;
        case 7: return 3;
        case 8: return 3;
        case 9: return 5;
        case 10: return 5;
        case 11: return 5;
        case 12: return 5;
        default: break;
    }

    return 0;
}