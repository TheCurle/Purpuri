/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Native.hpp>
#ifdef WIN32
    #include <windows.h>
    #include <libloaderapi.h>
    #undef GetClassName
    #undef LoadLibrary
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

std::string Native::EncodeName(NativeContext Context) {
    std::string MangledName("Java_");
    // Class name with every . replaced by _
    std::string ClassName(Context.ClassName);
    std::regex_replace(ClassName, std::regex("/"), "_");
    MangledName.append(ClassName).append("_");

    // Function name
    MangledName.append(Context.MethodName).append("_");

    // Get the middle portion of the descriptor
    std::string Descriptor(Context.MethodDescriptor);
    std::string Parameters = Descriptor.substr(Descriptor.find_first_of("(") + 1, Descriptor.find_first_of(")") - 1);
    
    // Replace / with _ (Lnet_company_Class;)
    // Replace ; with E (Lnet_company_ClassE)
    std::regex_replace(Parameters, std::regex("/"), "_");
    std::regex_replace(Parameters, std::regex(";"), "E");
    if(Parameters.size() == 0) Parameters = "V";
    MangledName.append(Parameters).append("_");

    std::string ReturnType = Descriptor.substr(Descriptor.find_first_of(")") + 1, Descriptor.size());
    std::regex_replace(ReturnType, std::regex("/"), "_");
    std::regex_replace(ReturnType, std::regex(";"), "E");
    MangledName.append(ReturnType).append("_");

    // Java_uk_gemwire_purpuri_run_IIZ_Z_

    return MangledName;
}

void* Native::LoadLibrary(std::string LibraryName) {
    #ifdef WIN32
        void* Handle = LoadLibraryA(LibraryName.c_str());
        LibraryHandleMap.emplace(LibraryName, Handle);
        return Handle;
    #endif
}

function_t Native::LoadSymbol(std::string LibraryName, std::string SymbolName) {
    #ifdef WIN32
        return GetProcAddress((HMODULE) LibraryHandleMap.at(LibraryName), SymbolName.c_str());
    #endif
}