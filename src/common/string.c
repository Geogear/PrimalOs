#include <common/string.h>

uint32_t strlen(const char* str){
    uint32_t i = 0;
    for(; i < 0xffffffff; ++i){
        if(str[i] == '\0')
            break;
    }
    return i;
}