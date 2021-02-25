/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#pragma once
#include "Class.hpp"

class ObjectHeap {
    public:
        ObjectHeap();
        virtual ~ObjectHeap();

        virtual Variable* GetObjectPtr(Object obj);
        Object CreateObject(Class* Class);
        Object CreateString(char** String, ClassHeap* ClassHeap);
        Object CreateArray(uint8_t type, int16_t Count);
        bool CreateObjectArray(Class* Class, uint32_t Count, Object &Object);

    private:

        std::map<size_t, size_t> ObjectMap;
        uint32_t NextObjectID;
};