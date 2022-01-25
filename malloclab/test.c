#include <stdio.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

int main() {
    unsigned int i = 0;
    int p = 32-__builtin_clz(i)-1;
    printf("%d\n",p);
    exit(0);
}