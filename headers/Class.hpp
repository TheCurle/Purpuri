/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#pragma once
#include <map>

#include "Constants.hpp"
#include "Fields.hpp"
#include "Methods.hpp"
#include "Objects.hpp"


struct ClassFile {
    uint32_t MagicNumber;
    uint16_t BytecodeVersionMajor;
    uint16_t BytecodeVersionMinor;

    uint16_t ConstantCount;
    struct ConstantPoolEntry** Constants;

    uint16_t ClassAccess;
    uint16_t This;
    uint16_t Super;

    uint16_t InterfaceCount;
    uint16_t* Interfaces;

    uint16_t FieldCount;
    struct FieldData** Fields;

    uint16_t MethodCount;
    struct Method* Methods;

    uint16_t AttributeCount;
    struct AttributeData** Attributes;
};

class ClassHeap {
    std::map<std::string, Class*> ClassMap;

    public:
        ClassHeap();
        std::string ClassPrefix;

        bool LoadClass(char* ClassName, Class* Class);
        bool AddClass(Class* Class);
        Class* GetClass(char* Name);
        
};

class Class : public ClassFile {
    public:
        Class();
        virtual ~Class();

        virtual bool LoadFromFile(const char* Filename);
        void SetCode(const char* Code);

        bool ParseFullClass();
        bool ParseInterfaces(const char* &Code);
        bool ParseFields(const char* &Code);
        bool ParseMethods(const char* &Code);
        bool ParseMethodCodePoints(int Method, CodePoint* Code);
        bool ParseAttribs(const char* &Code);

        void ClassloadReferents(const char* &Code);

        bool GetConstants(uint16_t index, ConstantPoolEntry &Pool);
        bool GetStringConstant(uint32_t index, char* &String);

        char* GetClassName();
        char* GetSuperName();

        virtual uint32_t GetClassSize();
        virtual uint32_t GetClassFieldCount();

        Class* GetSuper();

        uint32_t GetMethodFromDescriptor(char* MethodName, char* Descriptor, Class* &Class);
        uint32_t GetFieldFromDescriptor(char* FieldName, char* &FieldDescriptor);

        bool CreateObject(uint16_t Index, ObjectHeap* ObjectHeap, Object &Object);
        bool CreateObjectArray(uint16_t Index, uint32_t Count, ObjectHeap* ObjectHeap, Object &Object);

        void SetClassHeap(ClassHeap* p_ClassHeap) {
            this->_ClassHeap = p_ClassHeap;
        }

    private:
        size_t BytecodeLength;
        const char* Code;
        struct ClassHeap* _ClassHeap;
        uint16_t FieldsCount;

        bool ParseConstants(const char* &Code);
        uint32_t GetConstantsCount(const char* Constants);

        char* GetName(uint16_t ThisOrSuper);
};

