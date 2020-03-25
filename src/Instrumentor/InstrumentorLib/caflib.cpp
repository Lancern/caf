#include "caflib.h"
#include<stdlib.h>

int main()
{
    int size;
    for(int i = 0; i < 3; i++) {
        inputIntTo(&size);
        printf("%d\n", size);
    }
    double val;
    inputDoubleTo(&val);
    // scanf("%lf", &val);
    printf("val = %lf\n", val);
}
