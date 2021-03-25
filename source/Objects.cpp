/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Objects.hpp"
#include "../headers/Class.hpp"

#include <cstring>

ObjectHeap::ObjectHeap() {
    NextObjectID = 1;
}

ObjectHeap::~ObjectHeap() {}

Object ObjectHeap::CreateObject(Class *Class) {
    Object object;
    object.Heap = 0;
    object.Type = 0;

    if(Class == NULL) return object;

    size_t ObjectSize = Class->GetClassFieldCount() + 1;
    Variable* ClassObj = new Variable[ObjectSize];
    if(!ClassObj) return object;

    memset(ClassObj, 0, sizeof(Variable) * ObjectSize);

    object.Heap = NextObjectID++;

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

Object ObjectHeap::CreateString(char **String, ClassHeap *ClassHeap) {
    Object obj;
    obj.Heap = 0;
    obj.Type = 0;

    if(ClassHeap == NULL) return obj;
    Class* Class = ClassHeap->GetClass((char*)"java/lang/String");

    if(Class == NULL) return obj;
    obj = CreateObject(Class);

    Variable* Var = this->GetObjectPtr(obj);

    if(Var == NULL) return obj;

    Var[1].pointerVal = (size_t)String;

    return obj;
}