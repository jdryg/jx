#ifndef JX_SPOOKY_HASH_H
#define JX_SPOOKY_HASH_H

#include <stdint.h>
#include <stddef.h> // size_t

// http://burtleburtle.net/bob/hash/spooky.html
//
// SpookyHash: a 128-bit noncryptographic hash function
// By Bob Jenkins, public domain
//   Oct 31 2010: alpha, framework + SpookyHash::Mix appears right
//   Oct 31 2011: alpha again, Mix only good to 2^^69 but rest appears right
//   Dec 31 2011: beta, improved Mix, tested it for 2-bit deltas
//   Feb  2 2012: production, same bits as beta
//   Feb  5 2012: adjusted definitions of uint* to be more portable
//   Mar 30 2012: 3 bytes/cycle, not 4.  Alpha was 4 but wasn't thorough enough.
//   August 5 2012: SpookyV2 (different results)
// 
// Up to 3 bytes/cycle for long messages.  Reasonably fast for short messages.
// All 1 or 2 bit deltas achieve avalanche within 1% bias per output bit.
//
// This was developed for and tested on 64-bit x86-compatible processors.
// It assumes the processor is little-endian.  There is a macro
// controlling whether unaligned reads are allowed (by default they are).
// This should be an equally good hash on big-endian machines, but it will
// compute different results on them than on little-endian machines.
//
// Google's CityHash has similar specs to SpookyHash, and CityHash is faster
// on new Intel boxes.  MD4 and MD5 also have similar specs, but they are orders
// of magnitude slower.  CRCs are two or more times slower, but unlike 
// SpookyHash, they have nice math for combining the CRCs of pieces to form 
// the CRCs of wholes.  There are also cryptographic hashes, but those are even 
// slower than MD5.
//

namespace jx
{
//
// SpookyHash: hash a single message in one call, produce 128-bit output
//
void spookyHash128(const void* message, size_t length, uint64_t* hash1, uint64_t* hash2);

//
// Hash64: hash a single message in one call, return 64-bit output
//
uint64_t spookyHash64(const void* message, size_t length, uint64_t seed);

//
// Hash32: hash a single message in one call, produce 32-bit output
//
uint32_t spookyHash32(const void* message, size_t length, uint32_t seed);

}

#endif
