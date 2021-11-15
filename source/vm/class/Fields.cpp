/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Common.hpp>
#include <vm/Class.hpp>
#include <vm/Stack.hpp>

void Engine::PutStatic(StackFrame* Stack) {
    uint16_t ConstantIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    Class* SearchClass = Stack->_Class;
    std::string FieldName = SearchClass->GetStringConstant(ConstantIndex);
    
    // The field's owning class takes some searching.
    // First read the constant pool for the class' name index.
    char* Data = (char*) SearchClass->Constants[ConstantIndex];
    uint16_t classInd = ReadShortFromStream(Data + 1);
    
    // Read the constant pool for the class' name string
    Data = (char*) SearchClass->Constants[classInd];
    uint16_t val = ReadShortFromStream(Data + 1);
    // Save it as a std::string
    std::string ClassName = SearchClass->GetStringConstant(val);

    SearchClass = _ClassHeap->GetClass(ClassName);

    uint32_t FieldIndex = SearchClass->GetFieldFromDescriptor(FieldName);
    printf("Field %d resolved to index %d.\n", ConstantIndex, FieldIndex);

    Variable Value = Stack->Stack[Stack->StackPointer];

    if(!SearchClass->PutStatic(FieldIndex, Value))
        printf("Setting static field failed!\n");
}

void Engine::GetStatic(StackFrame* Stack) {
    uint16_t ConstantIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    Class* SearchClass = Stack->_Class;
    std::string FieldName = SearchClass->GetStringConstant(ConstantIndex);
    
    // The field's owning class takes some searching.
    // First read the constant pool for the class' name index.
    char* Data = (char*) SearchClass->Constants[ConstantIndex];
    uint16_t classInd = ReadShortFromStream(Data + 1);
    
    // Read the constant pool for the class' name string
    Data = (char*) SearchClass->Constants[classInd];
    uint16_t val = ReadShortFromStream(Data + 1);
    // Save it as a std::string
    std::string ClassName = SearchClass->GetStringConstant(val);

    SearchClass = _ClassHeap->GetClass(ClassName);

    uint32_t FieldIndex = SearchClass->GetFieldFromDescriptor(FieldName);

    Variable Value = SearchClass->GetStatic(FieldIndex);

    Stack->Stack[Stack->StackPointer + 1] = Value;
}

void Engine::PutField(StackFrame* Stack) {
    printf("Value " PrtSizeT ", object " PrtSizeT ", field " PrtSizeT "\n", Stack->Stack[Stack->StackPointer].pointerVal, Stack->Stack[Stack->StackPointer - 1].pointerVal, (size_t) 1);

    uint16_t SearchIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    Variable Obj = Stack->Stack[Stack->StackPointer - 1];
    Variable ValueToSet = Stack->Stack[Stack->StackPointer];

    Variable* VarList = _ObjectHeap.GetObjectPtr(Obj.object);
    
    Class* FieldsClass = (Class*)VarList->pointerVal;

    std::string FieldName = FieldsClass->GetStringConstant(SearchIndex);

    uint32_t FieldIndex = FieldsClass->GetFieldFromDescriptor(FieldName);

    printf("Setting field %s to " PrtSizeT ".\r\n", FieldName.c_str(), ValueToSet.pointerVal);

    VarList[FieldIndex + 1] = ValueToSet;
}

void Engine::GetField(StackFrame* Stack) {
    uint16_t SearchIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);
    printf("Reading field %d.\n", SearchIndex);
    Variable Obj = Stack->Stack[Stack->StackPointer];
    printf("\tFound object " PrtSizeT ".\r\n", Obj.pointerVal);
    Variable* VarList = _ObjectHeap.GetObjectPtr(Obj.object);
    Class* FieldsClass = (Class*)VarList->pointerVal;

    printf("\tFound class %s from list at 0x" PrtHex64 ".\r\n", FieldsClass->GetClassName().c_str(), (size_t) VarList);

    std::string FieldName = FieldsClass->GetStringConstant(SearchIndex);

    size_t ClassSize = FieldsClass->GetClassSize(), Fields = FieldsClass->GetClassFieldCount();
    printf("\tClass has " PrtSizeT " entries and " PrtSizeT " field(s), we want to read the %dth.\n", ClassSize, Fields, SearchIndex);

    uint32_t FieldIndex = FieldsClass->GetFieldFromDescriptor(FieldName);

	Stack->Stack[Stack->StackPointer] = VarList[FieldIndex + 1];
    printf("Reading value " PrtSizeT " from field %s of class %s\r\n", Stack->Stack[Stack->StackPointer].pointerVal, FieldName.c_str(), FieldsClass->GetClassName().c_str());
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


bool Class::PutStatic(uint16_t Field, Variable Value) {
    std::string Name = GetClassName();
    if(!_ClassHeap->ClassExists(Name))
        return false;

    printf("Setting static field %s::%s (index %d) to " PrtSizeT ".\n", Name.c_str(), GetStringConstant(Fields[Field]->Name).c_str(), Field, Value.pointerVal);
    ClassStatics[Field] = Value;

    return true;
}

Variable Class::GetStatic(uint16_t Field) {
    std::string Name = GetClassName();
    if(!_ClassHeap->ClassExists(Name))
        return false;

    printf("Retrieving value from field %s::%s\n", Name.c_str(), GetStringConstant(Fields[Field]->Name).c_str());

    return ClassStatics[Field];
}
