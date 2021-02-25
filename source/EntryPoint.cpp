/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <cstdio>
#include <cstring>
#include "../headers/Stack.hpp"

int main(int argc, char* argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Purpuri VM v1 - Gemwire Institute\n");
        fprintf(stderr, "***************************************\n");
        fprintf(stderr, "Usage: %s <class file>\n", argv[0]);
        fprintf(stderr, "\n16:50 25/02/21 Curle\n");
        return 1;
    }

    ClassHeap heap;
    Class* Object = new Class();
    Class* GivenClass = new Class();

    heap.LoadClass((char*)"java/lang/Object", Object);
    heap.LoadClass(argv[1], GivenClass);

    ObjectHeap objects;

    StackFrame* Stack = new StackFrame[20];
    StackFrame::FrameBase = Stack;
    memset(Stack, 0, sizeof(StackFrame) * 20);

    StackFrame::MemberStack = new Variable[100];
    memset(StackFrame::MemberStack, 0, sizeof(Variable) * 100);

    Engine Engine;

    Engine.ClassHeap = &heap;
    Engine.ObjectHeap = &objects;
    int StartFrame = 0;

    class Object object = objects.CreateObject(GivenClass);
    Class* VirtualClass = GivenClass;

    int EntryPoint = GivenClass->GetMethodFromDescriptor((char*) "EntryPoint", (char*) "()I", GivenClass);

    Stack[StartFrame].Class = GivenClass;
    Stack[StartFrame].Method = &GivenClass->Methods[EntryPoint];
    Stack[StartFrame].Stack = StackFrame::MemberStack;
    Stack[StartFrame].StackPointer = Stack[StartFrame].Method->Code->LocalsSize;
    Stack[StartFrame].Stack[0].object = object;

    Engine.Ignite(&Stack[StartFrame]);

    return 1;
}