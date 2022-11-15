/*
 *  Copyright 2014-2022 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef GMSSL_RAND_H
#define GMSSL_RAND_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "error.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
Rand Public API

	rand_bytes

*/

int rand_bytes(uint8_t *buf, size_t buflen);

int rdrand_bytes(uint8_t *buf, size_t buflen);
int rdseed_bytes(uint8_t *buf, size_t buflen);

#define RAND_MAX_BUF_SIZE 4096

int rand_bytes(uint8_t *buf, size_t len)
{
	FILE *fp;
	if (!buf) {
		error_print();
		return -1;
	}
	if (len > RAND_MAX_BUF_SIZE) {
		error_print();
		return -1;
	}
	if (!len) {
		return 0;
	}

	if (!(fp = fopen("/dev/urandom", "rb"))) {
		error_print();
		return -1;
	}
	if (fread(buf, 1, len, fp) != len) {
		error_print();
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 1;
}


#ifdef __cplusplus
}
#endif
#endif
