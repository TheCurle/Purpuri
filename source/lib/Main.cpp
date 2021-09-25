/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/
#include <iostream>
#include <native/Native.hpp>

extern "C" void Java_Native_doStuff_V_I_() {
    Variable Fifteen;
    Fifteen.pointerVal = 15;
    VM::Return(Fifteen);
}

void DumpVariables(std::vector<Variable> list) {
    for(auto item : list) {
        std::cout << item.pointerVal << " (heap object " << item.object.Heap << ")" << std::endl;
    }
}

extern "C" void Java_java_vm_Printer_println_Ljava_lang_StringE_V_() {
    auto Params = VM::GetParameters();
    printf("println dump:\n");
    DumpVariables(Params);
    printf("println dump over.\n");

    Variable Ten;
    Ten.pointerVal = 10;
    VM::Return(Ten);
}