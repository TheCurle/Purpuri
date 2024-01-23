/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Class.hpp>
#include <string>

/**
 * This file implements the Constant Pool related functions in the Class class.
 *
 * This includes parsing the constants pool, and retrieving common constants such as strings.
 *
 * The parsing of the constants pool requires three passes, so it is rather large. It is therefore contained here.
 *
 * @author Curle
 */

/**
 * Parse the Constants Pool.
 *
 * This works on a three step process.
 *  1. Read all UTF-8 string constants.
 *  2. Read all Named constants.
 *  3. Read everything else.
 *
 * It is necessary to do it this way because of the hierarchy of Constants.
 * Take, for example, a Method.
 * A Method Constant holds within it a Named Constant, which itself holds a UTF-8 Constant.
 *
 * There are no guarantees that Strings will come first, followed by Named, followed by Methods, we have to process
 *  them separately.
 * This leads to having three loops in the parsing process.
 *
 * There are also some other tricks going on here; such as the StringConstants vector.
 * It is synchronized with the length of all Constants, so that we can save whatever else we need to into it.
 * Thus, if we want to access other Constants (such as Named, or Classes, or Methods) as if they were a String,
 *  we can do so with minimal effort.
 *
 * @param pCode the code of the class currently being loaded.
 * @return whether parsing finished properly.
 */
bool Class::ParseConstants(const char *&pCode) {
    // ConstantCount is immediately before the pool, so we can rely on knowing how big it is.
    // We add 1 as insurance for later.
    Constants = new ConstantPoolEntry* [ConstantCount + 1];
    StringConstants.resize(ConstantCount + 1);

    puts("Reading constants..");

    // If allocation of the array failed, we need to cancel now, otherwise we'll segfault.
    if(Constants == nullptr) return false;

    // Pass 1: Read all String constants.
    for(size_t i = 1; i < ConstantCount; i++) {
        // We use this pass to set all entries in the constants field.
        // If there's nothing here, skip to the next entry.
        Constants[i] = (ConstantPoolEntry*) pCode;
        if(Constants[i] == nullptr) continue;

        // If we're looking at a Utf8 string, we need to save it for the next pass.
        if(Constants[i]->Tag == TypeUtf8) {
            // Convert to std::string
            std::string NewString(pCode + 3, ReadShortFromStream(pCode + 1));

            // Save into the vector for easy searching.
            StringConstants[i] = NewString;
        }

        // We don't actually want to parse any more data here, we'll leave that for the later passes.
        // Therefore, we read how much more data is going to come, and skip past it.
        uint32_t Size = GetConstantsCount(pCode);
        pCode += Size;

        // Long and Double types increase the constant offset by two, so we need to make sure we don't check the "empty" space on the next pass.
        if(Constants[i]->Tag == TypeLong || Constants[i]->Tag == TypeDouble) {
            Constants[i + 1] = nullptr;
            i++;
        }
    }

    // Pass 2: Read Typed constants.
    for(int i = 1; i < ConstantCount; i++) {
        // Skip if we're in the "empty" space of a double-wide constant, like above.
        if(Constants[i] == nullptr) continue;

        // "Named" here means it has a name and a descriptor; such as methods and fields.
        // They're composed of two string constants, and may reference future strings that haven't been loaded yet.
        // That's why we do a separate pass for Strings, and for Named (since this will happen again with classes).
        if(Constants[i]->Tag == TypeNamed) {
            char* Temp = (char*)Constants[i];
            // Two Stream reads to fetch Constant IDs, and fetching from the String Constants vector to compose the Named string.
            auto nameInd = ReadShortFromStream(Temp + 1);
            auto descInd = ReadShortFromStream(Temp + 3);
            std::string Name = GetStringConstant(nameInd);
            std::string Desc = GetStringConstant(descInd);

            std::string NameAndDesc = Name;
            NameAndDesc.append(Desc);

            // It would be handy if we could access these Named types as if they were Strings, and since constants never overlap, we can use
            // the empty spaces in the StringConstants array (as described above) to make this happen.
            StringConstants[i] = NameAndDesc;
            printf("%d:\tType %s\n", i, NameAndDesc.c_str());
        }
    }

    // Pass 3: All Other Data.
    for(int i = 1; i < ConstantCount; i++) {
        // Yet again, we need to skip if we're in an empty space from a double-wide constant.
        if(Constants[i] == nullptr) continue;

        // We need to use a pointer a bunch of times, and since we can't use the same name in different switch cases, it's here.
        char* Temp;
        switch(Constants[i]->Tag) {
            // Strings have already been taken care of in Pass 1.
            case TypeUtf8: 
                break;

            // Integers are a simple matter of reading from the stream.
            case TypeInteger: {
                Temp = (char*)Constants[i];
                auto val = ReadIntFromStream(Temp + 1);
            
                printf("%d:\tValue %d\n", i, val);
                break;
            }

            // Longs are a simple matter of reading from the stream.
            case TypeLong: {
                Temp = (char*)Constants[i];
                auto val = ReadLongFromStream(Temp + 1);
                printf("%d:\tValue " PrtInt64 "\n", i, val);
                break;
            }

            // Floats have a weird construction bitwise, so we type pun to the union to read it later.
            case TypeFloat: {
                Temp = (char*)Constants[i];
                Variable val = *(Variable*) (Temp + 1);
            
                printf("%d:\tValue %.6f\n", i, val.floatVal);
                break;
            }

            // Doubles have a weird construction bitwise, so we type pun to the union to read it later.
            case TypeDouble: {
                Temp = (char*)Constants[i];
                Variable val = *(Variable*) (Temp + 1);
                printf("%d:\tValue %.6f\n", i, val.doubleVal);
                break;
            }

            // Classes use String constants and may appear before their String, so it goes in pass 3.
            case TypeClass: {
                Temp = (char*)Constants[i];
                auto val = ReadShortFromStream(Temp + 1);
                std::string name = GetStringConstant(val);

                // Here we can use the same StringConstants gap trick - Classes are referenced by Constant ID in a bunch of places.
                StringConstants[i] = name;

                printf("%d:\tName %s\n", i, name.c_str());
                break;
            }

            // Methods use Named constants and may appear before their Named, so it goes in pass 3.
            case TypeInterfaceMethod:
            case TypeMethod: {
                Temp = (char*)Constants[i];
                // Method types contain the Class and Named indexes.
                auto classInd = ReadShortFromStream(Temp + 1);
                auto nameAndDescInd = ReadShortFromStream(Temp + 3);

                // Once we have the class and Named that the method is in, we can switch the pointer to the class to read its' name.
                Temp = (char*)Constants[classInd];
                auto val = ReadShortFromStream(Temp + 1);

                // With this information we can do some extra juicy logging.
                std::string ClassName = GetStringConstant(val);
                std::string NameAndDesc = GetStringConstant(nameAndDescInd);

                printf("Retrieved name %s from index %d\n", NameAndDesc.c_str(), nameAndDescInd);

                // Here we can use the same StringConstants gap trick - Methods are referenced by Constant ID in a bunch of places.
                StringConstants[i] = NameAndDesc;

                printf("%d:\tMethod %s belongs to class %s\n", i, NameAndDesc.c_str(), ClassName.c_str());
                break;
            }

            
            case TypeField: {
                Temp = (char*)Constants[i];
                auto classInd = ReadShortFromStream(Temp + 1);
                auto nameAndDescInd = ReadShortFromStream(Temp + 3);

                Temp = (char*)Constants[classInd];
                auto val = ReadShortFromStream(Temp + 1);
                std::string ClassName = GetStringConstant(val);

                std::string FieldDesc = GetStringConstant(nameAndDescInd);

                // Here we can use the same StringConstants gap trick - Fields are referenced by Constant ID in a bunch of places.
                StringConstants[i] = FieldDesc;

                printf("%d:\tField %s belongs to class %s (%d)\n", i, FieldDesc.c_str(), ClassName.c_str(), classInd);
                break;
            }

            // This type refers to Strings that have been interred by the compiler - they represent instances of the String object rather than utf8 character arrays.
            case TypeString: {
                Temp = (char*)Constants[i];
                auto utf8Ind = ReadShortFromStream(Temp + 1);

                std::string String = GetStringConstant(utf8Ind);

                // Here we can use the same StringConstants gap trick - Interred Strings are referenced by Constant ID in a bunch of places.
                StringConstants[i] = String;

                printf("%d:\tInterred String %s (%d).\n", i, String.c_str(), utf8Ind);
                break;
            }

            // Fall back to a debug print, we don't want to stop execution if we don't absolutely have to.
            default:
                if(Constants[i]->Tag != TypeUtf8 && Constants[i]->Tag != TypeNamed) // These are handled by passes 1 and 2.
                    printf("%d:\tValue unknown. Type %d\n", i, Constants[i]->Tag);
        }
    }
    
    return true;
}

/**
 * A simple wrapper to fetch the std::string representation of a Constant by ID.
 * @param index an index to the Constants pool of the current Class.
 * @return the std::string representation of the constant, or "Unknown Value" if out of range or not String representable.
 */
std::string Class::GetStringConstant(uint32_t index) {
    if(index < 1 || index >= ConstantCount)
        return ClassHeap::UnknownClass;
    
    return StringConstants.at(index);
}

/**
 * A simple wrapper to get the length in bytes that should be skipped to reach the next Constant entry.
 * Note that we want to pre-initialize the next byte, so this is usually one higher than the size of the constant.
 *
 * Strings are variable length, so we take note of that here.
 * @param pCode The code reference of the current constant being parsed, in case we need more information.
 * @return The number of bytes to pass.
 */
uint32_t Class::GetConstantsCount(const char* pCode) {
    auto* Pool = (ConstantPoolEntry*) pCode;

    switch(Pool->Tag) {
        case TypeUtf8: return 3 + ReadShortFromStream(pCode + 1);

        case TypeInteger:
        case TypeFloat: return 5;

        case TypeLong:
        case TypeDouble: return 9;

        case TypeClass:
        case TypeString: return 3;

        case TypeField:
        case TypeMethod:
        case TypeInterfaceMethod:
        case TypeNamed: return 5;

        default: break;
    }

    return 0;
}