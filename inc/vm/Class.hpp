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
#include <filesystem>

#include "Constants.hpp"
#include "Fields.hpp"
#include "Methods.hpp"
#include "Objects.hpp"
#include "ZipFile.hpp"

class Engine {
    public:
        static ObjectHeap _ObjectHeap;
        static bool QuietMode;
        ClassHeap* _ClassHeap;
        StackFrame* ClassloadingStack;
        bool clinit;

        Engine();
        virtual ~Engine();
        uint32_t Ignite(StackFrame* Stack, bool clinit = false);

        void Invoke(StackFrame* Stack, uint16_t Type);

        void InvokeNative(const NativeContext& Context);

        void PutStatic(StackFrame* Stack) const;
        void PutField(StackFrame* Stack);
        void GetStatic(StackFrame* Stack);
        void GetField(StackFrame* Stack);

        Variable GetConstant(Class* Class, uint8_t Index) const;

        uint16_t GetParameters(const char* Descriptor);

        int New(StackFrame* Stack);
        void NewArray(StackFrame* Stack);
        void ANewArray(StackFrame* Stack);
        
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

/**
 * A class information can either be inside a zip archive, or on the filesystem.
 * Passing the information out each time can be troublesome.
 * This packages it up a little more cleanly.
 */

struct ClassLocation {
    bool inZip;
    bool isValid;
    std::filesystem::path FSPath;
    std::string ZipPath;
    ZipFile* Zip;
};

class ClassHeap {
    std::list<std::string> ClassCache;
    std::map<std::string, Class*> ClassMap;
    std::list<ClassLocation> ClassPath;
    std::list<ClassLocation> BootstrapClasspath;
    std::list<ZipFile*> ZipsInUse;

    public:
        ClassHeap();
        
        static std::string UnknownClass;
        std::string ClassPrefix;

        void AddToClassPath(std::string);
        void AddToBootstrapClasspath(std::string);
        ClassLocation SearchClassPath(std::string&);

        bool LoadClass(const char* ClassName, Class* pClass, StackFrame* frame = nullptr, Engine* engine = nullptr);
        bool AddClass(Class* pClass);
        bool ClassExists(const std::string& Name);
        Class* GetClass(const std::string& Name, StackFrame* frame = nullptr, Engine* engine = nullptr);

        std::list<Class*> GetAllClasses() { 
            std::list<Class*> list;
            for(auto& it : ClassMap)
                list.push_back(it.second);

            return list;
        }
        
};

class StackFrame {
public:
    static Variable* MemberStack;
    static Variable* ClassloadStack;
    Class* _Class;
    Method* _Method;
    uint32_t ProgramCounter;
    uint16_t StackPointer;
    Variable* Stack;

    StackFrame() {
        StackPointer = -1;
        ProgramCounter = 0;
        _Class = nullptr;
        Stack = nullptr;
    }

    StackFrame(int16_t StackPointer) {
        this->StackPointer = StackPointer;
        _Class = nullptr;
        Stack = nullptr;
        ProgramCounter = 0;
    }
};

class Class : public ClassFile {
    public:
        Class();
        virtual ~Class();

        virtual bool LoadFromFile(const char* Filename);
        virtual bool LoadFromMemory(char* Data, size_t size);
        void SetCode(const char* Code);

        bool ParseFullClass();
        bool ParseInterfaces(const char* &ClassCode);
        bool ParseFields(const char* &ClassCode);
        bool ParseMethods(const char* &ClassCode);
        bool ParseMethodCodePoints(int Method, CodePoint* MethodCode);
        bool ParseAttribs(const char* &ClassCode);

        std::string GetStringConstant(uint32_t index);

        std::string GetClassName();
        std::string GetSuperName();

        virtual uint32_t GetClassSize();
        virtual uint32_t GetClassFieldCount();

        bool PutStatic(uint16_t Field, Variable Value);
        Variable GetStatic(uint16_t Field);

        Class* GetSuper();

        uint32_t GetMethodFromDescriptor(const char* MethodName, const char* Descriptor, const char* ClassName, Class* &Class);
        uint32_t GetFieldFromDescriptor(const std::string& FieldAndDescriptor);


        Object CreateObject(uint16_t Index, ObjectHeap* ObjectHeap);
        bool CreateObjectArray(uint16_t Index, uint32_t Count, ObjectHeap ObjectHeap, Object &Object);

        void SetClassHeap(ClassHeap* p_ClassHeap) {
            this->_ClassHeap = p_ClassHeap;
        }

        void RunClassloadInit(StackFrame* Stack, Engine* engine);

    private:
        size_t BytecodeLength;
        const char* Code;

        ClassHeap* _ClassHeap;
        uint16_t FieldsCount;
        std::vector<std::string> StringConstants;

        Variable* ClassStatics {};
        std::vector<size_t> StaticFieldIndexes;

        bool ParseConstants(const char* &pCode);
        uint32_t GetConstantsCount(const char* pCode);

        std::string GetName(uint16_t ThisOrSuper);
};

