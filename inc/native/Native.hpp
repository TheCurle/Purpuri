/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>
#include <exception>

#ifdef VM_BUILDING
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __declspec(dllimport)
#endif

#ifndef COMMON_SYMBOLS
class Object {
    public:
        size_t Heap;
        uint8_t Type;
};

union Variable {
    uint8_t charVal;
    uint16_t shortVal;
    uint32_t intVal;
    float floatVal;
    double doubleVal;
    size_t pointerVal;
    Object object;
};

struct NativeContext {
    int InvocationMethod;

    std::string ClassName;
    std::string MethodName;
    std::string MethodDescriptor;

    std::vector<Variable> Parameters;
    Object* ClassInstance;
};
#endif

struct NativeReturn : public std::exception {
    public:

    Variable Value;
    const char* what() const throw() {
        return "Native Function Returned";
    }
};


/**
 * The primary way for a Natives application to interact with the Purpuri VM.
 * All of these methods are static - and thus can be called from anywhere.
 * Context is generated on-the-fly, rather than gathered beforehad.
 * 
 * This constitutes a substantial performance improvement over the JNI.
 */

class EXPORT VM {
    public:
        /**
         * Emit a return value and return to the calling function.
         */
        static void Return(Variable var);

        /**
         * Get the parameters that were passed by the function call, if there were any.
         * The first entry (number #0) is always the class instance that the method was called on (unless it was static.)
         * With this, you can access the fields of the class.
         */
        static std::vector<Variable> GetParameters();
};
