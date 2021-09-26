/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Common.hpp>
#include <vm/Class.hpp>
#include <vm/Stack.hpp>

void Engine::PutField(StackFrame* Stack) {
    printf("Value %zu, object %zu, field %zu\n", Stack->Stack[Stack->StackPointer].pointerVal, Stack->Stack[Stack->StackPointer - 1].pointerVal, 1);

    uint16_t SearchIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    Variable Obj = Stack->Stack[Stack->StackPointer - 1];
    Variable ValueToSet = Stack->Stack[Stack->StackPointer];

    Variable* VarList = _ObjectHeap.GetObjectPtr(Obj.object);
    
    Class* FieldsClass = (Class*)VarList->pointerVal;

    std::string FieldName = FieldsClass->GetStringConstant(SearchIndex);

    uint32_t FieldIndex = FieldsClass->GetFieldFromDescriptor(FieldName);

    printf("Setting field %s to %zu.\r\n", FieldName.c_str(), ValueToSet.pointerVal);

    VarList[FieldIndex + 1] = ValueToSet;
}

void Engine::GetField(StackFrame* Stack) {
    uint16_t SearchIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);
    printf("Reading field %d.\n", SearchIndex);
    Variable Obj = Stack->Stack[Stack->StackPointer];
    printf("\tFound object %zu.\r\n", Obj.pointerVal);
    Variable* VarList = _ObjectHeap.GetObjectPtr(Obj.object);
    Class* FieldsClass = (Class*)VarList->pointerVal;

    printf("\tFound class %s from list at 0x%zx.\r\n", FieldsClass->GetClassName().c_str(), (size_t) VarList);

    std::string FieldName = FieldsClass->GetStringConstant(SearchIndex);

    size_t ClassSize = FieldsClass->GetClassSize(), Fields = FieldsClass->GetClassFieldCount();
    printf("\tClass has %zu entries and %zu field(s), we want to read the %dth.\n", ClassSize, Fields, SearchIndex);

    uint32_t FieldIndex = FieldsClass->GetFieldFromDescriptor(FieldName);

	Stack->Stack[Stack->StackPointer] = VarList[FieldIndex + 1];
    printf("Reading value %zu from field %s of class %s\r\n", Stack->Stack[Stack->StackPointer].pointerVal, FieldName.c_str(), FieldsClass->GetClassName().c_str());
}

uint32_t Class::GetFieldFromDescriptor(std::string FieldAndDescriptor) {
    for(uint32_t i = 0; i < FieldsCount; i++) {
        std::string FieldName = GetStringConstant(Fields[i]->Name);
        std::string FieldDesc = GetStringConstant(Fields[i]->Descriptor);
        std::string NameDesc = FieldName.append(FieldDesc);

        if(NameDesc.compare(FieldAndDescriptor) == COMPARE_MATCH)
            return i;
    }

    printf("Unable to find field %s in class %s. Fatal error.\n", FieldAndDescriptor.c_str(), GetClassName().c_str());
    exit(7);
    return 0;
}
