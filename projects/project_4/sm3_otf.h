#ifndef SM3_OTF_H
#define SM3_OTF_H

#include <cstdint>
#include <vector>
#include <string>

class SM3_OTF {
public:
    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data);
    static std::string         hashHex(const std::string& input);

private:
    static constexpr uint32_t IV[8] = {
        0x7380166f,0x4914b2b9,0x172442d7,0xda8a0600,
        0xa96f30bc,0x163138aa,0xe38dee4d,0xb0fb0e4e
    };

    static inline uint32_t rotl(uint32_t x,int n){
        return (x<<n)|(x>>(32-n));
    }
    static inline uint32_t Tj(int j){
        return (j<16?0x79cc4519u:0x7a879d8au);
    }
    static inline uint32_t FF(uint32_t x,uint32_t y,uint32_t z,int j){
        return (j<16?x^y^z:(x&y)|(x&z)|(y&z));
    }
    static inline uint32_t GG(uint32_t x,uint32_t y,uint32_t z,int j){
        return (j<16?x^y^z:(x&y)|((~x)&z));
    }
    static inline uint32_t P0(uint32_t x){
        return x^rotl(x,9)^rotl(x,17);
    }
    static inline uint32_t P1(uint32_t x){
        return x^rotl(x,15)^rotl(x,23);
    }

    static void pad(const std::vector<uint8_t>& data,std::vector<uint8_t>& out);
    static void compress(uint32_t H[8],const uint8_t block[64]);
};

#endif // SM3_OTF_H
