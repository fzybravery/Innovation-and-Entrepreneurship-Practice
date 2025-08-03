#include "sm3_otf.h"
#include <cstring>
#include <sstream>
#include <iomanip>

using std::vector;
using std::string;

// 填充同前
void SM3_OTF::pad(const vector<uint8_t>& data, vector<uint8_t>& out){
    size_t L=data.size(); uint64_t bitLen=uint64_t(L)*8;
    out.reserve(((L+9+63)/64)*64);
    out.insert(out.end(), data.begin(), data.end());
    out.push_back(0x80);
    size_t zeros=(56-(L+1)%64+64)%64;
    out.insert(out.end(), zeros, 0x00);
    for(int i=7;i>=0;--i) out.push_back(uint8_t(bitLen>>(i*8)));
}

// on‐the‐fly：16 深度环形缓冲
void SM3_OTF::compress(uint32_t H[8], const uint8_t block[64]){
    uint32_t Wbuf[16];
    // 先装入 W[0..15]
    for(int i=0;i<16;i++){
        Wbuf[i] = (uint32_t(block[4*i])<<24)
                | (uint32_t(block[4*i+1])<<16)
                | (uint32_t(block[4*i+2])<<8)
                |  uint32_t(block[4*i+3]);
    }

    uint32_t A=H[0],B=H[1],C=H[2],D=H[3];
    uint32_t E=H[4],F=H[5],G=H[6],Ht=H[7];

    for(int j=0;j<64;j++){
        // 1) 计算 W[j]
        uint32_t Wj;
        if(j<16){
            Wj = Wbuf[j];
        } else {
            uint32_t X = Wbuf[(j-16)&15] ^ Wbuf[(j-9)&15] ^ rotl(Wbuf[(j-3)&15],15);
            Wj = P1(X) ^ rotl(Wbuf[(j-13)&15],7) ^ Wbuf[(j-6)&15];
            Wbuf[j&15] = Wj;
        }
        // 2) 计算 W1[j]
        uint32_t W1j = Wj ^ Wbuf[(j+4)&15];

        // 3) 一轮压缩
        uint32_t SS1 = rotl(rotl(A,12) + E + rotl(Tj(j),j),7);
        uint32_t SS2 = SS1 ^ rotl(A,12);
        uint32_t TT1 = FF(A,B,C,j) + D + SS2 + W1j;
        uint32_t TT2 = GG(E,F,G,j) + Ht + SS1 + Wj;

        D=C; C=rotl(B,9); B=A; A=TT1;
        Ht=G; G=rotl(F,19); F=E; E=P0(TT2);
    }

    // xor 初始向量
    H[0]^=A; H[1]^=B; H[2]^=C; H[3]^=D;
    H[4]^=E; H[5]^=F; H[6]^=G; H[7]^=Ht;
}

// 对外接口同前
vector<uint8_t> SM3_OTF::hash(const vector<uint8_t>& data){
    vector<uint8_t> buf; pad(data,buf);
    uint32_t H[8]; std::memcpy(H,IV,sizeof(H));
    for(size_t i=0;i<buf.size()/64;i++)
        compress(H,buf.data()+i*64);
    vector<uint8_t> out(32);
    for(int i=0;i<8;i++){
        out[4*i]   = uint8_t(H[i]>>24);
        out[4*i+1] = uint8_t(H[i]>>16);
        out[4*i+2] = uint8_t(H[i]>>8);
        out[4*i+3] = uint8_t(H[i]);
    }
    return out;
}

string SM3_OTF::hashHex(const string& input){
    vector<uint8_t> d(input.begin(),input.end());
    auto dg=hash(d);
    std::ostringstream oss;
    oss<<std::uppercase<<std::hex<<std::setfill('0');
    for(auto b:dg) oss<<std::setw(2)<<int(b);
    return oss.str();
}
