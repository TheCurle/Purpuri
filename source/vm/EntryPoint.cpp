/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <cstdio>
#include <cstring>
#include <vm/Stack.hpp>

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
    GivenClass->SetClassHeap(&heap);
    Object->SetClassHeap(&heap);

    std::string GivenPath(argv[1]);
    size_t LastInd = GivenPath.find_last_of("/");
    if(LastInd != GivenPath.size())
        heap.ClassPrefix = GivenPath.substr(0, LastInd + 1);

    if(!heap.LoadClass((char*)"java/lang/Object", Object)) {
        printf("Unable to load Object class. Fatal error.\n");
        exit(6);
    }

    if(!heap.LoadClass(argv[1], GivenClass)) {
        printf("Loading given class failed. Fatal error.\n");
        exit(6);
    }

    ObjectHeap objects;

    StackFrame* Stack = new StackFrame[20];
    StackFrame::FrameBase = Stack;

    for(size_t i = 0; i < 20; i++) {
        Stack[i] = StackFrame();
    }

    StackFrame::MemberStack = new Variable[100];
    memset(StackFrame::MemberStack, 0, sizeof(Variable) * 100);

    Engine engine;

    class Object object = objects.CreateObject(GivenClass);
    //Class* VirtualClass = GivenClass;

    int EntryPoint = GivenClass->GetMethodFromDescriptor("EntryPoint", "()I", GivenClass->GetClassName().c_str(), GivenClass);

    if(EntryPoint < 0) {
        printf("%s does not have an EntryPoint function, unable to execute.\n", argv[1]);
        return 1;
    }

    int StartFrame = 0;

    Stack[StartFrame]._Class = GivenClass;
    Stack[StartFrame]._Method = &GivenClass->Methods[EntryPoint];
    Stack[StartFrame].Stack = StackFrame::MemberStack;
    Stack[StartFrame].StackPointer = Stack[StartFrame]._Method->Code->LocalsSize;
    Stack[StartFrame].Stack[0].object = object;

    puts("*****************");
    puts("\n\nStarting Execution\n\n");
    puts("*****************");


    engine._ClassHeap = &heap;
    engine._ObjectHeap = objects;

    engine.Ignite(&Stack[StartFrame]);

    return 1;
}