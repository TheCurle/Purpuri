/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Class.hpp>
#include <string>

bool Class::ParseConstants(const char *&Code) {
    Constants = new ConstantPoolEntry* [ConstantCount + 1];

    puts("Reading constants..");

    if(Constants == NULL) return false;

    for(size_t i = 1; i < ConstantCount; i++) {
        Constants[i] = (ConstantPoolEntry*) Code;

        if(Constants[i] == NULL) continue;

        if(Constants[i]->Tag == TypeUtf8) {
            // Convert to std::string
            std::string NewString(Code + 3, ReadShortFromStream(Code + 1));
            // Save into the vector for easy searching.
            if(StringConstants.size() < i + 1)
                StringConstants.resize(i + 1); // When i = 1, this makes the list 1 long, and then tries to set StringConstants[1], which is invalid.

            StringConstants[i] = NewString;
        }
            
        int Size = GetConstantsCount(Code);
        Code += Size;

        // Long and Double types increase the constant offset by two
        if(Constants[i]->Tag == TypeLong || Constants[i]->Tag == TypeDouble) {
            Constants[i + 1] = NULL;
            i++;
        }
    }

    // NameAndDesc has to be parsed after Strings but before Methods, so it has its own little loop.
    for(int i = 1; i < ConstantCount; i++) {
        if(Constants[i] == NULL) continue;

        if(Constants[i]->Tag == TypeNamed) {
            char* Temp = (char*)Constants[i];
            uint16_t nameInd = ReadShortFromStream(Temp + 1);
            uint16_t descInd = ReadShortFromStream(Temp + 3);
            std::string Name = GetStringConstant(nameInd);
            std::string Desc = GetStringConstant(descInd);

            std::string NameAndDesc = Name;
            NameAndDesc.append(Desc);

            StringConstants[i] = NameAndDesc;
            printf("%d:\tType %s\n", i, NameAndDesc.c_str());
        }
    }

    for(int i = 1; i < ConstantCount; i++) {
        
        if(Constants[i] == NULL) continue;

        //printf("Constant %d has type %d\n", i, Constants[i]->Tag);
        char* Temp;
        switch(Constants[i]->Tag) {
            case TypeUtf8: 
                break;
            
            case TypeInteger: {
                Temp = (char*)Constants[i];
                uint32_t val = ReadIntFromStream(Temp + 1);
            
                printf("%d:\tValue %d\n", i, val);
                break;
            }

            case TypeLong: {
                Temp = (char*)Constants[i];
                size_t val = ReadLongFromStream(Temp + 1);
                printf("%d:\tValue %zd\n", i, val);
                break;
            }

            case TypeFloat: {
                Temp = (char*)Constants[i];
                Variable val = *(Variable*) (Temp + 1);
            
                printf("%d:\tValue %.6f\n", i, val.floatVal);
                break;
            }

            case TypeDouble: {
                Temp = (char*)Constants[i];
                Variable val = *(Variable*) (Temp + 1);
                printf("%d:\tValue %.6f\n", i, val.doubleVal);
                break;
            }

            case TypeClass: {
                Temp = (char*)Constants[i];
                uint16_t val = ReadShortFromStream(Temp + 1);
                std::string name = GetStringConstant(val);

                StringConstants[i] = name;

                printf("%d:\tName %s\n", i, name.c_str());
                break;
            }

            case TypeInterfaceMethod:
            case TypeMethod: {
                Temp = (char*)Constants[i];
                uint16_t classInd = ReadShortFromStream(Temp + 1);
                uint16_t nameAndDescInd = ReadShortFromStream(Temp + 3);

                std::string ClassName = GetStringConstant(classInd);
                std::string NameAndDesc = GetStringConstant(nameAndDescInd);
                printf("Retrieved name %s from index %d\n", NameAndDesc.c_str(), nameAndDescInd);

                StringConstants[i] = NameAndDesc;

                printf("%d:\tMethod %s belongs to class %s\n", i, NameAndDesc.c_str(), ClassName.c_str());
                break;
            }

            
            case TypeField: {
                Temp = (char*)Constants[i];
                uint16_t classInd = ReadShortFromStream(Temp + 1);
                uint16_t nameAndDescInd = ReadShortFromStream(Temp + 3);

                std::string ClassName = GetStringConstant(classInd);
                std::string FieldDesc = GetStringConstant(nameAndDescInd);

                StringConstants[i] = FieldDesc;

                printf("%d:\tField %s belongs to class %s\n", i, FieldDesc.c_str(), ClassName.c_str());
                break;
            }

            case TypeString: {
                Temp = (char*)Constants[i];
                uint16_t utf8Ind = ReadShortFromStream(Temp + 1);

                std::string String = GetStringConstant(utf8Ind);

                StringConstants[i] = String;

                printf("%d:\tInterred String %s (%d).\n", i, String.c_str(), utf8Ind);
                break;
            }

            default:
                if(Constants[i]->Tag != TypeUtf8 && Constants[i]->Tag != TypeNamed) // These are handled above.
                    printf("%d:\tValue unknown. Type %d\n", i, Constants[i]->Tag);
        }
    }
    
    return true;
}

std::string Class::GetStringConstant(uint32_t index) {
    if(index < 1 || index >= ConstantCount)
        return ClassHeap::UnknownClass;
    
    return StringConstants.at(index);
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

bool Engine::MethodClassMatches(uint16_t MethodInd, Class* pClass, const char* TestName) {
    char* Data = (char*) pClass->Constants[MethodInd];
    uint16_t classInd = ReadShortFromStream(Data + 1);

    std::string ClassName = pClass->GetStringConstant(classInd);

    return ClassName.compare(TestName) == COMPARE_MATCH;
}