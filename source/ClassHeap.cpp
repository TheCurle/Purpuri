/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Class.hpp"
#include <iterator>

ClassHeap::ClassHeap() {}

bool ClassHeap::AddClass(Class *Class) {
    if(!Class) return false;
    char* Name = Class->GetClassName();

    ClassMap.emplace(Name, Class);
    return true;
}

Class* ClassHeap::GetClass(char *Name) {

    std::map<char*, Class*>::iterator classIter;
    classIter = ClassMap.find(Name);

    if(classIter == ClassMap.end())
        return NULL;
    
    Class* Class = new class Class();
    bool Res = this->LoadClass(Name, Class);

    if(!Res) {
        delete Class;
        Class = NULL;
    }

    return Class;
}

bool ClassHeap::LoadClass(char *ClassName, Class *Class) {
    const char* Path, *RelativePath;
    if(!Class) return false;

    std::string pathTemp(ClassName);
    pathTemp.append(".class");
    RelativePath = pathTemp.c_str();

    if(!Class->LoadFromFile(RelativePath))
        return false;

    Class->SetClassHeap(this);

    return AddClass(Class);
}
