#ifndef JX_SPOOKY_HASH_H
#error "Must be included from spooky_hash.h"
#endif

namespace jx
{
inline uint64_t spookyHash64(const void* message, size_t length, uint64_t seed)
{
	uint64_t hash1 = seed;
	spookyHash128(message, length, &hash1, &seed);
	return hash1;
}

inline uint32_t spookyHash32(const void* message, size_t length, uint32_t seed)
{
	uint64_t hash1 = seed, hash2 = seed;
	spookyHash128(message, length, &hash1, &hash2);
	return (uint32_t)hash1;
}

}
