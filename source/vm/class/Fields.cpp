/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Common.hpp>
#include <vm/Class.hpp>
#include <vm/Stack.hpp>

/**
 * This file implements all of the functions of the Engine class declared in Class.hpp related to fields.
 *
 * This includes:
 *  - Reading and writing static fields
 *  - Reading and writing instance fields
 *  - Finding fields
 *
 * Note that some of these functions are also from the Class class, declared in Common.hpp.
 * They live here to unify the purpose of this file.
 * Class functions are all at the bottom of the file.
 *
 * A lot of time is spent in these functions in the sake of debugging and double checking.
 * A future improvement would be to move most of this extra information into LOUD blocks so that they only
 *  execute if Quiet Mode is disabled.
 */


/**
 * Write a value to a static field.
 *
 * The field index is stored in the next byte of the program code.
 * It indexes a Field constant in the Class' Constants Pool.
 * The field may not exist in the current class, so it must also be resolved here.
 *
 * Most of this is a simple wrapper around Class#PutStatic.
 *
 * @param Stack the Stack Frame for the method currently being executed.
 */
void Engine::PutStatic(StackFrame* Stack) const {
    // First, figure out which field we need to write to
    auto ConstantIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    // Now we can get the field's name and descriptor.
    Class* SearchClass = Stack->_Class;
    std::string FieldName = SearchClass->GetStringConstant(ConstantIndex);
    
    // The field's owning class takes some searching.
    // This is because a class' fields can be in the super class, or any implemented interface.
    // Thankfully, the Java language tells us which class the field actually belongs to - it's part of the Field data
    //  that the Constants Pool index points to.

    // First, read the constant pool for the class' name index.
    char* Data = (char*) SearchClass->Constants[ConstantIndex];
    auto classInd = ReadShortFromStream(Data + 1);
    
    // Read the constant pool for the class' name string
    Data = (char*) SearchClass->Constants[classInd];
    auto val = ReadShortFromStream(Data + 1);
    // Save it as a std::string
    std::string ClassName = SearchClass->GetStringConstant(val);

    // Now, this class is the class (or interface) where the field actually lives.
    SearchClass = _ClassHeap->GetClass(ClassName);

    // We can finally get the field location..
    uint32_t FieldIndex = SearchClass->GetFieldFromDescriptor(FieldName);
    printf("Field %d resolved to index %d.\n", ConstantIndex, FieldIndex);

    // Fetch the data for what was last pushed to the stack..
    Variable Value = Stack->Stack[Stack->StackPointer];

    // And save that data to the class.
    if(!SearchClass->PutStatic(FieldIndex, Value))
        printf("Setting static field failed!\n");
}

/**
 * Read a value from a static field.
 *
 * The field index is stored in the next byte of the program code.
 * It indexes a Field constant in the Class' Constants Pool.
 * The field may not exist in the current class, so it must also be resolved here.
 *
 * Most of this is a simple wrapper around Class#GetStatic.
 *
 * @param Stack the Stack Frame for the method currently being executed.
 */
void Engine::GetStatic(StackFrame* Stack) {
    // First, figure out which field we need to write to
    auto ConstantIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    // Now we can get the field's name and descriptor.
    Class* SearchClass = Stack->_Class;
    std::string FieldName = SearchClass->GetStringConstant(ConstantIndex);

    // The field's owning class takes some searching.
    // This is because a class' fields can be in the super class, or any implemented interface.
    // Thankfully, the Java language tells us which class the field actually belongs to - it's part of the Field data
    //  that the Constants Pool index points to.

    // First, read the constant pool for the class' name index.
    char* Data = (char*) SearchClass->Constants[ConstantIndex];
    auto classInd = ReadShortFromStream(Data + 1);

    // Read the constant pool for the class' name string
    Data = (char*) SearchClass->Constants[classInd];
    auto val = ReadShortFromStream(Data + 1);
    // Save it as a std::string
    std::string ClassName = SearchClass->GetStringConstant(val);

    // Now, this class is the class (or interface) where the field actually lives.
    SearchClass = _ClassHeap->GetClass(ClassName);

    // We can finally get the field location..
    uint32_t FieldIndex = SearchClass->GetFieldFromDescriptor(FieldName);
    printf("Field %d resolved to index %d.\n", ConstantIndex, FieldIndex);

    // Retrieve the data in the field...
    Variable Value = SearchClass->GetStatic(FieldIndex);

    // And push it to the stack.
    Stack->Stack[Stack->StackPointer + 1] = Value;

    // Note that we don't increase the stack pointer yet - this isn't a push, just a write.
    // It's up to the bytecode what to do now.
}

/**
 * Write a value to an instanced field.
 *
 * The value to write is the last thing pushed to the stack.
 * The Object Reference to store the data in, was pushed before that.
 * The Field Index is the next byte in the code.
 *
 * @param Stack the Stack Frame for the method currently being executed.
 */
void Engine::PutField(StackFrame* Stack) {
    // First, read the index of the Field Name + Descriptor from the bytecode.
    auto FieldNameIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    // Read the Object and Value from the stack
    Variable Obj = Stack->Stack[Stack->StackPointer - 1];
    Variable ValueToSet = Stack->Stack[Stack->StackPointer];

    // We have the Object Reference, now we need to get the field data as an array.
    Variable* VarList = _ObjectHeap.GetObjectPtr(Obj.object);

    // Having the required data, now we can fetch the class data that the Object Reference refers to...
    auto* FieldsClass = (Class*)VarList->pointerVal;

    // Read the full name of the Field in its' owning class..
    std::string FieldName = FieldsClass->GetStringConstant(FieldNameIndex);

    // Fetch the index of the field into that class (the index into VarList from the class data)
    uint32_t FieldIndex = FieldsClass->GetFieldFromDescriptor(FieldName);

    printf("Setting field %s to " PrtSizeT ".\r\n", FieldName.c_str(), ValueToSet.pointerVal);

    // And store that data (+ 1 for the class referred to at the start) in the VarList.
    VarList[FieldIndex + 1] = ValueToSet;

    // Note that we don't increase the stack pointer yet - this isn't a push, just a write.
    // It's up to the bytecode what to do now.
}

/**
 * Read a value from an instanced field.
 *
 * The Object Reference to read the data from is the last thing pushed to the stack.
 * The Field Index is the next byte in the code.
 *
 * @param Stack the Stack Frame for the method currently being executed.
 */
void Engine::GetField(StackFrame* Stack) {
    // First, read the index of the Field Name + Descriptor from the bytecode.
    auto FieldNameIndex = ReadShortFromStream(&Stack->_Method->Code->Code[Stack->ProgramCounter + 1]);

    // Read the Object and Value from the stack
    Variable Obj = Stack->Stack[Stack->StackPointer];

    // We have the Object Reference, now we need to get the field data as an array.
    Variable* VarList = _ObjectHeap.GetObjectPtr(Obj.object);

    // Having the required data, now we can fetch the class data that the Object Reference refers to...
    auto* FieldsClass = (Class*)VarList->pointerVal;

    // Read the full name of the Field in its' owning class..
    std::string FieldName = FieldsClass->GetStringConstant(FieldNameIndex);
    printf("Reading field data - field is %s, class is %s.\n", FieldName.c_str(), FieldsClass->GetClassName().c_str());

    // Fetch the index of the field into that class (the index into VarList from the class data)
    uint32_t FieldIndex = FieldsClass->GetFieldFromDescriptor(FieldName);

    size_t ClassSize = FieldsClass->GetClassSize(), Fields = FieldsClass->GetClassFieldCount();
    printf("\tClass has " PrtSizeT " entries and " PrtSizeT " field(s), we want to read the %dth.\n", ClassSize, Fields, FieldNameIndex);

    // And save the data into the stack, overwriting the current value with our modified array.
    Stack->Stack[Stack->StackPointer] = VarList[FieldIndex + 1];
    printf("Reading value " PrtSizeT " from field\n", Stack->Stack[Stack->StackPointer].pointerVal);

    // Note that we don't touch the stack pointer - we effectively popped the old value and pushed a new one, so it's
    // safe to just overwrite in-place.
}

/**
 * A simple wrapper to search a class for a field with the matching name and descriptor.
 *
 * Attempting to call this method with a field name that is not in this class is invalid behavior.
 * The program will immediately close upon detecting this.
 *
 * TODO: make it not do that?
 *
 * @param FieldAndDescriptor the field information in the form of Name(Parameters)Return - name and descriptor.
 * @return the index of the field in the given class.
 */
uint32_t Class::GetFieldFromDescriptor(const std::string& FieldAndDescriptor) {
    for(uint32_t i = 0; i < FieldsCount; i++) {
        std::string FieldName = GetStringConstant(Fields[i]->Name);
        std::string FieldDesc = GetStringConstant(Fields[i]->Descriptor);
        std::string NameDesc = FieldName.append(FieldDesc);

        if(NameDesc == FieldAndDescriptor)
            return i;
    }

    printf("Unable to find field %s in class %s. Fatal error.\n", FieldAndDescriptor.c_str(), GetClassName().c_str());
    exit(7);
}

/**
 * A simple wrapper to place a value into a static field of this class.
 *
 * @param Field the index of the Field into the fields array of this class
 * @param Value the value to store into the field
 * @return true if the value was stored successfully, false otherwise
 */
bool Class::PutStatic(uint16_t Field, Variable Value) {
    std::string Name = GetClassName();
    if(!_ClassHeap->ClassExists(Name))
        return false;

    printf("Setting static field %s::%s (index %d) to " PrtSizeT ".\n", Name.c_str(), GetStringConstant(Fields[Field]->Name).c_str(), Field, Value.pointerVal);
    ClassStatics[Field] = Value;

    return true;
}

/**
 * A simple wrapper to fetch a value from a static field of this class.
 *
 * @param Field the index of the Field into the fields array of this class
 * @return the value stored in the Field
 */
Variable Class::GetStatic(uint16_t Field) {
    std::string Name = GetClassName();
    if(!_ClassHeap->ClassExists(Name))
        return Variable(ObjectHeap::Null);

    printf("Retrieving value from field %s::%s\n", Name.c_str(), GetStringConstant(Fields[Field]->Name).c_str());

    return ClassStatics[Field];
}
