#include <common/string.h>

uint32_t strlen(const char* str){
    uint32_t i = 0;
    for(; i < 0xffffffff; ++i){
        if(str[i] == '\0')
            break;
    }
    return i;
}

uint8_t strequal(const char* str1, const char* str2){
    uint32_t len1 = strlen(str1);
    uint32_t len2 = strlen(str2);

    if(len1 != len2)
        return 0;

    for(uint32_t i = 0; i < len1; ++i){
        if(str1[i] != str2[i])
            return 0;
    }

    return 1;
}