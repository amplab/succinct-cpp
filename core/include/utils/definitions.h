#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#ifndef MAX
#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* Check if n is a power of 2 */
#define ISPOWOF2(n)     ((n != 0) && ((n & (n - 1)) == 0))

/* Bitwise operations */
#define GETBIT(n, i)    ((n >> (63UL - i)) & 1UL)
#define SETBIT(n, i)    n = (n | (1UL << (63UL - i)))
#define CLEARBIT(n, i)  n = (n & ~(1UL << (63UL - i)))

#define GETBIT16(n, i)  ((n >> (15UL - i)) & 1UL)
#define SETBIT16(n, i)  n = (n | (1UL << (15UL - i)))

#define GETBIT8(n, i)  ((n >> (7UL - i)) & 1UL)
#define SETBIT8(n, i)  n = (n | (1UL << (7UL - i)))

#define BITS2BLOCKS(bits) \
    (((bits) % 64 == 0) ? ((bits) / 64) : (((bits) / 64) + 1))

/* Pop-count Constants */
#define m1   0x5555555555555555 //binary: 0101...
#define m2   0x3333333333333333 //binary: 00110011..
#define m4   0x0f0f0f0f0f0f0f0f //binary:  4 zeros,  4 ones ...
#define m8   0x00ff00ff00ff00ff //binary:  8 zeros,  8 ones ...
#define m16  0x0000ffff0000ffff //binary: 16 zeros, 16 ones ...
#define m32  0x00000000ffffffff //binary: 32 zeros, 32 ones
#define hff  0xffffffffffffffff //binary: all ones
#define h01  0x0101010101010101 //the sum of 256 to the power of 0,1,2,3...

#endif
