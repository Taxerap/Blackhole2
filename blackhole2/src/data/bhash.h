#ifndef BH2_DATA_BHASH_H
#define BH2_DATA_BHASH_H

#include <pch.h>

// 8-byte hash value.
typedef uint64_t BHash;

// A very simple hash algorithm.
// In Blackhole 1, I use this to create an identifier for clients and use hash map to queue responds.
// This is no longer useful for now.
BHash
CalculateBHash( size_t length, const unsigned char *data );

#endif // !BH2_DATA_BHASH_H
