#include "crc.h"


char crc8(char* addr, int size)
{
    uint8_t r = 0;
    for (int i = 0; i < size; i++)
    {
        r = (r >> 1) | (r << 7);
        r += addr[i];
    }
    return r;
}
