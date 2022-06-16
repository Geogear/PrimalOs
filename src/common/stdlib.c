#include <common/stdlib.h>
#include <kernel/mem.h>

static uint32_t state = 777;

__inline__ uint32_t div(uint32_t dividend, uint32_t divisor) {
    // Use long division, but in binary.
    uint32_t denom = divisor;
    uint32_t current = 1;
    uint32_t answer = 0;

    if ( denom > dividend)
        return 0;

    if ( denom == dividend)
        return 1;

    while (denom <= dividend) {
        denom <<= 1;
        current <<= 1;
    }

    denom >>= 1;
    current >>= 1;

    while (current!=0) {
        if ( dividend >= denom) {
            dividend -= denom;
            answer |= current;
        }
        current >>= 1;
        denom >>= 1;
    }
    return answer;
}

__inline__ divmod_t divmod(uint32_t dividend, uint32_t divisor) {
    divmod_t res;
    res.div = div(dividend, divisor);
    res.mod = dividend - res.div*divisor;
    return res;
}

__inline__ uint32_t __aeabi_uidiv(uint32_t dividend, uint32_t divisor) {
    // Use long division, but in binary.
    uint32_t denom = divisor;
    uint32_t current = 1;
    uint32_t answer = 0;

    if ( denom > dividend)
        return 0;

    if ( denom == dividend)
        return 1;

    while (denom <= dividend) {
        denom <<= 1;
        current <<= 1;
    }

    denom >>= 1;
    current >>= 1;

    while (current!=0) {
        if ( dividend >= denom) {
            dividend -= denom;
            answer |= current;
        }
        current >>= 1;
        denom >>= 1;
    }
    return answer;
}

__inline__ divmod_t __aeabi_uidivmod(uint32_t dividend, uint32_t divisor) {
    divmod_t res;
    res.div = __aeabi_uidiv(dividend, divisor);
    res.mod = dividend - res.div*divisor;
    return res;
}

void* memcpy(void *dest, const void *src, size_t n)
{
	char *s = (char *)src;
	char *d = (char *)dest;
	while(n > 0)
	{
		*d++ = *s++;
		n--;
	}
	return dest;
}

void bzero(void* dest, int bytes) {
    char* d = dest;
    while (bytes--) {
        *d++ = 0;
    }
}

char * itoa(int num, int base) {
    static char intbuf[33];
    uint32_t j = 0, isneg = 0, i;
    divmod_t divmod_res;

    if (num == 0) {
        intbuf[0] = '0';
        intbuf[1] = '\0';
        return intbuf;
    }

    if (base == 10 && num < 0) {
        isneg = 1;
        num = -num;
    }

    i = (uint32_t) num;

    while (i != 0) {
       divmod_res = divmod(i,base);
       intbuf[j++] = (divmod_res.mod) < 10 ? '0' + (divmod_res.mod) : 'a' + (divmod_res.mod) - 10;
       i = divmod_res.div;
    }

    if (isneg)
        intbuf[j++] = '-';

    if (base == 16) {
        intbuf[j++] = 'x';
        intbuf[j++] = '0';
    } else if(base == 8) {
        intbuf[j++] = '0';
    } else if(base == 2) {
        intbuf[j++] = 'b';
        intbuf[j++] = '0';
    }

    intbuf[j] = '\0';
    j--;
    i = 0;
    while (i < j) {
        isneg = intbuf[i];
        intbuf[i] = intbuf[j];
        intbuf[j] = isneg;
        i++;
        j--;
    }

    return intbuf;
}

int atoi(char* str) {
    // Initialize result
    int res = 0;
 
    // Initialize sign as positive
    int sign = 1;
 
    // Initialize index of first digit
    int i = 0;
 
    // If number is negative,
    // then update sign
    if (str[0] == '-') {
        sign = -1;
 
        // Also update index of first digit
        i++;
    }
 
    // Iterate through all digits
    // and update the result
    for (; str[i] != '\0'; ++i)
        res = res * 10 + str[i] - '0';
 
    // Return result with sign
    return sign * res;
}

void* malloc(uint32_t bytes){
    return mem_alloc(bytes, get_heap_head());
}

void free(void* ptr){
    return mem_free(ptr);
}

void srand(uint32_t seed){
    state = seed;
}

uint32_t rand(void){
   state = state * 1664525 + 1013904223;
   return state;
}