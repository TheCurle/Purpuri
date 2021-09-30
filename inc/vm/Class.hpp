/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#pragma once
#include <map>
#include <string>
#include <cstring>
#include <list>
#include <vector>

#include "Constants.hpp"
#include "Fields.hpp"
#include "Methods.hpp"
#include "Objects.hpp"

class Engine {
    public:
        static ObjectHeap _ObjectHeap;
        static bool QuietMode;
        ClassHeap* _ClassHeap;

        Engine();
        virtual ~Engine();
        virtual uint32_t Ignite(StackFrame* Stack);

        void Invoke(StackFrame* Stack, uint16_t Type);

        void InvokeNative(NativeContext Context);
        void HandleNativeReturn(NativeContext Context, Variable Value);

        bool MethodClassMatches(uint16_t MethodInd, Class* pClass, const char* TestName);

        void PutField(StackFrame* Stack);
        void GetField(StackFrame* Stack);

        Variable GetConstant(Class* Class, uint8_t Index);

        uint16_t GetParameters(const char* Descriptor);
        uint16_t GetParametersStack(const char* Descriptor);

        int New(StackFrame* Stack);
        void NewArray(StackFrame* Stack);
        void ANewArray(StackFrame* Stack);

        Variable CreateObject(Class* Class);
        Variable* CreateArray(uint8_t Type, int32_t Count);
        
        void DumpObject(Object Object);
};

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
    std::list<std::string> ClassCache;
    std::map<std::string, Class*> ClassMap;

    public:
        ClassHeap();
        
        static std::string UnknownClass;
        std::string ClassPrefix;

        bool LoadClass(const char* ClassName, Class* Class);
        bool AddClass(Class* Class);
        bool ClassExists(std::string Name);
        Class* GetClass(std::string Name);
        
        std::list<std::string> GetAllClasses() { return ClassCache; }
        
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
        std::string GetStringConstant(uint32_t index);

        std::string GetClassName();
        std::string GetSuperName();

        virtual uint32_t GetClassSize();
        virtual uint32_t GetClassFieldCount();

        Class* GetSuper();

        uint32_t GetMethodFromDescriptor(const char* MethodName, const char* Descriptor, const char* ClassName, Class* &Class);
        uint32_t GetFieldFromDescriptor(std::string FieldAndDescriptor);


        Object CreateObject(uint16_t Index, ObjectHeap* ObjectHeap);
        bool CreateObjectArray(uint16_t Index, uint32_t Count, ObjectHeap ObjectHeap, Object &Object);

        void SetClassHeap(ClassHeap* p_ClassHeap) {
            this->_ClassHeap = p_ClassHeap;
        }

    private:
        size_t BytecodeLength;
        size_t LoadedLocation;
        const char* Code;
        ClassHeap* _ClassHeap;
        uint16_t FieldsCount;
        std::vector<std::string> StringConstants;

        bool ParseConstants(const char* &Code);
        uint32_t GetConstantsCount(const char* Constants);

        std::string GetName(uint16_t ThisOrSuper);
};

