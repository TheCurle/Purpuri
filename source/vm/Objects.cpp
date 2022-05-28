/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <vm/Objects.hpp>
#include <vm/Class.hpp>

#include <cstring>

// This is the global Object that represents "null" across the whole JVM.
// This facilitates null checking, while retaining a valid Object on the heap.
// It is imperative that everything that COULD use a null value, check that it doesn't.
Object ObjectHeap::Null;

/**
 * This file implements the ObjectHeap class, declared in Objects.hpp.
 *
 * Objects are a simple pair of ID, Heap.
 * ID is an ascending long value that uniquely identifies each Object.
 * Heap is a pointer to the actual Variable data on the heap.
 *
 * Heap Data is implemented as an array of Variables.
 * See the enum in Common.hpp for how Variable is implemented.
 *
 * In short, classes will have the format of:
 * ┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
 * │                                                                                                             │
 * │  ┌───────────────────────┐  ┌───────────────────────┐ ┌─ ─ ── ── ── ── ── ── ─┐ ┌─ ─ ── ── ── ── ── ── ─┐   │
 * │  │                       │  │                       │ │                       │ │                       │   │
 * │  │                       │  │                       │                                                       │
 * │  │    Class Pointer      │  │    Class Variables    │ │    Class Variables    │ │    Class Variables    │   │
 * │  │                       │  │                       │                                                       │
 * │  │                       │  │                       │ │                       │ │                       │   │
 * │  └───────────────────────┘  └───────────────────────┘ └─ ─ ── ── ── ── ── ── ─┘ └─ ─ ── ── ── ── ── ── ─┘   │
 * │                                                                                                             │
 * └─────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
 *
 * Class Variables will continue for as many fields are in the class (including superclasses).
 * ie. a simple Object with no fields will have no Variables, a class with two fields will have two Class Variables
 *  (for a total length of three), etc
 *
 * Arrays will have the format of:
 * ┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
 * │                                                                                                             │
 * │  ┌───────────────────────┐  ┌───────────────────────┐ ┌─ ─ ── ── ── ── ── ── ─┐ ┌─ ─ ── ── ── ── ── ── ─┐   │
 * │  │                       │  │                       │ │                       │ │                       │   │
 * │  │                       │  │                       │                                                       │
 * │  │      Array Type       │  │       Array Data      │ │       Array Data      │ │       Array Data      │   │
 * │  │                       │  │                       │                                                       │
 * │  │                       │  │                       │ │                       │ │                       │   │
 * │  └───────────────────────┘  └───────────────────────┘ └─ ─ ── ── ── ── ── ── ─┘ └─ ─ ── ── ── ── ── ── ─┘   │
 * │                                                                                                             │
 * └─────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
 *
 * The Array Type is either primitive or reference.
 * Reference includes other arrays.
 * As you'd expect, Array Data continues for the length of the array.
 *
 * To facilitate the arraylength instruction, there's also an Array Size Map here, that provides fast ID -> Length mapping.
 *
 * Otherwise, this file is mostly dedicated to the management of these IDs and the references to the Heap values they contain.
 */


ObjectHeap::ObjectHeap() {
    NextObjectID = 1;
    ObjectMap = std::map<size_t, size_t>();
}

ObjectHeap::~ObjectHeap() = default;

/**
 * A simple wrapper to create a new instance of the given class.
 * The instance is returned as an Object.
 *
 * The Object will have its' layout according to that of the main comment;
 *  First, the class pointer, then the class field data.
 *
 * Once the new Object is ready, it will be emplaced into the heap.
 *
 * @param Class the Class to be instantiated.
 * @return the constructed class, or the Null object if failed.
 */

Object ObjectHeap::CreateObject(Class *Class) {
    if(Class == nullptr) return Null;

    // This is where we set the length of the Variable Data.
    size_t ObjectSize = Class->GetClassFieldCount() + 1;
    auto* ClassObj = new Variable[ObjectSize];

    Object object{};
    object.Heap = NextObjectID++;
    object.Type = 0;

    // The first entry in the Variable Data is the Class pointer.
    ClassObj[0].pointerVal = (size_t) Class;

    // The Object is placed into the map so that we can fetch it later.
    ObjectMap.emplace((size_t) object.Heap, (size_t) ClassObj);

    return object;
}

/**
 * A simple wrapper to retrieve the Variable Data of the given Object.
 * Effectively reads the heap ID and looks up the data in the Object Map.
 * @param obj the Object to retrieve the data of
 * @return the Variable Data of the given Object.
 */
Variable* ObjectHeap::GetObjectPtr(Object obj) {
    auto objIter = ObjectMap.find(obj.Heap);

    // Perform a basic sanity check, but this should not be possible in normal usage.
    // If an Object has been instantiated (and a new ID generated) without being placed in the Map, something
    // has gone horribly wrong.
    if(objIter == ObjectMap.end()) {
        printf("******************\nObject Heap does not contain object " PrtSizeT ". Highest object is " PrtSizeT ".\n******************\n", obj.Heap, ObjectMap.size());
        for(;;);
    }

    // For most normal usage, just cast and return the map value.
    return (Variable*) objIter->second;
}

/**
 * Creating an instance of a String is more complex than most other Objects.
 * Primarily, we need to account for interning, which breaks the usual Object pattern.
 *
 * However, if we assume this method is the end point of all interning handling, then it's simple;
 *  - Create an instance of the String class
 *  - Create a Char array to fit inside it
 *  - Copy the String data into the char array
 *  - Put the char array into the class instance
 *  - Return the class instance
 *
 * That's rather a lot of work, so it's handled in this wrapper.
 *
 * @param String the String data to put in the new instance
 * @param ClassHeap the Heap to load the String class from.
 * @return the new String instance as an Object
 */
Object ObjectHeap::CreateString(const std::string& String, ClassHeap *ClassHeap) {
    // Retrieve the String class
    if(ClassHeap == nullptr) return Null;
    Class* Class = ClassHeap->GetClass("java/lang/String");
    if(Class == nullptr) return Null;

    // Create a new String instance
    Object obj = CreateObject(Class);
    Variable* Var = this->GetObjectPtr(obj);
    if(Var == nullptr) return Null;

    // Create a new char array
    Object array = CreateArray(5, String.length());
    auto* data = (Variable*) ObjectMap.at(array.Heap);
    
    // Copy the string into the array
    const char* str = String.c_str();
    size_t strLen = String.length();
    for(size_t i = 0; i < strLen; i++)
        data[i + 1] = Variable(str[i]);

    // Append the array to the String instance, storing the data within.
    Var[1].object = array;

    return obj;
}

/**
 * A simple wrapper to create an array Object on heap.
 *
 * An array structure is specified in the primary comment of this file.
 * In short, the first index specifies the type of the array, followed by the data.
 *
 * @param Type the Type of the data that will go into this Array.
 * @param Count the amount of data that will go into this Array.
 * @return the new, empty Array with the correct allocated size
 */
Object ObjectHeap::CreateArray(uint8_t Type, uint32_t Count) {
    // Allocate the Variable Data first.
    auto* array = new Variable[Count + 1];

    // Prepare the Object ready for emplacement later
    Object object{};
    object.Heap = NextObjectID++;
    object.Type = Type;

    // Initialize the Variable Data, since we've only allocated it
    for (size_t i = 0; i < Count + 1; i++)
        array[i] = Variable((char) 0);
    // Set the first index; array type.
    array[0].intVal = Type;

    // Emplace the object into the required maps - it's an array and an Object, so there's two.
    ObjectMap.emplace((size_t) object.Heap, (size_t) array);
    ArraySizeMap.emplace((size_t) object.Heap, (size_t) Count);

    return object;
}

/**
 * A simple wrapper to create an array of Class Instances.
 * This is much the same as a regular array, except its' type is replaced with the class pointer.
 *
 * @param pClass the Class type that this array will store
 * @param Count the number of items in the array.
 * @return the empty, allocated Array of Class instances.
 */
Object ObjectHeap::CreateObjectArray(Class* pClass, uint32_t Count) {
    // Pre-allocate the Variable Data.
    auto* array = new Variable[Count];

    // Initialize the Variables, since we only allocated them
    for (size_t i = 1; i < Count; i++)
        array[i] = Variable((char) 0);
    // Set the first index; the class type
    array[0].pointerVal = (size_t) pClass;

    Object object{};
    object.Heap = NextObjectID++;

    // This is an Object and an array, so emplace it into two Maps.
    ObjectMap.emplace((size_t) object.Heap, (size_t) array);
    ArraySizeMap.emplace((size_t) object.Heap, (size_t) Count);
    return object;
}

/**
 * A simple wrapper to query the length of an array Object.
 * @param obj the array to query
 * @return the length of the given array.
 */
size_t ObjectHeap::GetArraySize(Object obj) {
    auto objIter = ArraySizeMap.find(obj.Heap);

    // Quick sanity check - this should not be possible in normal usage.
    if(objIter == ArraySizeMap.end())
        return 0;

    return objIter->second;
}