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

/**
 * This file implements the ClassHeap class declared in Class.hpp.
 *
 * It is what handles class management.
 * It does this with two primary data structures; the class cache and the class map.
 *
 * The Class Cache keeps a list of all class names that are currently loaded into the VM.
 * It may be used as a (very) quick reference to check whether a specific class is available.
 *
 * The Class Map is what allows you to access a specific class by name.
 * It throws out_of_range if you attempt to access a class that does not exist, so check with the Class Cache first.
 * They are always synchronized.
 *
 * The implementation of this class is mostly wrappers around searching through the Cache and Map, with some
 *  added handling for class prefixes.
 * For an explanation on class prefixes, see EntryPoint.cpp.
 *
 * @author Curle
 */

ClassHeap::ClassHeap() = default;

/**
 * A simple wrapper to add a Class to the heap.
 * It is intended to be called only from LoadClass, so it does not touch the Cache.
 * @param Class the Class to add.
 * @return whether the class was added successfully.
 */
bool ClassHeap::AddClass(Class *Class) {
    if(!Class) return false;

    std::string Name = Class->GetClassName();
    ClassMap.emplace(Name, Class);

    return true;
}

/**
 * A simple wrapper to check whether a class is currently loaded.
 * The Class Cache makes this extremely simple.
 * @param Name the name of the class to check
 * @return whether the class is loaded.
 */
bool ClassHeap::ClassExists(const std::string& Name) {
    return std::find(ClassCache.begin(), ClassCache.end(), Name) != ClassCache.end();
}

/**
 * A simple wrapper to fetch a class from the Class Map.
 * If the name is invalid, or the class is not in the Cache, null is returned.
 * @param Name the Class to retrieve.
 * @return the class, or null if the above conditions are met.
 */
Class* ClassHeap::GetClass(const std::string& Name) {
    if (Name.empty()) return nullptr;
    if (!ClassExists(Name)) return nullptr;

    // Because the ClassExists call passed, and we can guarantee the Cache is synced to the Map, this is safe.
    auto classIter = ClassMap.find(Name);
    Class* Class = classIter->second;

    // TODO: Why do we check the file?
    std::ifstream File(Name.c_str(), std::ios::binary);
    File.close();

    return Class;
}

/**
 * A relatively complex wrapper to load a class from the given path, to the given Class*.
 * This is where the Class Prefix is handled.
 * See EntryPoint.cpp for what the Class Prefix is, and why we need it.
 *
 * This calls out to Class#LoadFromFile to perform the actual classloading.
 * @param ClassName the name of the class to load, from the Java Language's perspective.
 * @param Class the class instance to laod it into
 * @return whether the class was successfully loaded.
 */
bool ClassHeap::LoadClass(const char *ClassName, Class *Class) {
    if(!Class) return false;

    const char* RelativePath;

    // The ClassName is given to us by the Java Language.
    // Ergo, we're going to be given something like "OtherClass", or "java/lang/OtherClass".

    // However, if we started executing in a different folder than our working directory, then we can't just
    // load the file as-is.

    // As explained in EntryPoint.cpp, we need to handle the Class Prefix, which is the difference between the
    // cwd and the Class' location.

    // Thus, we do a simple transformation; prepend the Prefix, add the Class Name, and add ".class" on the end.
    // It is now a valid file name that we can load from disk, assuming it actually exists.

    std::string pathTemp;
    std::string classTemp(ClassName);
    if(classTemp.find("java/") != 0 && classTemp.find(ClassPrefix) != 0)
        pathTemp.append(ClassPrefix);
    pathTemp.append(ClassName);
    pathTemp.append(".class");

    // We need to transform it to a char* so that it can be read properly, so we do that here.
    RelativePath = pathTemp.c_str();

    // Since we're preparing the class for being loaded, we need to tell it which class heap it belongs to.
    // That's this one.
    Class->SetClassHeap(this);

    // The class is being loaded anew, so it can be added to the Cache.
    // This also prevents recursive classloading when the class evaluates all of its' references.
    ClassCache.emplace_back(ClassName);

    // Now spin out to do the actual classloading.
    // If it fails, it'll return here, so we check it.
    if(!Class->LoadFromFile(RelativePath))
        return false;

    // With the class in the cache and loaded properly, we can add it to the Map and continue with what we were doing.
    return AddClass(Class);
}
