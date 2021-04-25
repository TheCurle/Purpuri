/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Objects.hpp"
#include "../headers/Class.hpp"

#include <cstring>

ObjectHeap::ObjectHeap() {
    NextObjectID = 1;
    ObjectMap = std::map<size_t, size_t>();
}

ObjectHeap::~ObjectHeap() {}

Object ObjectHeap::CreateObject(Class *Class) {
    Object Empty = (Object) {0, 0};

    if(Class == NULL) return Empty;

    size_t ObjectSize = Class->GetClassFieldCount() + 1;
    Variable* ClassObj = new Variable[ObjectSize];
    
    if(!ClassObj) return Empty;

    Object object = (Object) {
        .Heap = NextObjectID++,
        .Type = 0
    };

    ClassObj[0].pointerVal = (size_t) Class;
    
    ObjectMap.emplace((size_t) object.Heap, (size_t) ClassObj);

    return object;
}

Variable* ObjectHeap::GetObjectPtr(Object obj) {
    //size_t Obj = 0;

    std::map<size_t, size_t>::iterator objIter;
    objIter = ObjectMap.find(obj.Heap);

    if(objIter == ObjectMap.end())
        return NULL;

    return (Variable*) objIter->second;
}

Object ObjectHeap::CreateString(std::string String, ClassHeap *ClassHeap) {
    Object obj;
    obj.Heap = 0;
    obj.Type = 0;

    if(ClassHeap == NULL) return obj;
    Class* Class = ClassHeap->GetClass("java/lang/String");

    if(Class == NULL) return obj;
    obj = CreateObject(Class);

    Variable* Var = this->GetObjectPtr(obj);

    if(Var == NULL) return obj;

    // Java dictates we need a pointer to this str.
    const char* Str = String.c_str();
    Var[1].pointerVal = (size_t)(&Str);

    return obj;
}