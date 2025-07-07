#include <iostream>
#include <vector>
#include <numeric> 

class SM4 {
private:
    std::vector<unsigned int> key_r; // 改为 unsigned int

    // S-box parameters (using int for simplicity, but could be unsigned char)
    const int SBOX[256] = {
        0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2, 0x28, 0xFB, 0x2C, 0x05,
        0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3, 0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
        0x9C, 0x42, 0x50, 0xF4, 0x91, 0xEF, 0x98, 0x7A, 0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62,
        0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95, 0x80, 0xDF, 0x94, 0xFA, 0x75, 0x8F, 0x3F, 0xA6,
        0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA, 0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8,
        0x68, 0x6B, 0x81, 0xB2, 0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35,
        0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B, 0x01, 0x21, 0x78, 0x87,
        0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52, 0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E,
        0xEA, 0xBF, 0x8A, 0xD2, 0x40, 0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1,
        0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55, 0xAD, 0x93, 0x32, 0x30, 0xF5, 0x8C, 0xB1, 0xE3,
        0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60, 0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F,
        0xD5, 0xDB, 0x37, 0x45, 0xDE, 0xFD, 0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51,
        0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41, 0x1F, 0x10, 0x5A, 0xD8,
        0x0A, 0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD, 0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0,
        0x89, 0x69, 0x97, 0x4A, 0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84,
        0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20, 0x79, 0xEE, 0x5F, 0x3E, 0xD7, 0xCB, 0x39, 0x48
    };

    // FK 和 CK 数组中的值是无符号的，应使用 unsigned int
    const unsigned int FK[4] = {0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc}; // 改为 unsigned int
    const unsigned int CK[32] = { // 改为 unsigned int
        0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
        0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
        0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
        0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
        0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
        0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
        0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
        0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
    };

    /* 将 input 左移 n 位 (循环左移) */
    static unsigned int shift(unsigned int input, int n) {
        return (input << n) | (input >> (32 - n));
    }

    /* 将32比特数拆分成4个8比特数 */
    static std::vector<unsigned char> splitInt(unsigned int n) {
        std::vector<unsigned char> bytes(4);
        bytes[0] = (unsigned char)(n >> 24);
        bytes[1] = (unsigned char)(n >> 16);
        bytes[2] = (unsigned char)(n >> 8);
        bytes[3] = (unsigned char)n;
        return bytes;
    }

    /* 将4个8比特数合并成32比特数 */
    static unsigned int jointBytes(unsigned char byte_0, unsigned char byte_1, unsigned char byte_2, unsigned char byte_3) {
        return ((unsigned int)byte_0 << 24) | ((unsigned int)byte_1 << 16) | ((unsigned int)byte_2 << 8) | (unsigned int)byte_3;
    }

    /* S盒变换 */
    unsigned int sBox(unsigned int box_input) {
        std::vector<unsigned char> temp = splitInt(box_input);
        std::vector<unsigned char> output_bytes(4);

        for (int i = 0; i < 4; ++i) {
            output_bytes[i] = (unsigned char)SBOX[temp[i]];
        }
        return jointBytes(output_bytes[0], output_bytes[1], output_bytes[2], output_bytes[3]);
    }

    /* 密钥拓展 */
    std::vector<unsigned int> keyGenerate(const std::vector<unsigned char>& key) { // 返回类型改为 unsigned int
        std::vector<unsigned int> key_r(32); // 轮密钥rk_i // 改为 unsigned int
        std::vector<unsigned int> key_temp(4); // 改为 unsigned int

        // 将输入的密钥每32比特合并，并异或FK
        for (int i = 0; i < 4; ++i) {
            key_temp[i] = jointBytes(key[4 * i], key[4 * i + 1], key[4 * i + 2], key[4 * i + 3]);
            key_temp[i] ^= FK[i];
        }

        // 32轮密钥拓展
        for (int i = 0; i < 32; ++i) {
            unsigned int box_in = key_temp[1] ^ key_temp[2] ^ key_temp[3] ^ CK[i];
            unsigned int box_out = sBox(box_in);
            key_r[i] = key_temp[0] ^ box_out ^ shift(box_out, 13) ^ shift(box_out, 23);
            key_temp[0] = key_temp[1];
            key_temp[1] = key_temp[2];
            key_temp[2] = key_temp[3];
            key_temp[3] = key_r[i];
        }
        return key_r;
    }

    /* 加解密主模块 */
    std::vector<unsigned char> sm4Main(const std::vector<unsigned char>& input, int mod) {
        std::vector<unsigned int> text(4); // 32比特字 // 改为 unsigned int

        // 将输入以32比特分组
        for (int i = 0; i < 4; ++i) {
            text[i] = jointBytes(input[4 * i], input[4 * i + 1], input[4 * i + 2], input[4 * i + 3]);
        }

        for (int i = 0; i < 32; ++i) {
            int index = (mod == 0) ? i : (31 - i); // 通过改变key_r的顺序改变模式
            unsigned int box_input = text[1] ^ text[2] ^ text[3] ^ key_r[index];
            unsigned int box_output = sBox(box_input);
            unsigned int temp = text[0] ^ box_output ^ shift(box_output, 2) ^ shift(box_output, 10) ^ shift(box_output, 18) ^ shift(box_output, 24);
            text[0] = text[1];
            text[1] = text[2];
            text[2] = text[3];
            text[3] = temp;
        }

        std::vector<unsigned char> output(16); // 输出
        // 将结果的32比特字拆分
        for (int i = 0; i < 4; ++i) {
            std::vector<unsigned char> split_bytes = splitInt(text[3 - i]);
            for (int j = 0; j < 4; ++j) {
                output[4 * i + j] = split_bytes[j];
            }
        }
        return output;
    }

public:
    /* 初始化轮密钥 */
    SM4(const std::vector<unsigned char>& key) {
        this->key_r = keyGenerate(key);
    }

    /* 加密 */
    std::vector<unsigned char> encrypt(const std::vector<unsigned char>& plaintext) {
        return sm4Main(plaintext, 0);
    }

    /* 解密 */
    std::vector<unsigned char> decrypt(const std::vector<unsigned char>& ciphertext) {
        return sm4Main(ciphertext, 1);
    }
};

// Example Usage (main function for testing)
int main() {
    // Example Key (16 bytes)
    std::vector<unsigned char> key = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    // Example Plaintext (16 bytes)
    std::vector<unsigned char> plaintext = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    SM4 sm4_cipher(key);

    // Encrypt
    std::vector<unsigned char> ciphertext = sm4_cipher.encrypt(plaintext);
    std::cout << "Ciphertext: ";
    for (unsigned char b : ciphertext) {
        std::cout << std::hex << (int)b << " ";
    }
    std::cout << std::dec << std::endl;

    // Decrypt
    std::vector<unsigned char> decrypted_text = sm4_cipher.decrypt(ciphertext);
    std::cout << "Decrypted:  ";
    for (unsigned char b : decrypted_text) {
        std::cout << std::hex << (int)b << " ";
    }
    std::cout << std::dec << std::endl;

    return 0;
}