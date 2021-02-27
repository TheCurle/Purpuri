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

        // Long and Double types increase the constant offset by two
        if(Constants[i]->Tag == TypeLong || Constants[i]->Tag == TypeDouble) {
            Constants[i + 1] = NULL;
            i++;
        }
    }

    for(int i = 1; i < ConstantCount; i++) {
        
        if(Constants == NULL) return false;
        if(Constants[i] == NULL) continue;

        printf("Constant %d has type %d\n", i, Constants[i]->Tag);
        char* Temp;
        switch(Constants[i]->Tag) {
            case TypeUtf8: 
                GetStringConstant(i, Temp);
                //printf("\tValue %s\n", Temp);
                break;
            
            
            case TypeInteger: {
                Temp = (char*)Constants[i];
                uint32_t val = ReadIntFromStream(&Temp[1]);
            
                //printf("\tValue %d\n", val);
                break;
            }

            case TypeLong: {
                Temp = (char*)Constants[i];
                size_t val = ReadLongFromStream(&Temp[1]);
                //printf("\tValue %zd\n", val);
                break;
            }

            case TypeFloat: {
                Temp = (char*)Constants[i];
                uint32_t val = ReadIntFromStream(&Temp[1]);
            
                //printf("\tValue %.6f\n", *reinterpret_cast<float*>(&val));
                break;
            }

            case TypeDouble: {
                Temp = (char*)Constants[i];
                size_t val = ReadLongFromStream(&Temp[1]);
                //printf("\tValue %.6f\n", *reinterpret_cast<double*>(&val));
                break;
            }

            case TypeClass: {
                Temp = (char*)Constants[i];
                uint16_t val = ReadShortFromStream(&Temp[1]);
                GetStringConstant(val, Temp);
                printf("\tName %s\n", Temp);
                break;
            }

            case TypeInterfaceMethod:
            case TypeMethod: {
                Temp = (char*)Constants[i];
                uint16_t classInd = ReadShortFromStream(&Temp[1]);
                uint16_t nameAndDescInd = ReadShortFromStream(&Temp[3]);
                Temp = (char*)Constants[classInd];
                uint16_t val = ReadShortFromStream(&Temp[1]);
                char* ClassName;
                GetStringConstant(val, ClassName);

                Temp = (char*)Constants[nameAndDescInd];
                uint16_t nameInd = ReadShortFromStream(&Temp[1]);
                uint16_t descInd = ReadShortFromStream(&Temp[3]);
                char* MethodName, *MethodDesc;
                GetStringConstant(nameInd, MethodName);
                GetStringConstant(descInd, MethodDesc);
                printf("\tMethod %s%s belongs to class %s\n", MethodName, MethodDesc, ClassName);
                break;
            }

            case TypeNamed: {
                Temp = (char*)Constants[i];
                uint16_t nameInd = ReadShortFromStream(&Temp[1]);
                uint16_t descInd = ReadShortFromStream(&Temp[3]);
                char* MethodName, *MethodDesc;
                GetStringConstant(nameInd, MethodName);
                GetStringConstant(descInd, MethodDesc);
                printf("\tType %s%s\n", MethodName, MethodDesc);
                break;
            }

            case TypeString: 
            case TypeField:
                printf("\tValue unknown. Potential forward reference\n");
                break;
            default:
                printf("\tValue unknown. Unrecognized type.\n");
        }
    }
    
    return true;
}

bool Class::GetStringConstant(uint32_t index, char *&String) {
    if(index < 1 || index >= ConstantCount)
        return false;

    if(Constants[index]->Tag != TypeUtf8)
        return false;
    
    char* Entry = (char*)Constants[index];

    uint16_t Length = ReadShortFromStream(&Entry[1]);

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
        case TypeUtf8: return 3 + ReadShortFromStream(Code + 1);
        case TypeInteger: return 5;
        case TypeFloat: return 5;
        case TypeLong: return 9;
        case TypeDouble: return 9;
        case TypeClass: return 3;
        case TypeString: return 3;
        case TypeField: return 5;
        case TypeMethod: return 5;
        case TypeInterfaceMethod: return 5;
        case TypeNamed: return 5;
        default: break;
    }

    return 0;
}