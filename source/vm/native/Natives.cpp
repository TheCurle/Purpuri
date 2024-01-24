/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Class.hpp>
#include <vm/Native.hpp>
#if defined WIN32
    #include <windows.h>
    #include <libloaderapi.h>
    #undef GetClassName
    #undef LoadLibrary
    #undef GetObject
#elif defined linux || defined __APPLE__
    #include <dlfcn.h>
#endif

#include <regex>


std::map<std::string, void*> Native::LibraryHandleMap;

std::vector<Variable> Native::Parameters;

void VM::Return(Variable retVal) {
    NativeReturn Return;
    Return.Value = retVal;

    throw Return;
}

std::vector<Variable> VM::GetParameters() {
    return Native::Parameters;
}

Variable* VM::GetObject(Object ID) {
    return Engine::_ObjectHeap.GetObjectPtr(ID);
}

size_t VM::GetArrayLength(Object ID) {
    return Engine::_ObjectHeap.GetArraySize(ID);
}

// Find and replace all instances of a substring inside a std::string
void ICantBelieveThisIsNeededWithModernCPP(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

std::string Native::EncodeName(NativeContext Context) {
    std::string MangledName("Java_");
    // Class name with every . replaced by _
    std::string ClassName(Context.ClassName);
    ICantBelieveThisIsNeededWithModernCPP(ClassName, "/", "_");
    MangledName.append(ClassName).append("_");

    // Function name
    MangledName.append(Context.MethodName).append("_");

    // Get the middle portion of the descriptor
    std::string Descriptor(Context.MethodDescriptor);
    std::string Parameters = Descriptor.substr(Descriptor.find_first_of("(") + 1, Descriptor.find_first_of(")") - 1);
    
    // Replace / with _ (Lnet_company_Class;)
    // Replace ; with E (Lnet_company_ClassE)
    ICantBelieveThisIsNeededWithModernCPP(Parameters, "/", "_");
    ICantBelieveThisIsNeededWithModernCPP(Parameters, ";", "E");
    if(Parameters.empty()) Parameters = "V";
    MangledName.append(Parameters).append("_");

    std::string ReturnType = Descriptor.substr(Descriptor.find_first_of(")") + 1, Descriptor.size());
    ICantBelieveThisIsNeededWithModernCPP(ReturnType, "/", "_");
    ICantBelieveThisIsNeededWithModernCPP(ReturnType, ";", "E");
    MangledName.append(ReturnType).append("_");

    // Java_uk_gemwire_purpuri_run_IIZ_Z_

    return MangledName;
}

void* Native::LoadLibrary(std::string LibraryName) {
    #ifdef WIN32
        void* Handle = LoadLibraryA(LibraryName.c_str());    
    #elif defined linux || defined __APPLE__
        void* Handle = dlopen(LibraryName.c_str(), RTLD_NOW | RTLD_LOCAL);
    #endif

    LibraryHandleMap.emplace(LibraryName, Handle);
    return Handle;
}

function_t Native::LoadSymbol(std::string LibraryName, std::string SymbolName) {
    #ifdef WIN32
        return GetProcAddress((HMODULE) LibraryHandleMap.at(LibraryName), SymbolName.c_str());
    #elif defined __linux__ || defined __APPLE__
        return function_t(dlsym(LibraryHandleMap.at(LibraryName), SymbolName.c_str()));
    #endif
}