

#include <string.h>

#include "inner.h"

const uint64_t RC[24] =
{
  0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
  0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
  0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
  0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
  0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
  0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
  0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
  0x8000000000008080, 0x0000000080000001, 0x8000000080008008
};

uint64_t ROTL64(uint64_t x, uint64_t y) {
    if(y == 0 or y == 64) {
        return x;
    }
    return (x << y) | (x >> (64 - y));
}


int process_block(uint64_t* state)
{
    const int rx[25] = {
      0, 1, 62, 28, 27,
      36, 44, 6, 55, 20,
      3, 10, 43, 25, 39,
      41, 45, 15, 21, 8,
      18, 2, 61, 56, 14
    };


    for (int i = 0; i < 24; ++i) {
      
        uint64_t C[5] = {0, 0, 0, 0, 0};
        uint64_t D[5] = {0, 0, 0, 0, 0};

        for (int x = 0; x < 5; ++x) {
            C[x] = state[x] ^ state[5 + x] ^ state[10 + x] ^ state[15 + x] ^ state[20 + x];
        }

        for (int x = 0; x < 5; ++x) {
      
            D[x] = C[(x + 4) % 5] ^ ROTL64(C[(x + 1) % 5], 1);

            for (int y = 0; y < 5; ++y) {
                state[y * 5 + x] = state[y * 5 + x] ^ D[x];
            }
        }

        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                state[y * 5 + x] = ROTL64(state[y * 5 + x], rx[y * 5 + x]);
            }
        }
        uint64_t B[25];

        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                B[y * 5 + x] = state[5 * y + x];
            }
        }
        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                int u = (0 * x + 1 * y) % 5;
                int v = (2 * x + 3 * y) % 5;
                state[v * 5 + u] = B[5 * y + x];
            }
        }

        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                C[x] = state[y * 5 + x] ^ ((~state[y * 5 + ((x + 1) % 5)]) & state[y * 5 + ((x + 2) % 5)]);
            }

            for (int x = 0; x < 5; ++x) {
                state[y * 5 + x] = C[x];
            }
        }
        state[0] = state[0] ^ RC[i];
    }
    return 0;
}





/* see inner.h */
void
inner_shake256_init(inner_shake256_context *sc)
{
	sc->dptr = 0;

	/*
	 * Representation of an all-ones uint64_t is the same regardless
	 * of local endianness.
	 */
	memset(sc->st.A, 0, sizeof sc->st.A);
}

/* see inner.h */
void
inner_shake256_inject(inner_shake256_context *sc, const uint8_t *in, size_t len)
{
	size_t dptr;

	dptr = (size_t)sc->dptr;
	while (len > 0) {
		size_t clen, u;

		clen = 136 - dptr;
		if (clen > len) {
			clen = len;
		}

		for (u = 0; u < clen; u ++) {
			size_t v;

			v = u + dptr;
			sc->st.A[v >> 3] ^= (uint64_t)in[u] << ((v & 7) << 3);
		}

		dptr += clen;
		in += clen;
		len -= clen;
		if (dptr == 136) {
			process_block(sc->st.A);
			dptr = 0;
		}
	}
	sc->dptr = dptr;
}

/* see falcon.h */
void
inner_shake256_flip(inner_shake256_context *sc)
{

	unsigned v;

	v = (unsigned int)sc->dptr;
	sc->st.A[v >> 3] ^= (uint64_t)0x1F << ((v & 7) << 3);
	sc->st.A[16] ^= (uint64_t)0x80 << 56;
	sc->dptr = 136;
}

/* see falcon.h */
void
inner_shake256_extract(inner_shake256_context *sc, uint8_t *out, size_t len)
{
	size_t dptr;

	dptr = (size_t)sc->dptr;
	while (len > 0) {
		size_t clen;

		if (dptr == 136) {
			process_block(sc->st.A);
			dptr = 0;
		}
		clen = 136 - dptr;
		if (clen > len) {
			clen = len;
		}
		len -= clen;

		while (clen -- > 0) {
			*out ++ = sc->st.A[dptr >> 3] >> ((dptr & 7) << 3);
			dptr ++;
		}

	}
	sc->dptr = dptr;
}
