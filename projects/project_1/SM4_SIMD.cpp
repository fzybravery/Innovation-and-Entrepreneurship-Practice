#include <iostream>
#include <immintrin.h>
#include <vector>
#include <cstdint>

alignas(16) const uint8_t SM4_SBOX[16][16] = {
    // 可从原始 SBOX 填入展开为 16x16 二维表，用于 PSHUFB 查询
    // 示例略去，可用你原来的 SBOX 填充
};

inline __m128i sboxTransform(__m128i input) {
    // 简化处理：逐字节查表实现 S-box，真实优化可用 GFNI 或 PSHUFB
    uint8_t tmp[16], out[16];
    _mm_storeu_si128((__m128i*)tmp, input);
    for (int i = 0; i < 16; i++) {
        uint8_t b = tmp[i];
        out[i] = SM4_SBOX[b >> 4][b & 0x0F];
    }
    return _mm_loadu_si128((__m128i*)out);
}

inline __m128i linearTransform(__m128i sboxOut) {
    // 使用 VPROLD 模拟 SM4 的移位和 XOR
    __m128i r1 = _mm_rol_epi32(sboxOut, 2);
    __m128i r2 = _mm_rol_epi32(sboxOut, 10);
    __m128i r3 = _mm_rol_epi32(sboxOut, 18);
    __m128i r4 = _mm_rol_epi32(sboxOut, 24);

    return _mm_xor_si128(
             _mm_xor_si128(
                 _mm_xor_si128(sboxOut, r1),
                 _mm_xor_si128(r2, r3)),
             r4);
}

void sm4_encrypt_block(__m128i* state, const __m128i rk[32]) {
    for (int i = 0; i < 32; ++i) {
        __m128i temp = _mm_xor_si128(_mm_xor_si128(state[1], state[2]), _mm_xor_si128(state[3], rk[i]));
        __m128i sboxOut = sboxTransform(temp);
        __m128i L = linearTransform(sboxOut);
        __m128i newX = _mm_xor_si128(state[0], L);

        // 循环更新
        state[0] = state[1];
        state[1] = state[2];
        state[2] = state[3];
        state[3] = newX;
    }

    // 输出顺序调整
    __m128i tmp = state[0];
    state[0] = state[3];
    state[3] = tmp;
    tmp = state[1];
    state[1] = state[2];
    state[2] = tmp;
}

int main() {
    alignas(16) uint8_t plaintext[16] = {
        0x01, 0x23, 0x45, 0x67,
        0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98,
        0x76, 0x54, 0x32, 0x10
    };

    __m128i state[4];
    for (int i = 0; i < 4; ++i)
        state[i] = _mm_set_epi32(
            (plaintext[i * 4 + 0] << 24) | (plaintext[i * 4 + 1] << 16) |
            (plaintext[i * 4 + 2] << 8) | plaintext[i * 4 + 3],
            0, 0, 0); // 模拟 32-bit word

    // 示例轮密钥（真实中应通过密钥扩展生成）
    __m128i rk[32];
    for (int i = 0; i < 32; ++i) rk[i] = _mm_set1_epi32(0x12345678 + i);

    sm4_encrypt_block(state, rk);

    std::cout << "加密后数据：" << std::hex;
    for (int i = 0; i < 4; ++i) {
        uint32_t out;
        memcpy(&out, &state[i], sizeof(uint32_t));
        std::cout << out << " ";
    }
    std::cout << std::dec << std::endl;

    return 0;
}
