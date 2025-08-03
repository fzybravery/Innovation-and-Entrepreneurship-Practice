#include "sm3_unrolled.h"
#include <cstring>
#include <sstream>
#include <iomanip>

using std::vector;
using std::string;

// 填充函数与前面完全相同
void SM3_UNROLLED::pad(const vector<uint8_t>& data, vector<uint8_t>& out){
    size_t L = data.size();
    uint64_t bitLen = uint64_t(L)*8;
    out.reserve(((L+9+63)/64)*64);
    out.insert(out.end(), data.begin(), data.end());
    out.push_back(0x80);
    size_t zeros = (56 - (L+1)%64 +64)%64;
    out.insert(out.end(), zeros, 0x00);
    for(int i=7;i>=0;--i) out.push_back(uint8_t(bitLen>>(i*8)));
}

// 首先定义一个宏，完成单轮 SM3 压缩
#define SM3_ROUND(i, A, B, C, D, E, F, G, H)                    \
{                                                               \
    uint32_t T = Tj(i);                                         \
    uint32_t SS1 = rotl(rotl(A,12) + E + rotl(T,i), 7);          \
    uint32_t SS2 = SS1 ^ rotl(A,12);                            \
    uint32_t TT1 = FF(A,B,C,i) + D + SS2 + W1[i];                \
    uint32_t TT2 = GG(E,F,G,i) + H + SS1 + W[i];                 \
    D = C; C = rotl(B,9); B = A; A = TT1;                       \
    H = G; G = rotl(F,19); F = E; E = P0(TT2);                   \
}

void SM3_UNROLLED::compress(uint32_t H[8], const uint8_t block[64]){
    uint32_t W[68], W1[64];
    // 1. W[0..15]
    for(int j=0;j<16;j++){
        W[j] = (uint32_t(block[4*j])<<24)
             | (uint32_t(block[4*j+1])<<16)
             | (uint32_t(block[4*j+2])<<8)
             |  uint32_t(block[4*j+3]);
    }
    // 2. W[16..67], W1
    for(int j=16;j<68;j++){
        W[j] = P1(W[j-16]^W[j-9]^rotl(W[j-3],15))
               ^ rotl(W[j-13],7) ^ W[j-6];
    }
    for(int j=0;j<64;j++){
        W1[j] = W[j] ^ W[j+4];
    }

    // 3. 展开 64 轮
    uint32_t A=H[0],B=H[1],C=H[2],D=H[3];
    uint32_t E=H[4],F=H[5],G=H[6],Ht=H[7];
    SM3_ROUND( 0,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 1,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 2,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 3,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 4,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 5,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 6,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 7,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 8,A,B,C,D,E,F,G,Ht);
    SM3_ROUND( 9,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(10,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(11,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(12,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(13,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(14,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(15,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(16,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(17,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(18,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(19,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(20,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(21,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(22,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(23,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(24,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(25,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(26,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(27,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(28,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(29,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(30,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(31,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(32,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(33,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(34,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(35,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(36,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(37,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(38,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(39,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(40,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(41,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(42,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(43,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(44,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(45,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(46,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(47,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(48,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(49,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(50,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(51,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(52,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(53,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(54,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(55,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(56,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(57,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(58,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(59,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(60,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(61,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(62,A,B,C,D,E,F,G,Ht);
    SM3_ROUND(63,A,B,C,D,E,F,G,Ht);

    // 4. 更新 IV
    H[0]^=A; H[1]^=B; H[2]^=C; H[3]^=D;
    H[4]^=E; H[5]^=F; H[6]^=G; H[7]^=Ht;
}

// 对外接口
vector<uint8_t> SM3_UNROLLED::hash(const vector<uint8_t>& data){
    vector<uint8_t> padded;
    pad(data,padded);
    uint32_t H[8]; std::memcpy(H,IV,sizeof(H));
    size_t blocks=padded.size()/64;
    for(size_t i=0;i<blocks;i++)
        compress(H,padded.data()+i*64);
    vector<uint8_t> out(32);
    for(int i=0;i<8;i++){
        out[4*i]   =uint8_t(H[i]>>24);
        out[4*i+1] =uint8_t(H[i]>>16);
        out[4*i+2] =uint8_t(H[i]>>8);
        out[4*i+3] =uint8_t(H[i]);
    }
    return out;
}

string SM3_UNROLLED::hashHex(const string& input){
    vector<uint8_t> data(input.begin(),input.end());
    auto dg=hash(data);
    std::ostringstream oss;
    oss<<std::uppercase<<std::hex<<std::setfill('0');
    for(auto b:dg) oss<<std::setw(2)<<int(b);
    return oss.str();
}
