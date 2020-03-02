#ifndef _HMAC_SHA1_H_
#define _HMAC_SHA1_H_

#include "CTool.h"

void hmac_sha1(uint8 *key_buff, int key_length, uint8 *data, int data_length, uint8 *digest);

#endif //_HMAC_SHA1_H_