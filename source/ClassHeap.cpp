/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include "../headers/Class.hpp"
#include <fstream>
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
    
    Class* Class = classIter->second;

    std::ifstream File(Name, std::ios::binary);

    File.close();
    return Class;
}

bool ClassHeap::LoadClass(char *ClassName, Class *Class) {
    const char* RelativePath;
    if(!Class) return false;

    std::string pathTemp("");
    std::string classTemp(ClassName);
    if(classTemp.find("java/") != 0 && classTemp.find(ClassPrefix) != 0)
        pathTemp.append(ClassPrefix);
    pathTemp.append(ClassName);
    pathTemp.append(".class");
    
    RelativePath = pathTemp.c_str();

    Class->SetClassHeap(this);
    if(!Class->LoadFromFile(RelativePath))
        return false;

    printf("Loading class %s at 0x%p.\r\n", ClassName, (void*)(size_t*)Class);
    return AddClass(Class);
}
