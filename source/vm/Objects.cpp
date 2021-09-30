/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Objects.hpp>
#include <vm/Class.hpp>

#include <cstring>

Object ObjectHeap::Null;

ObjectHeap::ObjectHeap() {
    NextObjectID = 1;
    ObjectMap = std::map<size_t, size_t>();
}

ObjectHeap::~ObjectHeap() {}

Object ObjectHeap::CreateObject(Class *Class) {
    Object Empty;
    Empty.Heap = 0;
    Empty.Type = 0;

    if(Class == NULL) return Empty;

    size_t ObjectSize = Class->GetClassFieldCount() + 1;
    Variable* ClassObj = new Variable[ObjectSize];

    if(!ClassObj) return Empty;

    Object object;
    object.Heap = NextObjectID++,
    object.Type = 0;

    ClassObj[0].pointerVal = (size_t) Class;

    ObjectMap.emplace((size_t) object.Heap, (size_t) ClassObj);

    return object;
}

Variable* ObjectHeap::GetObjectPtr(Object obj) {
    //size_t Obj = 0;

    std::map<size_t, size_t>::iterator objIter;
    objIter = ObjectMap.find(obj.Heap);

    if(objIter == ObjectMap.end()) {
        printf("**************\nObject Heap does not contain object %zu. Highest object is %zu.\n******************\n", obj.Heap, ObjectMap.size());
        for(;;);
        return NULL;
    }

    return (Variable*) objIter->second;
}

Object ObjectHeap::CreateString(std::string String, ClassHeap *ClassHeap) {
    Object obj;
    obj.Heap = 0;
    obj.Type = 0;

    // Retrieve the String class
    if(ClassHeap == NULL) return obj;
    Class* Class = ClassHeap->GetClass("java/lang/String");

    // Create a new String instance
    if(Class == NULL) return obj;
    obj = CreateObject(Class);
    Variable* Var = this->GetObjectPtr(obj);
    if(Var == NULL) return obj;

    // Create a new char array
    Object array = CreateArray(5, String.length());
    Variable* data = (Variable*) ObjectMap.at(array.Heap);
    
    // Copy the string into the array
    const char* str = String.c_str();
    size_t strLen = String.length();
    for(size_t i = 1; i < strLen; i++)
        data[i] = str[i - 1];

    Var[1].object = array;

    return obj;
}

Object ObjectHeap::CreateArray(uint8_t Type, uint32_t Count) {
    Variable* array = new Variable[Count + 1];
    Object object;
    object.Heap = 0;
    object.Type = Type;

    if(array) {
        memset(array, 0, sizeof(Variable) * (Count + 1));
        object.Heap = NextObjectID++;
        array[0].intVal = Type;

        ObjectMap.emplace((size_t) object.Heap, (size_t) array);
        ArraySizeMap.emplace((size_t) object.Heap, (size_t) Count);
    }

    return object;
}

bool ObjectHeap::CreateObjectArray(Class* pClass, uint32_t Count, Object& pObject) {
    Variable* array = new Variable[Count];

    if(array) {
        memset(array, 0, sizeof(Variable) * (Count));
        array[0].pointerVal = (size_t) pClass;
    } else {
        return false;
    }

    pObject.Heap = NextObjectID++;

    ObjectMap.emplace((size_t) pObject.Heap, (size_t) array);
    ArraySizeMap.emplace((size_t) pObject.Heap, (size_t) Count);
    return true;
}

size_t ObjectHeap::GetArraySize(Object obj) {
    std::map<size_t, size_t>::iterator objIter;
    objIter = ArraySizeMap.find(obj.Heap);

    if(objIter == ArraySizeMap.end())
        return 0;

    return objIter->second;
}