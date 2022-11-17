/*
 *  Copyright 2014-2022 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef GMSSL_GCM_H
#define GMSSL_GCM_H


#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "gf128.h"
#include "error.h"
#include "endian.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GCM_IV_MIN_SIZE		1
#define GCM_IV_MAX_SIZE		((uint64_t)(1 << (64-3)))
#define GCM_IV_DEFAULT_BITS	96
#define GCM_IV_DEFAULT_SIZE	12

#define GCM_MIN_AAD_SIZE	0
#define GCM_MAX_AAD_SIZE	((uint64_t)(1 << (64-3)))

#define GCM_MIN_PLAINTEXT_SIZE	0
#define GCM_MAX_PLAINTEXT_SIZE	((((uint64_t)1 << 39) - 256) >> 3)


#define GHASH_SIZE		(16)


#define GCM_IS_LITTLE_ENDIAN 1


void ghash(const uint8_t h[16], const uint8_t *aad, size_t aadlen,
	const uint8_t *c, size_t clen, uint8_t out[16]);



void ghash(const uint8_t h[16], const uint8_t *aad, size_t aadlen, const uint8_t *c, size_t clen, uint8_t out[16])
{
	gf128_t H = gf128_from_bytes(h);
	gf128_t X = gf128_zero();
	gf128_t L;

	PUTU64(out, (uint64_t)aadlen << 3);
	PUTU64(out + 8, (uint64_t)clen << 3);
	L = gf128_from_bytes(out);

	while (aadlen) {
		gf128_t A;
		if (aadlen >= 16) {
			A = gf128_from_bytes(aad);
			aad += 16;
			aadlen -= 16;
		} else {
			memset(out, 0, 16);
			memcpy(out, aad, aadlen);
			A = gf128_from_bytes(out);
			aadlen = 0;
		}
		X = gf128_add(X, A);
		X = gf128_mul(X, H);
	}

	while (clen) {
		gf128_t C;
		if (clen >= 16) {
			C = gf128_from_bytes(c);
			c += 16;
			clen -= 16;
		} else {
			memset(out, 0, 16);
			memcpy(out, c, clen);
			C = gf128_from_bytes(out);
			clen = 0;
		}
		X = gf128_add(X, C);
		X = gf128_mul(X, H);
	}

	X = gf128_add(X, L);
	H = gf128_mul(X, H);
	gf128_to_bytes(H, out);
}


#ifdef __cplusplus
}
#endif
#endif
