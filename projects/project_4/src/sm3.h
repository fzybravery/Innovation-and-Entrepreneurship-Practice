#ifndef SM3_H
#define SM3_H

#include <cstdint>
#include <vector>
#include <string>

/*
 * SM3 哈希算法类
 * 提供 SM3 摘要计算及十六进制字符串输出功能
 */
class SM3 {
public:
    /*
     * 计算输入数据的 SM3 摘要
     * @param data: 待哈希的字节序列
     * @return 32 字节的哈希值
     */
    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data);

    /*
     * 计算输入字符串的 SM3 摘要，并返回十六进制字符串
     * @param input: 待哈希的字符串
     * @return 64 字符的十六进制哈希字符串（大写）
     */
    static std::string hashHex(const std::string& input);

private:
    // 初始向量（IV），8×32 位
    static constexpr uint32_t IV[8] = {
        0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
        0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
    };

    // 左循环移位操作
    static inline uint32_t rotl(uint32_t x, int n);

    // 常量 Tj
    static inline uint32_t T(int j);

    // 布尔函数 FF
    static inline uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j);

    // 布尔函数 GG
    static inline uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j);

    // 置换函数 P0
    static inline uint32_t P0(uint32_t x);

    // 置换函数 P1
    static inline uint32_t P1(uint32_t x);

    // 数据填充，补齐到 512 位的整数倍
    static void pad(const std::vector<uint8_t>& data, std::vector<uint8_t>& out);

    // 压缩函数，对一个 512-bit 块进行迭代压缩
    static void compress(uint32_t H[8], const uint8_t block[64]);
};

#endif // SM3_H
