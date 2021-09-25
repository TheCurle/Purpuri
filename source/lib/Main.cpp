/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <native/Native.hpp>

extern "C" void Java_Native_doStuff_V_I_() {
    Variable Fifteen;
    Fifteen.pointerVal = 15;
    VM::Return(Fifteen);
}