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

    auto String = Params.at(0).object;
    printf("String parameter is object %zu on heap.\n", String.Heap);
    auto ClassObj = VM::GetObject(String);
    printf("String object is at 0x%zx\n", ClassObj);
    printf("println given a String: at 0x%zx\n", ClassObj[0].pointerVal);

    Variable Ten;
    Ten.pointerVal = 10;
    VM::Return(Ten);
}