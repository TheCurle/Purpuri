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

    std::string NameStr(Name);
    ClassMap.emplace(NameStr, Class);
    return true;
}

Class* ClassHeap::GetClass(char *Name) {

    std::map<std::string, Class*>::iterator classIter;
    std::string NameStr(Name);
    classIter = ClassMap.find(NameStr);

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

    std::string pathTemp("");
    std::string classTemp(ClassName);
    if(classTemp.find("java/") != 0 && classTemp.find(ClassPrefix) != 0)
        pathTemp.append(ClassPrefix);
    pathTemp.append(ClassName);
    pathTemp.append(".class");
    
    RelativePath = pathTemp.c_str();

    if(!Class->LoadFromFile(RelativePath))
        return false;

    Class->SetClassHeap(this);

    return AddClass(Class);
}
