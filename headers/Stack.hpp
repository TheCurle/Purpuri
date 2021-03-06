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
        Class* _Class;
        Method* _Method;
        uint32_t ProgramCounter;
        int16_t StackPointer;
        Variable* Stack;

        StackFrame() {
            StackPointer = -1;
            ProgramCounter = 0;
            _Class = nullptr;
            Stack = nullptr;
            FrameBase = nullptr;
            MemberStack = nullptr;
        }

        StackFrame(int16_t StackPointer) {
            this->StackPointer = StackPointer;
            _Class = nullptr;
            Stack = nullptr;
            ProgramCounter = 0;
        }
};