// SPDX-License-Identifier: MIT
#include "selinos_kabi_policy.h"

static const selinos_u32 sha256_constants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

static selinos_u32 rotr(selinos_u32 value, selinos_u32 bits)
{
    return (value >> bits) | (value << (32U - bits));
}

static selinos_u32 read_be32(const selinos_u8 *bytes)
{
    return ((selinos_u32)bytes[0] << 24U) | ((selinos_u32)bytes[1] << 16U) |
           ((selinos_u32)bytes[2] << 8U) | (selinos_u32)bytes[3];
}

static void write_be32(selinos_u8 *bytes, selinos_u32 value)
{
    bytes[0] = (selinos_u8)(value >> 24U);
    bytes[1] = (selinos_u8)(value >> 16U);
    bytes[2] = (selinos_u8)(value >> 8U);
    bytes[3] = (selinos_u8)value;
}

static void sha256_block(selinos_u32 state[8], const selinos_u8 block[64])
{
    selinos_u32 words[64];
    selinos_u32 a;
    selinos_u32 b;
    selinos_u32 c;
    selinos_u32 d;
    selinos_u32 e;
    selinos_u32 f;
    selinos_u32 g;
    selinos_u32 h;
    unsigned int index;

    for (index = 0U; index < 16U; ++index) {
        words[index] = read_be32(block + index * 4U);
    }
    for (index = 16U; index < 64U; ++index) {
        const selinos_u32 sigma0 = rotr(words[index - 15U], 7U) ^
                                   rotr(words[index - 15U], 18U) ^
                                   (words[index - 15U] >> 3U);
        const selinos_u32 sigma1 = rotr(words[index - 2U], 17U) ^
                                   rotr(words[index - 2U], 19U) ^
                                   (words[index - 2U] >> 10U);
        words[index] = words[index - 16U] + sigma0 + words[index - 7U] + sigma1;
    }
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    for (index = 0U; index < 64U; ++index) {
        const selinos_u32 sum1 = rotr(e, 6U) ^ rotr(e, 11U) ^ rotr(e, 25U);
        const selinos_u32 choose = (e & f) ^ ((~e) & g);
        const selinos_u32 temporary1 = h + sum1 + choose + sha256_constants[index] + words[index];
        const selinos_u32 sum0 = rotr(a, 2U) ^ rotr(a, 13U) ^ rotr(a, 22U);
        const selinos_u32 majority = (a & b) ^ (a & c) ^ (b & c);
        const selinos_u32 temporary2 = sum0 + majority;
        h = g; g = f; f = e; e = d + temporary1;
        d = c; c = b; b = a; a = temporary1 + temporary2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void selinos_kabi_sha256(const selinos_u8 *data, selinos_size_t size,
                         selinos_u8 digest[SELINOS_KABI_SHA256_BYTES])
{
    selinos_u32 state[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };
    selinos_u8 tail[128] = {0};
    selinos_size_t offset = 0U;
    selinos_size_t tail_size;
    selinos_u64 bit_length = (selinos_u64)size * 8U;
    unsigned int index;

    if (digest == (void *)0 || (data == (void *)0 && size != 0U)) {
        return;
    }
    while (size - offset >= 64U) {
        sha256_block(state, data + offset);
        offset += 64U;
    }
    tail_size = size - offset;
    for (index = 0U; index < tail_size; ++index) {
        tail[index] = data[offset + index];
    }
    tail[tail_size] = 0x80U;
    if (tail_size >= 56U) {
        sha256_block(state, tail);
        for (index = 0U; index < 64U; ++index) {
            tail[index] = 0U;
        }
    }
    for (index = 0U; index < 8U; ++index) {
        tail[56U + index] = (selinos_u8)(bit_length >> (56U - 8U * index));
    }
    sha256_block(state, tail);
    for (index = 0U; index < 8U; ++index) {
        write_be32(digest + index * 4U, state[index]);
    }
}

int selinos_kabi_check_pinned_digest(const selinos_u8 *image,
                                     selinos_size_t image_size,
                                     const selinos_u8 expected_digest[SELINOS_KABI_SHA256_BYTES])
{
    selinos_u8 actual[SELINOS_KABI_SHA256_BYTES];
    selinos_u8 difference = 0U;
    unsigned int index;

    if (image == (void *)0 || expected_digest == (void *)0) {
        return SELINOS_KABI_POLICY_E_ARGUMENT;
    }
    selinos_kabi_sha256(image, image_size, actual);
    for (index = 0U; index < SELINOS_KABI_SHA256_BYTES; ++index) {
        difference |= actual[index] ^ expected_digest[index];
    }
    return difference == 0U ? SELINOS_KABI_POLICY_OK : SELINOS_KABI_POLICY_E_HASH;
}
