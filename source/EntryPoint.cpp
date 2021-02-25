/**************
 * GEMWIRE    *
 *    PURPURI *
 **************/

#include <cstdio>
#include "../headers/Class.hpp"

int main(int argc, char* argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Purpuri VM v1 - Gemwire Institute\n");
        fprintf(stderr, "***************************************\n");
        fprintf(stderr, "Usage: %s <class file>\n", argv[0]);
        fprintf(stderr, "\n16:50 25/02/21 Curle\n");
        return 1;
    }

    bool Res;

    ClassHeap heap;
    Class* Object = new Class();
    Class* GivenClass = new Class();

    Res = heap.LoadClass((char*)"java/lang/Object", Object);
    Res = heap.LoadClass(argv[1], GivenClass);

    return Res;
}