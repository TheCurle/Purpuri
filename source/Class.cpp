/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Class.hpp"

#include <iostream>
#include <fstream>
#include <sys/stat.h>

Class::Class() {
    _ClassHeap = NULL;
    Code = NULL;
    BytecodeLength = 0;
    FieldsCount = 0;
}

// TODO: Delete EVERYTHING
Class::~Class() {
}

bool Class::LoadFromFile(const char* Filename) {
    char* LocalCode;
    size_t Length;
    int Temp;

    std::ifstream File(Filename, std::ios::binary);

    if(!File.is_open()) {
        printf("Unable to open class file: %s\n", Filename);
        return false;
    }

    struct stat statbuf;
    Temp = stat(Filename, &statbuf);
    Length = Temp == 0 ? statbuf.st_size : -1;

    if(Length == -1ULL) {
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
    
    MagicNumber = ReadIntFromStream(Code);
    Code += 4;

    if(MagicNumber != 0xCAFEBABE) {
        printf("Magic number 0x%x does not match 0xCAFEBABE\n", MagicNumber);
        return false;
    }

    printf("Magic: 0x%x\n", MagicNumber);

    BytecodeVersionMinor = ReadShortFromStream(Code); Code += 2;
    BytecodeVersionMajor = ReadShortFromStream(Code); Code += 2;

    printf("Bytecode Version %d.%d\n", BytecodeVersionMajor, BytecodeVersionMinor);

    ConstantCount = ReadShortFromStream(Code); Code += 2;

    if(ConstantCount > 0) 
        ParseConstants(Code);

    ClassAccess = ReadShortFromStream(Code); Code += 2;
    This = ReadShortFromStream(Code); Code += 2;
    Super = ReadShortFromStream(Code); Code += 2;

    InterfaceCount = ReadShortFromStream(Code); Code += 2;

    if(InterfaceCount > 0)
        ParseInterfaces(Code);

    FieldsCount = ReadShortFromStream(Code); Code += 2;

    if(FieldsCount > 0) {
        puts("NYI: Fields");
        return false;
    }

    MethodCount = ReadShortFromStream(Code); Code += 2;

    if(MethodCount > 0) 
        ParseMethods(Code);
    
    AttributeCount = ReadShortFromStream(Code); Code += 2;

    if(AttributeCount > 0) 
        ParseAttribs(Code);

    ClassloadReferents(Code);

    return 0;
}

void Class::ClassloadReferents(const char* &Code) {
    SHUTUPUNUSED(Code);

    printf("Searching for unloaded classes referenced by the current.\n");
    
    for(int i = 1; i < ConstantCount; i++) {
        if(Constants == NULL) return;
        if(Constants[i] == NULL) continue;

        if(Constants[i]->Tag == TypeClass) {
            char* Temp = (char*)Constants[i];
            uint16_t val = ReadShortFromStream(&Temp[1]);
            GetStringConstant(val, Temp);
            printf("\tName %s", Temp);

            if(i != Super && i != This && this->_ClassHeap->GetClass(Temp) == NULL) {
                printf("\tClass is not loaded - invoking the classloader\n");
                Class* Class = new class Class();
                this->_ClassHeap->LoadClass(Temp, Class);
            } else {
                printf(" - clear\n");
            }
        }
    }
}

bool Class::ParseAttribs(const char *&Code) {
    Attributes = new AttributeData* [AttributeCount];
    puts("Reading class Attributes");
    if(Methods == NULL) return false;

    for(int j = 0; j < AttributeCount; j++) {
        printf("\tParsing attribute %d\n", j);
        Attributes[j] = (AttributeData*) Code;

        uint16_t AttrName = ReadShortFromStream(Code); Code += 2;
        size_t AttrLength = ReadIntFromStream(Code); Code += 4;
        Code += AttrLength;

        //GetStringConstant(AttrName, Name);
        printf("\tAttribute has id %d, length %zu\n", AttrName, AttrLength);
            
    }

    return true;
}

bool Class::ParseInterfaces(const char* &Code) {
    Interfaces = new uint16_t[InterfaceCount];
    printf("Reading %d interfaces\n", InterfaceCount);

    for(int i = 0; i < InterfaceCount; i++) {
        Interfaces[i] = ReadShortFromStream(Code);
        Code += 2;

        char* Stream;
        Stream = (char*) Constants[Interfaces[i]];
        uint16_t NameInd = ReadShortFromStream(&Stream[1]);
        GetStringConstant(NameInd, Stream);

        printf("\tThis class implements interface %s\n", Stream);
    }

    return true;
}

bool Class::ParseMethods(const char *&Code) {
    Methods = new Method[MethodCount];

    if(Methods == NULL) return false;

    for(int i = 0; i < MethodCount; i++) {
        Methods[i].Data = (MethodData*) Code;
        Methods[i].Access = ReadShortFromStream(Code); Code += 2;
        Methods[i].Name = ReadShortFromStream(Code); Code += 2;
        Methods[i].Descriptor = ReadShortFromStream(Code); Code += 2;
        Methods[i].AttributeCount = ReadShortFromStream(Code); Code += 2;

        char* Name, *Descriptor;
        GetStringConstant(Methods[i].Name, Name);
        GetStringConstant(Methods[i].Descriptor, Descriptor);

        printf("Parsing method %s%s with access %d, %d attributes\n", Name, Descriptor, Methods[i].Access, Methods[i].AttributeCount);

        Methods[i].Code = NULL;

        if(Methods[i].AttributeCount > 0) {
            for(int j = 0; j < Methods[i].AttributeCount; j++) {
                printf("\tParsing attribute %d\n", j);
                uint16_t AttrName = ReadShortFromStream(Code); Code += 2;

                size_t AttrLength = ReadIntFromStream(Code); Code += 4;
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
    int AttribCount = ReadShortFromStream(CodeBase); CodeBase += 2;

    if(AttribCount > 0) {
        for(int i = 0; i < AttribCount; i++) {
            uint16_t NameInd = ReadShortFromStream(CodeBase); CodeBase += 2;

            char* AttribName;
            GetStringConstant(NameInd, AttribName);

            if(!strcmp(AttribName, "Code")) {
                char* AttribCode = CodeBase;
                Code->Name = NameInd;
                Code->Length = ReadIntFromStream(AttribCode); AttribCode += 4;
                Code->StackSize = ReadShortFromStream(AttribCode); AttribCode += 2;
                Code->LocalsSize = ReadShortFromStream(AttribCode); AttribCode += 2;
                Code->CodeLength = ReadIntFromStream(AttribCode); AttribCode += 4;

                if(Code->CodeLength > 0) {
                    Code->Code = new uint8_t[Code->CodeLength];

                    memcpy(Code->Code, AttribCode, Code->CodeLength);
                } else {
                    Code->Code = NULL;
                }

                AttribCode += Code->CodeLength;

                Code->ExceptionCount = ReadShortFromStream(AttribCode);
                if(Code->ExceptionCount > 0) {
                    Code->Exceptions = new Exception[Code->ExceptionCount];

                    for(int Exc = 0; Exc < Code->ExceptionCount; Exc++) {
                        Code->Exceptions[Exc].PCStart = ReadShortFromStream(AttribCode); AttribCode += 2;
                        Code->Exceptions[Exc].PCEnd = ReadShortFromStream(AttribCode); AttribCode += 2;
                        Code->Exceptions[Exc].PCHandler = ReadShortFromStream(AttribCode); AttribCode += 2;
                        Code->Exceptions[Exc].CatchType = ReadShortFromStream(AttribCode); AttribCode += 2;
                    }
                }
            }

            uint32_t Length = ReadIntFromStream(CodeBase); CodeBase += 4;
            CodeBase += Length;
        }
    }

    return 0;
}

uint32_t Class::GetMethodFromDescriptor(char *MethodName, char *Descriptor, Class *&Class) {
    if(Methods == NULL) {
        puts("GetMethodFromDescriptor called too early! Class not initialised yet!");
        return false;
    }

    class Class* CurrentClass = this;
    while(CurrentClass) {
        printf("Searching class %s for %s%s\n", CurrentClass->GetClassName(), MethodName, Descriptor);
        char* l_Name, *l_Descriptor;
        for(int i = 0; i < CurrentClass->MethodCount; i++) {
            CurrentClass->GetStringConstant(CurrentClass->Methods[i].Name, l_Name);
            CurrentClass->GetStringConstant(CurrentClass->Methods[i].Descriptor, l_Descriptor);

            if(!strcmp(MethodName, l_Name) && !strcmp(Descriptor, l_Descriptor)) {
                if(Class) Class = CurrentClass;

                printf("Found at index %d\n", i);
                return i;
            }
        }

        if(Class != NULL)
            CurrentClass = CurrentClass->GetSuper();
        else
            break;

    }

    return -1;
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
    return _ClassHeap->GetClass(GetSuperName());
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

    if(Constants[Obj]->Tag != TypeClass)
        return Ret;
    
    char* Entry = (char*)Constants[Obj];
    uint16_t Name = ReadShortFromStream(&Entry[1]);

    GetStringConstant(Name, Ret);
    return Ret;
}

bool Class::CreateObject(uint16_t Index, ObjectHeap *ObjectHeap, Object &Object) {
    uint8_t* Code = (uint8_t*) this->Constants[Index];

    if(Code[0] != TypeClass)
        return false;
    
    uint16_t Name = ReadShortFromStream(&Code[1]);
    char* NameStr;
    if(!this->GetStringConstant(Name, NameStr))
        return false;
    
    printf("Creating new object from class %s\n", NameStr);

    Class* NewClass = this->_ClassHeap->GetClass((char*) this->_ClassHeap->ClassPrefix.append(NameStr).c_str());
    if(NewClass == NULL) return false;

    Object = ObjectHeap->CreateObject(NewClass);
    return true;
}