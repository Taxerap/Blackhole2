#include "bhash.h"

// Some random prime numbers I found at <https://bigprimes.org>.
static const uint64_t HASH_ORIGIN = 0x36fa2296f7439;
static const uint64_t HASH_PRIME = 0x39bcbddd021;
static const char HASH_SALT[] = "_EraP.TaX_";

BHash
CalculateBHash( size_t length, const unsigned char *data )
{
    BHash res = HASH_ORIGIN;

    for (size_t i = 0; i < length; i++)
    {
        res ^= *data;
        res += HASH_PRIME;
    }
    res *= HASH_PRIME;
    for (size_t i = 0; i < sizeof(HASH_SALT) - 1; i++)
    {
        res ^= HASH_SALT[i];
        res *= HASH_PRIME;
    }

    return res;
}
