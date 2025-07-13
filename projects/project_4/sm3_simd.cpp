#include "sm3_simd.h"
#include <cstring>
#include <sstream>
#include <iomanip>

using std::vector;
using std::string;

inline uint32_t SM3_SIMD::rotl(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

inline uint32_t SM3_SIMD::Tj(int j) {
    return (j < 16 ? 0x79cc4519u : 0x7a879d8au);
}

inline uint32_t SM3_SIMD::FF(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j < 16) return x ^ y ^ z;
    return (x & y) | (x & z) | (y & z);
}

inline uint32_t SM3_SIMD::GG(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j < 16) return x ^ y ^ z;
    return (x & y) | ((~x) & z);
}

inline uint32_t SM3_SIMD::P0(uint32_t x) {
    return x ^ rotl(x, 9) ^ rotl(x, 17);
}

inline uint32_t SM3_SIMD::P1(uint32_t x) {
    return x ^ rotl(x, 15) ^ rotl(x, 23);
}

#ifdef __AVX2__
// 对 __m256i 中的 8 个 uint32 同时循环左移 n 位
inline __m256i SM3_SIMD::rotl256(__m256i x, int n) {
    return _mm256_or_si256(_mm256_slli_epi32(x, n), _mm256_srli_epi32(x, 32 - n));
}

// 向量化 P0: x ⊕ (x≪9) ⊕ (x≪17)
inline __m256i SM3_SIMD::P0_256(__m256i x) {
    return _mm256_xor_si256(x,
           _mm256_xor_si256(rotl256(x, 9), rotl256(x, 17)));
}

// 向量化 P1: x ⊕ (x≪15) ⊕ (x≪23)
inline __m256i SM3_SIMD::P1_256(__m256i x) {
    return _mm256_xor_si256(x,
           _mm256_xor_si256(rotl256(x, 15), rotl256(x, 23)));
}
#endif

void SM3_SIMD::pad(const vector<uint8_t>& data, vector<uint8_t>& out) {
    size_t L = data.size();
    uint64_t bitLen = uint64_t(L) * 8;
    out.reserve(((L + 9 + 63) / 64) * 64);
    out.insert(out.end(), data.begin(), data.end());
    out.push_back(0x80);
    size_t zeros = (56 - (L + 1) % 64 + 64) % 64;
    out.insert(out.end(), zeros, 0x00);
    for (int i = 7; i >= 0; --i) {
        out.push_back(uint8_t(bitLen >> (i * 8)));
    }
}

void SM3_SIMD::compress(uint32_t H[8], const uint8_t block[64]) {
    uint32_t W[68], W1[64];
    // W[0..15]
    for (int j = 0; j < 16; ++j) {
        W[j] = (uint32_t(block[4*j]) << 24)
             | (uint32_t(block[4*j+1]) << 16)
             | (uint32_t(block[4*j+2]) <<  8)
             |  uint32_t(block[4*j+3]);
    }
    // W[16..67]
    for (int j = 16; j < 68; ++j) {
#ifdef __AVX2__
        // 利用 AVX2 向量化 P1 计算
        __m256i v_a = _mm256_set1_epi32(int32_t(W[j-16] ^ W[j-9] ^ rotl(W[j-3],15)));
        __m256i v_p1 = P1_256(v_a);
        W[j] = _mm256_extract_epi32(v_p1, 0)
             ^ rotl(W[j-13], 7)
             ^ W[j-6];
#else
        uint32_t tmp = W[j-16] ^ W[j-9] ^ rotl(W[j-3], 15);
        W[j] = P1(tmp) ^ rotl(W[j-13], 7) ^ W[j-6];
#endif
    }
    for (int j = 0; j < 64; ++j) {
        W1[j] = W[j] ^ W[j+4];
    }

    // 初始化寄存器
    uint32_t A=H[0], B=H[1], C=H[2], D=H[3];
    uint32_t E=H[4], F=H[5], G=H[6], Ht=H[7];

    // 64 轮压缩
    for (int j = 0; j < 64; ++j) {
#ifdef __AVX2__
        // 向量化 SS1 计算
        __m256i vA   = _mm256_set1_epi32(int32_t(A));
        __m256i vE   = _mm256_set1_epi32(int32_t(E));
        __m256i vTj  = _mm256_set1_epi32(int32_t(Tj(j)));
        __m256i v1   = _mm256_add_epi32(rotl256(vA,12), _mm256_add_epi32(vE, rotl256(vTj, j)));
        uint32_t SS1 = _mm256_extract_epi32(rotl256(v1, 7), 0);
#else
        uint32_t SS1 = rotl(rotl(A,12) + E + rotl(Tj(j), j), 7);
#endif
        uint32_t SS2 = SS1 ^ rotl(A,12);
        uint32_t TT1 = FF(A,B,C,j) + D + SS2 + W1[j];
        uint32_t TT2 = GG(E,F,G,j) + Ht + SS1 + W[j];
        D = C;  C = rotl(B,9);  B = A;  A = TT1;
        Ht= G;  G = rotl(F,19); F = E;  E = P0(TT2);
    }

    // 更新 IV
    H[0]^=A; H[1]^=B; H[2]^=C; H[3]^=D;
    H[4]^=E; H[5]^=F; H[6]^=G; H[7]^=Ht;
}

std::vector<uint8_t> SM3_SIMD::hash(const std::vector<uint8_t>& data) {
    vector<uint8_t> padded;
    pad(data, padded);

    uint32_t H[8];
    std::memcpy(H, IV, sizeof(H));

    size_t blocks = padded.size() / 64;
    for (size_t i = 0; i < blocks; ++i) {
        compress(H, padded.data() + i*64);
    }

    vector<uint8_t> digest(32);
    for (int i = 0; i < 8; ++i) {
        digest[4*i    ] = uint8_t(H[i] >> 24);
        digest[4*i + 1] = uint8_t(H[i] >> 16);
        digest[4*i + 2] = uint8_t(H[i] >>  8);
        digest[4*i + 3] = uint8_t(H[i]);
    }
    return digest;
}

std::string SM3_SIMD::hashHex(const std::string& input) {
    vector<uint8_t> data(input.begin(), input.end());
    auto dg = hash(data);

    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    for (auto b : dg) {
        oss << std::setw(2) << int(b);
    }
    return oss.str();
}
