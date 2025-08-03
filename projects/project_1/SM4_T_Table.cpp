#include <iostream>
#include <vector>
#include <numeric>

class OptimizedSM4 {
private:
    std::vector<unsigned int> key_r;
    static unsigned int T_TABLE[256];

    // S-box and other constants remain the same
    // ... (Your original SBOX, FK, CK arrays here)

    // Function to calculate and build the T-table
    static void buildTTable() {
        if (T_TABLE[0] == 0) { // Check if the table is already built
            for (int i = 0; i < 256; ++i) {
                unsigned int sbox_out = SBOX[i];
                unsigned int temp = sbox_out ^ shift(sbox_out, 2) ^ shift(sbox_out, 10) ^ shift(sbox_out, 18) ^ shift(sbox_out, 24);
                T_TABLE[i] = temp;
            }
        }
    }

    // New optimized round function
    unsigned int optimizedRound(unsigned int x0, unsigned int x1, unsigned int x2, unsigned int x3, unsigned int rk) {
        // T-table replaces S-box and L-box in one go
        unsigned int temp = (T_TABLE[x1 >> 24] ^ T_TABLE[(x1 >> 16) & 0xFF] ^ T_TABLE[(x1 >> 8) & 0xFF] ^ T_TABLE[x1 & 0xFF]) ^ rk;
        return x0 ^ temp;
    }

    /* 加解密主模块 (Optimized) */
    std::vector<unsigned char> sm4Main(const std::vector<unsigned char>& input, int mod) {
        std::vector<unsigned int> text(4);
        for (int i = 0; i < 4; ++i) {
            text[i] = jointBytes(input[4 * i], input[4 * i + 1], input[4 * i + 2], input[4 * i + 3]);
        }

        for (int i = 0; i < 32; ++i) {
            int index = (mod == 0) ? i : (31 - i);
            unsigned int temp = optimizedRound(text[0], text[1], text[2], text[3], key_r[index]);
            text[0] = text[1];
            text[1] = text[2];
            text[2] = text[3];
            text[3] = temp;
        }

        // Final reverse order
        std::vector<unsigned char> output(16);
        for (int i = 0; i < 4; ++i) {
            std::vector<unsigned char> split_bytes = splitInt(text[3 - i]);
            for (int j = 0; j < 4; ++j) {
                output[4 * i + j] = split_bytes[j];
            }
        }
        return output;
    }

public:
    OptimizedSM4(const std::vector<unsigned char>& key) {
        buildTTable(); // Ensure T-table is built
        this->key_r = keyGenerate(key);
    }
    // Encrypt and Decrypt methods remain the same
    // ...
};

unsigned int OptimizedSM4::T_TABLE[256] = {0}; // Static initialization
