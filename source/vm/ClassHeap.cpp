/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Class.hpp>

#include <fstream>
#include <iterator>
#include <string>
#include <list>
#include <algorithm>

ClassHeap::ClassHeap() {}

bool ClassHeap::AddClass(Class *Class) {
    if(!Class) return false;
    std::string Name = Class->GetClassName();

    ClassMap.emplace(Name, Class);
    return true;
}

bool ClassHeap::ClassExists(std::string Name) {
    return std::find(ClassCache.begin(), ClassCache.end(), Name) != ClassCache.end();
}

Class* ClassHeap::GetClass(std::string Name) {
    if (Name.empty()) return nullptr;

    std::map<std::string, Class*>::iterator classIter;

    classIter = ClassMap.find(Name);

    if(classIter == ClassMap.end() && Name != UnknownClass) {
        printf("*************\nClass %s does not exist!\n************\n", Name.c_str());
        return NULL;
    }
        
    Class* Class = classIter->second;

    std::ifstream File(Name.c_str(), std::ios::binary);

    File.close();
    return Class;
}

bool ClassHeap::LoadClass(const char *ClassName, Class *Class) {
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
    ClassCache.emplace_back(ClassName);

    if(!Class->LoadFromFile(RelativePath))
        return false;

    printf("Loading class %s at 0x%p.\r\n", ClassName, (void*)(size_t*)Class);
    return AddClass(Class);
}
