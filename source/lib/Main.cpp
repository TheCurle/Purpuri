/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/
#include <iostream>
#include <native/Native.hpp>

extern "C" void Java_Native_doStuff_V_I_() {
    VM::Return(15);
}

void DumpVariables(std::vector<Variable> list) {
    for(auto item : list) {
        std::cout << item.pointerVal << " (heap object " << item.object.Heap << ")" << std::endl;
    }
}

extern "C" void Java_java_vm_Printer_println_Ljava_lang_StringE_V_() {
    auto Params = VM::GetParameters();

    auto String = Params.at(0).object;
    Variable* ClassObj = VM::GetObject(String);

    Object charArray = ClassObj[1].object;
    Variable* data = VM::GetObject(charArray);

    printf("%s\n", data + 1);
    
    VM::Return(10);
}