/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#pragma once
#include "Common.hpp"
#include <map>

class ObjectHeap {
    public:
        ObjectHeap();
        virtual ~ObjectHeap();

        static Object Null;

        virtual Variable* GetObjectPtr(Object obj);
        virtual size_t GetArraySize(Object obj);

        Object CreateObject(Class* Class);
        Object CreateString(std::string String, ClassHeap* ClassHeap);
        Object CreateArray(uint8_t Type, uint32_t Count);
        bool CreateObjectArray(Class* Class, uint32_t Count, Object &Object);

    private:
        std::map<size_t, size_t> ObjectMap;
        std::map<size_t, size_t> ArraySizeMap;
        uint32_t NextObjectID;
};