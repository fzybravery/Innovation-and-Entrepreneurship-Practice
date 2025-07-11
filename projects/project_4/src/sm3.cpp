#include "sm3.h"
#include <sstream>
#include <iomanip>
#include <cstring>

using std::vector;
using std::string;

/*
 * 左循环移位
 */
inline uint32_t SM3::rotl(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

/*
 * 常量 Tj：0≤j≤15 时取 Tj15，否则取 Tj63
 */
inline uint32_t SM3::T(int j) {
    return (j < 16) ? 0x79cc4519u : 0x7a879d8au;
}

/*
 * 布尔函数 FF：
 * 0 ≤ j ≤ 15: x⊕y⊕z
 * 16 ≤ j ≤ 63: (x∧y)∨(x∧z)∨(y∧z)
 */
inline uint32_t SM3::FF(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j < 16) return x ^ y ^ z;
    return (x & y) | (x & z) | (y & z);
}

/*
 * 布尔函数 GG：
 * 0 ≤ j ≤ 15: x⊕y⊕z
 * 16 ≤ j ≤ 63: (x∧y)∨((¬x)∧z)
 */
inline uint32_t SM3::GG(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j < 16) return x ^ y ^ z;
    return (x & y) | ((~x) & z);
}

/*
 * 置换函数 P0：x⊕(x≪9)⊕(x≪17)
 */
inline uint32_t SM3::P0(uint32_t x) {
    return x ^ rotl(x, 9) ^ rotl(x, 17);
}

/*
 * 置换函数 P1：x⊕(x≪15)⊕(x≪23)
 */
inline uint32_t SM3::P1(uint32_t x) {
    return x ^ rotl(x, 15) ^ rotl(x, 23);
}

/*
 * 数据填充：先 append 0x80，再补 0，最后 append 64-bit 长度
 */
void SM3::pad(const vector<uint8_t>& data, vector<uint8_t>& out) {
    size_t len = data.size();
    uint64_t bitLen = static_cast<uint64_t>(len) * 8;

    // 原始数据复制
    out.reserve(((len + 9 + 63) / 64) * 64);
    out.insert(out.end(), data.begin(), data.end());

    // 填充 0x80
    out.push_back(0x80);
    // 填充 0x00
    size_t padZeros = (56 - (len + 1) % 64 + 64) % 64;
    out.insert(out.end(), padZeros, 0x00);

    // 长度 64-bit 大端序
    for (int i = 7; i >= 0; --i) {
        out.push_back(static_cast<uint8_t>(bitLen >> (i * 8)));
    }
}

/*
 * 压缩函数：消息扩展 + 64 轮迭代
 */
void SM3::compress(uint32_t H[8], const uint8_t block[64]) {
    uint32_t W[68], W1[64];

    // W[0..15] 由 block 直接填充
    for (int j = 0; j < 16; ++j) {
        W[j] = (static_cast<uint32_t>(block[4*j]) << 24)
             | (static_cast<uint32_t>(block[4*j+1]) << 16)
             | (static_cast<uint32_t>(block[4*j+2]) << 8)
             |  static_cast<uint32_t>(block[4*j+3]);
    }
    // W[16..67] 由公式扩展
    for (int j = 16; j < 68; ++j) {
        uint32_t tmp = W[j-16] ^ W[j-9] ^ rotl(W[j-3], 15);
        W[j] = P1(tmp) ^ rotl(W[j-13], 7) ^ W[j-6];
    }
    // W1[j] = W[j] ⊕ W[j+4]
    for (int j = 0; j < 64; ++j) {
        W1[j] = W[j] ^ W[j+4];
    }

    // 迭代寄存器初始化
    uint32_t A = H[0], B = H[1], C = H[2], D = H[3];
    uint32_t E = H[4], F = H[5], G = H[6], Ht = H[7];

    // 64 轮迭代
    for (int j = 0; j < 64; ++j) {
        uint32_t SS1 = rotl(rotl(A, 12) + E + rotl(T(j), j), 7);
        uint32_t SS2 = SS1 ^ rotl(A, 12);
        uint32_t TT1 = FF(A, B, C, j) + D + SS2 + W1[j];
        uint32_t TT2 = GG(E, F, G, j) + Ht + SS1 + W[j];

        D  = C;
        C  = rotl(B, 9);
        B  = A;
        A  = TT1;
        Ht = G;
        G  = rotl(F, 19);
        F  = E;
        E  = P0(TT2);
    }

    // 更新哈希值
    H[0] ^= A; H[1] ^= B; H[2] ^= C; H[3] ^= D;
    H[4] ^= E; H[5] ^= F; H[6] ^= G; H[7] ^= Ht;
}

/*
 * 对外接口：计算字节数组哈希
 */
std::vector<uint8_t> SM3::hash(const std::vector<uint8_t>& data) {
    // 1. 填充
    std::vector<uint8_t> padded;
    pad(data, padded);

    // 2. 初始化向量
    uint32_t H[8];
    std::memcpy(H, IV, sizeof(H));

    // 3. 分组压缩
    size_t blocks = padded.size() / 64;
    for (size_t i = 0; i < blocks; ++i) {
        compress(H, padded.data() + i * 64);
    }

    // 4. 输出 32 字节摘要
    std::vector<uint8_t> digest(32);
    for (int i = 0; i < 8; ++i) {
        digest[4*i    ] = static_cast<uint8_t>(H[i] >> 24);
        digest[4*i + 1] = static_cast<uint8_t>(H[i] >> 16);
        digest[4*i + 2] = static_cast<uint8_t>(H[i] >> 8);
        digest[4*i + 3] = static_cast<uint8_t>(H[i]);
    }
    return digest;
}

/*
 * 对外接口：计算字符串哈希并返回大写十六进制
 */
std::string SM3::hashHex(const std::string& input) {
    std::vector<uint8_t> data(input.begin(), input.end());
    auto digest = hash(data);

    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    for (auto b : digest) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}
