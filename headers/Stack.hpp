/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "Common.hpp"
#include "Class.hpp"

class StackFrame {
    public: 
        static Variable* MemberStack;
        static StackFrame* FrameBase;
        Class* Class;
        Method* Method;
        uint32_t ProgramCounter;
        int16_t StackPointer;
        Variable* Stack;

        StackFrame() {
            StackPointer = -1;
            ProgramCounter = 0;
            Class = NULL;
            Stack = NULL;
            FrameBase = NULL;
            MemberStack = NULL;
        }

        StackFrame(int16_t StackPointer) {
            this->StackPointer = StackPointer;
            Class = NULL;
            Stack = NULL;
            ProgramCounter = 0;
        }
};