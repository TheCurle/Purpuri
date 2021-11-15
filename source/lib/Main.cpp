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
    data++; // Skip the char marker

    size_t datalen = VM::GetArrayLength(charArray);

    char text[datalen + 1];
    for(size_t i = 0; i < datalen; i++)
        text[i] = data[i].charVal;
    text[datalen] = '\0';

    printf("%s\n", text);
    
    VM::Return(10);
}


extern "C" void Java_java_vm_Printer_printint_I_V_() {
    auto Parameters = VM::GetParameters();

    printf("%d\n", Parameters.at(0).intVal);

    VM::Return(10);
}