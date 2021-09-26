/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Class.hpp>

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <list>
#include <algorithm>
#include <string>

Class::Class() {
    _ClassHeap = NULL;
    Code = NULL;
    BytecodeLength = 0;
    FieldsCount = 0;

    LoadedLocation = (size_t)(size_t*) this;

    Unknown.append("Unknown Value");
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
    bool hasSize = (LocalCode != nullptr);
    if(hasSize) {
        File.read(&LocalCode[0], Length);
    }

    File.close();

    if(hasSize) {
        LocalCode[Length] = 0;
        BytecodeLength = Length;
        SetCode(LocalCode);
    }

    return hasSize;
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

    printf("Class has access %d.\r\n", ClassAccess);
    This = ReadShortFromStream(Code); Code += 2;
    Super = ReadShortFromStream(Code); Code += 2;

    InterfaceCount = ReadShortFromStream(Code); Code += 2;

    if(InterfaceCount > 0)
        ParseInterfaces(Code);

    FieldsCount = ReadShortFromStream(Code); Code += 2;

    if(FieldsCount > 0) {
        ParseFields(Code);
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
            uint16_t ClassInd = ReadShortFromStream(&Temp[1]);
            std::string ClassName = GetStringConstant(ClassInd);

            if(ClassName.find_first_of('[') == 0) continue;

            printf("\tName %s", ClassName.c_str());

            if(i != Super && i != This && (this->_ClassHeap->GetClass(ClassName) == NULL && !this->_ClassHeap->ClassExists(ClassName))) {
                printf("\tClass is not loaded - invoking the classloader\n");
                Class* Class = new class Class();

                if(!this->_ClassHeap->LoadClass(ClassName.c_str(), Class)) {
                    printf("Classloading referenced class %s failed. Fatal error.\n", ClassName.c_str());
                    exit(6);
                }
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

        printf("\tThis class implements interface %s\n", GetStringConstant(NameInd).c_str());
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

        std::string Name = GetStringConstant(Methods[i].Name);
        std::string Descriptor = GetStringConstant(Methods[i].Descriptor);

        printf("Parsing method %s%s with access %d, %d attributes\n", Name.c_str(), Descriptor.c_str(), Methods[i].Access, Methods[i].AttributeCount);

        Methods[i].Code = NULL;

        if(Methods[i].AttributeCount > 0) {
            for(int j = 0; j < Methods[i].AttributeCount; j++) {
                printf("\tParsing attribute %d\n", j);
                uint16_t AttrNameInd = ReadShortFromStream(Code); Code += 2;

                size_t AttrLength = ReadIntFromStream(Code); Code += 4;
                Code += AttrLength;
                
                std::string AttrName = GetStringConstant(AttrNameInd);

                printf("\tAttribute has name %s, length %zu\n", AttrName.c_str(), AttrLength);
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

            std::string AttribName = GetStringConstant(NameInd);

            if(AttribName.compare("Code") == COMPARE_MATCH) {
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

bool Class::ParseFields(const char* &Code) {
    Fields = new FieldData*[FieldsCount];

    if(Fields == NULL) return false;

    for(size_t i = 0; i < FieldsCount; i++) {
        Fields[i] = (FieldData*) Code;

        Fields[i]->Access = ReadShortFromStream(Code); Code += 2;
        Fields[i]->Name = ReadShortFromStream(Code); Code += 2;
        Fields[i]->Descriptor = ReadShortFromStream(Code); Code += 2;
        Fields[i]->AttributeCount = ReadShortFromStream(Code); Code += 2;

        if(Fields[i]->AttributeCount > 0) {
            for(int attr = 0; attr < Fields[i]->AttributeCount; attr++) {
                Code += 2;
                size_t Length = ReadIntFromStream(Code); Code += 4;
                Code += Length;
            }
        }
    }

    return true;
}

uint32_t Class::GetMethodFromDescriptor(const char *MethodName, const char *Descriptor, const char* ClassName, Class *&pClass) {
    if(Methods == NULL) {
        puts("GetMethodFromDescriptor called too early! Class not initialised yet!");
        return false;
    }
    
    class Class* CurrentClass = pClass;
    std::string ClassNameStr(ClassName);

    while(CurrentClass) {
        std::string MethodClass = CurrentClass->GetClassName();

        printf("Searching class %s for %s%s\n", MethodClass.c_str(), MethodName, Descriptor);
        std::list<std::string> InterfaceNames;

        if(CurrentClass->InterfaceCount > 0) {
            for(size_t iface = 0; iface < CurrentClass->InterfaceCount; iface++) {
                uint16_t interface = CurrentClass->Interfaces[iface];

                char* Stream = (char*) CurrentClass->Constants[interface];
                uint16_t NameInd = ReadShortFromStream(&Stream[1]);

                std::string IfaceName = GetStringConstant(NameInd);
                InterfaceNames.emplace_back(IfaceName);
            }
        }

        if(InterfaceNames.size() > 0) {
            printf("Class implements interfaces ");
            PrintList(InterfaceNames);
            printf("\n");
        }

        for(int i = 0; i < CurrentClass->MethodCount; i++) {
            std::string CurrentName = CurrentClass->GetStringConstant(CurrentClass->Methods[i].Name);
            std::string CurrentDescriptor = CurrentClass->GetStringConstant(CurrentClass->Methods[i].Descriptor);

            printf("\t\tExamining class %s for %s%s, access %d\r\n", MethodClass.c_str(), CurrentName.c_str(), CurrentDescriptor.c_str(), CurrentClass->ClassAccess);

            // if method and descriptor and class match,
            // or method and descriptor match, and class is interface.
            if(((CurrentName.compare(MethodName) == 0) && (CurrentDescriptor.compare(Descriptor)) == 0) && 
            ((MethodClass.compare(ClassName) == 0) || (std::find(InterfaceNames.begin(), InterfaceNames.end(), ClassNameStr) != InterfaceNames.end()) )) {
                if(pClass) pClass = CurrentClass;

                printf("Found at index %d\n", i);
                return i;
            }
        }

        if(pClass != NULL)
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

std::string Class::GetSuperName() {
    return this->GetName(Super);
}

std::string Class::GetClassName() {
    return this->GetName(This);
}

std::string Class::GetName(uint16_t Obj) {
    if(Obj < 1 || (Obj != Super && Obj != This)) return Unknown;

    char* Entry = (char*)Constants[Obj];
    uint16_t NameInd = ReadShortFromStream(&Entry[1]);

    return GetStringConstant(NameInd);
}

bool Class::CreateObject(uint16_t Index, ObjectHeap* ObjectHeap, Object &Object) {
    uint8_t* Code = (uint8_t*) this->Constants[Index];

    if(Code[0] != TypeClass)
        return false;

    uint16_t NameInd = ReadShortFromStream(&Code[1]);
    std::string Name = GetStringConstant(NameInd);

    printf("Creating new object from class %s\n", Name.c_str());

    Class* NewClass = this->_ClassHeap->GetClass(Name);
    if(NewClass == NULL) return false;

    Object = ObjectHeap->CreateObject(NewClass);
    return true;
}

bool Class::CreateObjectArray(uint16_t Index, uint32_t Count, ObjectHeap ObjectHeap, Object &pObject) {
    std::string ClassName = GetStringConstant(Index);

    printf("Creating array of objects from class %s\n", ClassName.c_str());

    Class* newClass = this->_ClassHeap->GetClass(ClassName.c_str());
    if(newClass == NULL) return false;

    return ObjectHeap.CreateObjectArray(newClass, Count, pObject);
}

void PrintList(std::list<std::string> &list) {
    for (auto const& i: list) {
        puts(i.c_str());
    }
}