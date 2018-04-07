#ifndef JX_HASH128_H
#define JX_HASH128_H

#include <stdint.h>

namespace jx
{
#define JX_HASH128_DEFAULT_HASH_SEED_1 0xdeadbeefbaadf00duLL
#define JX_HASH128_DEFAULT_HASH_SEED_2 0xf005ba11deadbea7uLL

struct hash128
{
	uint64_t m_Value[2];

	hash128(uint64_t seed1 = JX_HASH128_DEFAULT_HASH_SEED_1, uint64_t seed2 = JX_HASH128_DEFAULT_HASH_SEED_2);
	~hash128();

	bool operator < (const hash128& other) const;
	bool operator == (const hash128& other) const;
};
}

#include "inline/hash128.inl"

#endif
