#include "caflib.h"
#include <stdio.h>
#include <stdlib.h>

void inputIntTo(int *dest) {
    *dest = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char cur = getchar();
        // printf("%d\n", cur);
        *dest += (cur * (1 << i*8));
    }
    // printf("%d\n", *dest);
}

void inputBytesTo(char *dest, int size) {
    // printf("size = %d\n", size);
    for (int i = 0; i < size; i++) {
        // printf("%d\n", i);
        scanf("%c", dest + i);
        // printf("%c", *(dest + i));
    }
}

// int main()
// {
//     int size;
//     for(int i = 0; i < 3; i++) {
//         inputIntTo(&size);
//         printf("%d\n", size);
//     }
// }
