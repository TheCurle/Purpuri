/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#pragma once
#include <vector>
#include <string>
#include <map>
#include <native/Native.hpp>

typedef long long (*function_t)();

class Native {
    public:

    /**
     * Given a class name and:
     *  a method name in the form of:
     *   method(params)return
     *  or a field name in the form of:
     *   field|type,
     * convert it into the corresponding PNI form.
     * 
     * PNI form is more thoroughly documented in the according file on Purpuri Docs.
     * 
     */
    static std::string EncodeName(NativeContext Context);

    /**
     * Given the path to a dynamically-linked library, load it.
     * Returns a handle that can be used to uniquely identify this library.
     * This handle can be used for loading symbols out of this library.
     */
    static void* LoadLibrary(std::string LibraryName);

    /**
     * Given the handle to a Library and the name of a symbol, return a
     * function pointer that can be used to call it.
     */
    static function_t LoadSymbol(std::string LibraryName, std::string SymbolName);

    /**
     * Given the handle to a Library, close it so that no more functions can be
     * queried from it.
     * This also releases filesystem locks to the file.
     */
    static void CloseHandle(std::string LibraryName);

    static std::vector<Variable> Parameters;
    
    private:
    static std::map<std::string, void*> LibraryHandleMap;
};

