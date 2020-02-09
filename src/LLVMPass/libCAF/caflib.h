#ifndef CAFLIB
#define CAFLIB

#include<cstdio>
#include<vector>

void inputIntTo(int *dest)
{
    *dest = 0;
    for(int i = 0; i < 4; i++)
    {
        unsigned char cur = getchar();
        // printf("%d\n", cur);
        *dest += (cur * (1 << i*8));
    }
    // printf("%d\n", *dest);
}

void inputBytesTo(char *dest, int size)
{
    // printf("size = %d\n", size);
    for(int i = 0; i < size; i++){
        // printf("%d\n", i);
        scanf("%c", dest + i);
        // printf("%c", *(dest + i));
    }
}

std::vector<long>__caf_object_list;
void saveToObjectList(long objPtr) // _Z16saveToObjectListl
{
    __caf_object_list.push_back(objPtr);
}

long getFromObjectList(int objIdx) // _Z17getFromObjectListi
{
    if(objIdx >= __caf_object_list.size())return long(0);
    return __caf_object_list[objIdx];
}


#endif //CAFLIB