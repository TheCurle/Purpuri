/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Class.hpp"

#include <iostream>
#include <fstream>
#include <sys/stat.h>

Class::Class() {
    ClassHeap = NULL;
    Code = NULL;
    BytecodeLength = 0;
    FieldsCount = 0;
}

// TODO: Delete EVERYTHING
Class::~Class() {
}

bool Class::LoadFromFile(const char* Filename) {
    char* LocalCode;
    size_t LengthRead, Length;
    int Temp;

    std::ifstream File(Filename, std::ios::binary);

    if(!File.is_open()) {
        printf("Unable to open class file: %s\n", Filename);
        return false;
    }

    struct stat statbuf;
    Temp = stat(Filename, &statbuf);
    Length = Temp == 0 ? statbuf.st_size : -1;

    if(Length < 0) {
        puts("Class file is empty or corrupt");
        return false;
    }

    LocalCode = (char*) new char[Length + 2];
    if(LocalCode) {

        File.read(&LocalCode[0], Length);
    } else {

        File.close();
        return false;
    }

    File.close();

    if(LocalCode) {
        LocalCode[Length] = 0;
        BytecodeLength = Length;
        SetCode(LocalCode);
    }

    return LocalCode != NULL;
}

void Class::SetCode(const char* p_Code) {
    if(Code) delete Code;
    Code = p_Code;

    if(Code) ParseFullClass();
}

bool Class::ParseFullClass() {
    if(Code == NULL || BytecodeLength < sizeof(ClassFile) + 20)
        return false;

    puts("New Class incoming..");
    
    MagicNumber = (Code[0] & 0xFF) << 24 | (Code[1] & 0xFF) << 16 | (Code[2] & 0xFF) << 8 | Code[3] & 0xFF;
    Code += 4;

    if(MagicNumber != 0xCAFEBABE) {
        printf("Magic number 0x%x does not match 0xCAFEBABE\n", MagicNumber);
        return false;
    }

    printf("Magic: 0x%x\n", MagicNumber);

    BytecodeVersionMinor = Code[0] << 8 | Code[1];
    Code += 2;
    BytecodeVersionMajor = Code[0] << 8 | Code[1];
    Code += 2;

    printf("Bytecode Version %d.%d\n", BytecodeVersionMajor, BytecodeVersionMinor);

    ConstantCount = Code[0] << 8 | Code[1];
    Code += 2;

    if(ConstantCount > 0) 
        ParseConstants(Code);

    ClassAccess = Code[0] << 8 | Code[1];
    Code += 2;
    This = Code[0] << 8 | Code[1];
    Code += 2;
    Super = Code[0] << 8 | Code[1];
    Code += 2;

    InterfaceCount = Code[0] << 8 | Code[1];
    Code += 2;

    if(InterfaceCount > 0) {
        puts("NYI: Interfaces");
        return false;
    }

    FieldsCount = Code[0] << 8 | Code[1];
    Code += 2;

    if(FieldsCount > 0) {
        puts("NYI: Fields");
        return false;
    }

    MethodCount = Code[0] << 8 | Code[1];
    Code += 2;

    if(MethodCount > 0) 
        ParseMethods(Code);
    
    AttributeCount = Code[0] << 8 | Code[1];

    if(AttributeCount > 0) 
        ParseAttribs(Code);

    return 0;
}

bool Class::ParseAttribs(const char *&Code) {
    Attributes = new AttributeData* [AttributeCount];
    puts("Reading class Attributes");
    if(Methods == NULL) return false;

    char* Name;

    for(int j = 0; j < AttributeCount; j++) {
        printf("\tParsing attribute %d\n", j);
        Attributes[j] = (AttributeData*) Code;

        uint16_t AttrName = Code[0] << 8 | Code[1];
        Code += 2;
        size_t AttrLength = Code[0] << 24 | Code[1] << 16 | Code[2] << 8 | Code[3];
        Code += 4;
        Code += AttrLength;

        //GetStringConstant(AttrName, Name);
        printf("\tAttribute has id %d, length %zu\n", AttrName, AttrLength);
            
    }

    return true;
}

bool Class::ParseMethods(const char *&Code) {
    Methods = new Method[MethodCount];

    if(Methods == NULL) return false;

    for(int i = 0; i < MethodCount; i++) {
        Methods[i].Data = (MethodData*) Code;
        Methods[i].Access = Code[0] << 8 | Code[1]; Code += 2;
        Methods[i].Name = Code[0] << 8 | Code[1]; Code += 2;
        Methods[i].Descriptor = Code[0] << 8 | Code[1]; Code += 2;
        Methods[i].AttributeCount = Code[0] << 8 | Code[1]; Code += 2;

        char* Name, *Descriptor;
        GetStringConstant(Methods[i].Name, Name);
        GetStringConstant(Methods[i].Descriptor, Descriptor);

        printf("Parsing method %s%s with access %d, %d attributes\n", Name, Descriptor, Methods[i].Access, Methods[i].AttributeCount);

        Methods[i].Code = NULL;

        if(Methods[i].AttributeCount > 0) {
            for(int j = 0; j < Methods[i].AttributeCount; j++) {
                printf("\tParsing attribute %d\n", j);
                uint16_t AttrName = Code[0] << 8 | Code[1];
                Code += 2;

                size_t AttrLength = Code[0] << 24 | Code[1] << 16 | Code[2] << 8 | Code[3];
                Code += 4;
                Code += AttrLength;
                GetStringConstant(AttrName, Name);

                printf("\tAttribute has name %s, length %zu\n", Name, AttrLength);
            }

            Methods[i].Code = new CodePoint;
            ParseMethodCodePoints(i, Methods[i].Code);
        }
    }

    return true;
}


bool Class::ParseMethodCodePoints(int Method, CodePoint *Code) {
    if(Methods == NULL || Method > MethodCount) return false;

    char* CodeBase = (char*)Methods[Method].Data;
    CodeBase += 6; // Skip Access, Name, Descriptor
    int AttribCount = CodeBase[0] << 8 | CodeBase[1];
    CodeBase += 2;

    if(AttribCount > 0) {
        for(int i = 0; i < AttribCount; i++) {
            uint16_t NameInd = CodeBase[0] << 8 | CodeBase[1];
            CodeBase += 2;

            char* AttribName;
            GetStringConstant(NameInd, AttribName);

            if(strcmp(AttribName, "Code")) {
                char* AttribCode = CodeBase;
                Code->Name = NameInd;
                Code->Length = AttribCode[0] << 24 | AttribCode[1] << 16 | AttribCode[2] << 8 | AttribCode[3]; AttribCode += 4;
                Code->StackSize = AttribCode[0] << 8 | AttribCode[1]; AttribCode += 2;
                Code->LocalsSize = AttribCode[0] << 8 | AttribCode[1]; AttribCode += 2;
                Code->CodeLength = AttribCode[0] << 24 | AttribCode[1] << 16 | AttribCode[2] << 8 | AttribCode[3]; AttribCode += 4;

                if(Code->CodeLength > 0) {
                    Code->Code = new uint8_t[Code->CodeLength];

                    memcpy(Code->Code, AttribCode, Code->CodeLength);
                } else {
                    Code->Code = NULL;
                }

                AttribCode += Code->CodeLength;

                Code->ExceptionCount = AttribCode[0] << 8 | AttribCode[1];
                if(Code->ExceptionCount > 0) {
                    Code->Exceptions = new Exception[Code->ExceptionCount];

                    for(int Exc = 0; Exc < Code->ExceptionCount; Exc++) {
                        Code->Exceptions[Exc].PCStart = AttribCode[0] << 8 | AttribCode[1]; AttribCode += 2;
                        Code->Exceptions[Exc].PCEnd = AttribCode[0] << 8 | AttribCode[1]; AttribCode += 2;
                        Code->Exceptions[Exc].PCHandler = AttribCode[0] << 8 | AttribCode[1]; AttribCode += 2;
                        Code->Exceptions[Exc].CatchType = AttribCode[0] << 8 | AttribCode[1]; AttribCode += 2;
                    }
                }
            }

            uint32_t Length = CodeBase[0] << 24 | CodeBase[1] << 16 | CodeBase[2] << 8 | CodeBase[3]; CodeBase += 4;
            CodeBase += Length;
        }
    }

    return 0;
}


uint32_t Class::GetClassSize() {
    uint32_t Size = FieldsCount * sizeof(Variable);

    Class* Super = GetSuper();

    uint32_t SuperSize = 0;
    if(Super)
        SuperSize = Super->GetClassSize();
    
    Size += SuperSize;
    return Size;
}

uint32_t Class::GetClassFieldCount() {
    uint32_t Count = FieldsCount;

    Class* Super = GetSuper();

    uint32_t SuperSize = 0;
    if(Super)
        SuperSize = Super->GetClassFieldCount();
    
    Count += SuperSize;
    return Count;
}

Class* Class::GetSuper() {
    return ClassHeap->GetClass(GetSuperName());
}

char* Class::GetSuperName() {
    return this->GetName(Super);
}

char* Class::GetClassName() {
    return this->GetName(This);
}

char* Class::GetName(uint16_t Obj) {
    char* Ret = new char[0];
    if(Obj < 1 || (Obj != Super && Obj != This)) return Ret;

    // 7 = class
    if(Constants[Obj]->Tag != 7)
        return Ret;
    
    char* Entry = (char*)Constants[Obj];
    uint16_t Name = (&Entry[1])[0] << 8 | (&Entry[1])[1];

    GetStringConstant(Name, Ret);
    return Ret;
}