/* sha256.c — SHA-256 puro (FIPS 180-4), C99, sem SIMD.
 * oep-tools: host e inside OvsbOS. Domínio público. */
#include "sha256.h"
#include <string.h>

static uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };

static uint32_t rotr(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }

void sha256_init(sha256_ctx *c) {
    c->h[0] = 0x6a09e667; c->h[1] = 0xbb67ae85; c->h[2] = 0x3c6ef372; c->h[3] = 0xa54ff53a;
    c->h[4] = 0x510e527f; c->h[5] = 0x9b05688c; c->h[6] = 0x1f83d9ab; c->h[7] = 0x5be0cd19;
    c->len = 0; c->buflen = 0;
}

static void sha256_block(sha256_ctx *c, const uint8_t *p) {
    uint32_t w[64], v[8], t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[i*4] << 24) | ((uint32_t)p[i*4+1] << 16) |
               ((uint32_t)p[i*4+2] << 8) | (uint32_t)p[i*4+3];
    for (i = 16; i < 64; i++) {
        uint32_t s0 = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
        uint32_t s1 = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    for (i = 0; i < 8; i++) v[i] = c->h[i];
    for (i = 0; i < 64; i++) {
        uint32_t S1 = rotr(v[4],6) ^ rotr(v[4],11) ^ rotr(v[4],25);
        uint32_t ch = (v[4] & v[5]) ^ ((~v[4]) & v[6]);
        t1 = v[7] + S1 + ch + K[i] + w[i];
        uint32_t S0 = rotr(v[0],2) ^ rotr(v[0],13) ^ rotr(v[0],22);
        uint32_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
        t2 = S0 + maj;
        v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
        v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
    }
    for (i = 0; i < 8; i++) c->h[i] += v[i];
}

void sha256_update(sha256_ctx *c, const void *data, size_t n) {
    const uint8_t *p = data;
    c->len += (uint64_t)n;
    while (c->buflen + n >= 64) {
        size_t take = 64 - c->buflen;
        memcpy(c->buf + c->buflen, p, take);
        sha256_block(c, c->buf);
        c->buflen = 0;
        p += take; n -= take;
    }
    if (n) { memcpy(c->buf + c->buflen, p, n); c->buflen += n; }
}

void sha256_final(sha256_ctx *c, uint8_t out[32]) {
    uint64_t bits = c->len * 8;
    uint8_t pad = 0x80;
    int i;
    sha256_update(c, &pad, 1);
    pad = 0;
    while (c->buflen != 56)
        sha256_update(c, &pad, 1);
    {
        uint8_t lenb[8];
        for (i = 0; i < 8; i++) lenb[i] = (uint8_t)(bits >> (56 - i * 8));
        memcpy(c->buf + 56, lenb, 8);
        sha256_block(c, c->buf);
    }
    for (i = 0; i < 8; i++) {
        out[i*4]   = (uint8_t)(c->h[i] >> 24);
        out[i*4+1] = (uint8_t)(c->h[i] >> 16);
        out[i*4+2] = (uint8_t)(c->h[i] >> 8);
        out[i*4+3] = (uint8_t)c->h[i];
    }
}

void sha256(const void *data, size_t n, uint8_t out[32]) {
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, data, n);
    sha256_final(&c, out);
}
