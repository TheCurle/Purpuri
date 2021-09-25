/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Native.hpp>
#include <regex>

std::vector<Variable> NativeParameters;

void VM::Return(Variable retVal) {
    NativeReturn Return;
    Return.Value = retVal;

    throw Return;
}

std::vector<Variable> VM::GetParameters() {
    return NativeParameters;
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