#include "sm3.h"
#include <iostream>

// 攻击者伪造的新数据
std::string forgeExtension = "&admin=true";

// 构造假填充
std::vector<uint8_t> createPadding(size_t originalLen) {
    uint64_t bitLen = originalLen * 8;
    std::vector<uint8_t> pad;
    pad.push_back(0x80);
    size_t padZero = (56 - (originalLen + 1) % 64 + 64) % 64;
    pad.insert(pad.end(), padZero, 0x00);
    for (int i = 7; i >= 0; --i) {
        pad.push_back(static_cast<uint8_t>(bitLen >> (i * 8)));
    }
    return pad;
}

// 利用原始哈希值和新数据构造新摘要
std::vector<uint8_t> lengthExtensionAttack(const std::vector<uint8_t>& originalHash, size_t originalLen, const std::string& suffix) {
    SM3 sm3;
    uint32_t H[8];
    for (int i = 0; i < 8; ++i) {
        H[i] = (originalHash[4*i] << 24) | (originalHash[4*i + 1] << 16) |
               (originalHash[4*i + 2] << 8) | originalHash[4*i + 3];
    }

    std::vector<uint8_t> forgedInput(suffix.begin(), suffix.end());
    std::vector<uint8_t> padded;
    sm3.pad(forgedInput, padded);

    // 修改填充，使 total length = originalLen + padding + suffix
    size_t fakeTotalLen = originalLen + createPadding(originalLen).size() + forgedInput.size();
    std::vector<uint8_t> actualPadding = createPadding(fakeTotalLen - forgedInput.size());

    // 重新压缩
    size_t blocks = padded.size() / 64;
    for (size_t i = 0; i < blocks; ++i) {
        sm3.compress(H, padded.data() + i * 64);
    }

    std::vector<uint8_t> result(32);
    for (int i = 0; i < 8; ++i) {
        result[4*i]     = static_cast<uint8_t>(H[i] >> 24);
        result[4*i + 1] = static_cast<uint8_t>(H[i] >> 16);
        result[4*i + 2] = static_cast<uint8_t>(H[i] >> 8);
        result[4*i + 3] = static_cast<uint8_t>(H[i]);
    }
    return result;
}

int main() {
    SM3 sm3;
    std::string original = "userid=1234";
    auto originalHash = sm3.hash(std::vector<uint8_t>(original.begin(), original.end()));
    auto forgedHash = lengthExtensionAttack(originalHash, original.size(), forgeExtension);

    std::cout << "Original Hash: ";
    for (auto c : originalHash) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    std::cout << "\nForged Hash:   ";
    for (auto c : forgedHash) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    std::cout << std::endl;
}
