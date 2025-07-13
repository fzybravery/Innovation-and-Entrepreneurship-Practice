#ifndef SM3_SIMD_H
#define SM3_SIMD_H

#include <cstdint>
#include <vector>
#include <string>

#ifdef __AVX2__
  #include <immintrin.h>
#endif

/**
 * SM3_SIMD
 * 基于 AVX2 的 SM3 哈希算法实现
 */
class SM3_SIMD {
public:
    // 计算字节数组的 SM3 摘要（32 字节）
    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data);

    // 计算字符串的 SM3 摘要，返回大写十六进制（64 字符）
    static std::string hashHex(const std::string& input);

private:
    // SM3 初始向量
    static constexpr uint32_t IV[8] = {
        0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
        0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
    };

    // 基本操作
    static inline uint32_t rotl(uint32_t x, int n);
    static inline uint32_t Tj(int j);
    static inline uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j);
    static inline uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j);
    static inline uint32_t P0(uint32_t x);
    static inline uint32_t P1(uint32_t x);

#ifdef __AVX2__
    // AVX2 向量化：对 8 个 uint32 同时做循环左移
    static inline __m256i rotl256(__m256i x, int n);
    // P0/P1 的向量版本
    static inline __m256i P0_256(__m256i x);
    static inline __m256i P1_256(__m256i x);
#endif

    // 消息填充
    static void pad(const std::vector<uint8_t>& data, std::vector<uint8_t>& out);

    // 核心压缩函数（每 512bit 块）
    static void compress(uint32_t H[8], const uint8_t block[64]);
};

#endif // SM3_SIMD_H
