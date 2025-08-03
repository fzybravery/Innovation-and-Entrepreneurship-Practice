# SM4加密算法C++实现详解报告


## 一、引言

### 1.1 算法背景
SM4加密算法是中国国家密码管理局发布的商用分组密码标准（GM/T 0002-2012），前身为SMS4算法，主要应用于无线局域网、数据加密与安全通信等领域。作为对称加密算法，其核心参数为：
- 分组长度：128位（16字节）
- 密钥长度：128位（16字节）
- 迭代轮数：32轮

该算法设计兼顾安全性与效率，目前已成为国内信息安全领域的核心加密标准之一。

### 1.2 实现目标
本项目提供SM4算法的C++完整实现及多种优化版本，旨在：
- 清晰展示SM4算法的数学原理与执行流程；
- 提供可直接运行的参考代码，支持128位数据的加解密操作；
- 通过多维度优化（S盒加速、T表预计算、SIMD并行）提升运行效率；
- 为教学研究与工程应用提供标准化实现参考。


## 二、SM4算法数学原理

### 2.1 核心参数定义
| 参数         | 规格                          | 说明                                  |
|--------------|-------------------------------|---------------------------------------|
| 明文/密文    | 128位（16字节）               | 固定分组长度，需填充机制支持长数据加密 |
| 密钥         | 128位（16字节）               | 主密钥，用于生成轮密钥                |
| 轮密钥       | 32个×32位                     | 由主密钥扩展生成，每轮迭代使用1个     |
| 迭代轮数     | 32轮                          | 固定轮数，确保加密强度                |
| 算法常数     | FK（4个32位）、CK（32个32位） | 用于密钥扩展的固定常数                |


### 2.2 密钥扩展（Key Expansion）

密钥扩展的核心是从128位主密钥生成32个32位轮密钥（$rk_0 \sim rk_{31}$），流程如下：

#### 2.2.1 初始变换
128位主密钥$MK$拆分为4个32位字：$MK = (MK_0, MK_1, MK_2, MK_3)$，与FK常数异或生成初始密钥状态：  
$$k_i = MK_i \oplus FK_i \quad (i=0,1,2,3)$$  

其中FK常数定义为：  
$FK = (0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc)$

#### 2.2.2 轮密钥生成
对$i=0$到$31$，轮密钥$rk_i$通过以下公式生成：  
$$rk_i = k_{i+4} = k_i \oplus T'(k_{i+1} \oplus k_{i+2} \oplus k_{i+3} \oplus CK_i)$$  

- $CK_i$为第$i$个轮常数（32个，定义见代码实现）；  
- $T'$为密钥扩展变换函数：  
  $T'(a) = S(a) \oplus (S(a) \ll 13) \oplus (S(a) \ll 23)$，其中：  
  - $S(a)$：对$a$的4个字节分别应用S盒变换（非线性置换）；  
  - $\ll n$：32位循环左移$n$位。


### 2.3 加解密轮函数

SM4加解密流程一致，仅轮密钥使用顺序相反（加密用$rk_0 \sim rk_{31}$，解密用$rk_{31} \sim rk_0$）。

#### 2.3.1 数据分组
128位明文（或密文）$X$拆分为4个32位字：$X = (X_0, X_1, X_2, X_3)$。

#### 2.3.2 轮迭代
对$i=0$到$31$，第$i$轮迭代公式为：  
$$X_{i+4} = F(X_i, X_{i+1}, X_{i+2}, X_{i+3}, rk_i)$$  

轮函数$F$定义为：  
$$F(a, b, c, d, rk) = a \oplus T(b \oplus c \oplus d \oplus rk)$$  

其中$T$为数据变换函数：  
- $T(x) = L(S(x))$  
  - $S(x)$：S盒变换（对$x$的4个字节分别置换）；  
  - $L(y)$：线性变换，公式为：  
    $L(y) = y \oplus (y \ll 2) \oplus (y \ll 10) \oplus (y \ll 18) \oplus (y \ll 24)$

#### 2.3.3 最终输出
32轮迭代后得到$(X_{32}, X_{33}, X_{34}, X_{35})$，密文（或明文）为：  
$$(X_{35}, X_{34}, X_{33}, X_{32})$$  


### 2.4 S盒与线性变换
- **S盒**：8位输入到8位输出的非线性置换表（256个值），基于有限域$GF(2^8)$逆变换设计，提供核心混淆能力；
- **线性变换$L$**：通过多轮循环左移与异或操作，将S盒输出的非线性结果扩散到整个32位字，增强雪崩效应（输入微小变化导致输出显著变化）。


## 算法流程简述

SM4 属于 **对称加密算法**，处理的数据为 **128位明文和128位密钥**，执行 **32轮迭代加解密**，其流程主要包括：

1. **密钥扩展（Key Expansion）**  
   使用给定的128位密钥生成32个轮密钥（每轮32位）。

2. **加解密主过程（Encryption/Decryption）**  
   对128位明文（或密文）分为4组32位字，依次执行 32 轮数据变换。

3. **S盒变换（SubBytes）**  
   使用固定的 SBOX，对输入字节进行非线性替换。

4. **循环移位和线性变换**  
   使用移位+异或的方式混淆数据，增强加密强度。

## 使用示例（main 函数）

```cpp
int main() {
    // 128位密钥
    std::vector<unsigned char> key = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    // 128位明文
    std::vector<unsigned char> plaintext = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    SM4 sm4_cipher(key);

    // 加密
    std::vector<unsigned char> ciphertext = sm4_cipher.encrypt(plaintext);
    std::cout << "Ciphertext: ";
    for (unsigned char b : ciphertext) {
        std::cout << std::hex << (int)b << " ";
    }

    // 解密
    std::vector<unsigned char> decrypted_text = sm4_cipher.decrypt(ciphertext);
    std::cout << "\nDecrypted:  ";
    for (unsigned char b : decrypted_text) {
        std::cout << std::hex << (int)b << " ";
    }

    return 0;
}
```

## 核心类说明：`SM4`

### 构造函数

```cpp
SM4(const std::vector<unsigned char>& key);
```
- 输入长度：16 字节密钥
- 自动进行轮密钥拓展

### 加密函数

```cpp
std::vector<unsigned char> encrypt(const std::vector<unsigned char>& plaintext);
```
- 输入长度：16 字节明文
- 输出长度：16 字节密文

### 解密函数

```cpp
std::vector<unsigned char> decrypt(const std::vector<unsigned char>& ciphertext);
```
- 输入长度：16 字节密文
- 输出长度：16 字节明文

## 文件结构

| 文件        | 说明                         |
|-------------|------------------------------|
| `main()`    | 示例代码，展示加解密流程     |
| `SM4` 类    | 加解密核心实现               |
| `SBOX`      | 固定 S 盒（256字节查找表）    |
| `FK/CK`     | 算法常数，来自 SM4 标准      |

## 参考资料

- 博客园 kentle：[SM4加密算法原理和简单实现（Java）](https://www.cnblogs.com/kentle/p/14135865.html)
- 国家商用密码标准 GM/T 0002-2012
- 百度百科 - [SM4](https://baike.baidu.com/item/SM4)

## 编译说明

支持任意 C++11 及以上版本编译器：

```bash
g++ -std=c++11 sm4.cpp -o sm4
./sm4
```

## 注意事项

- 本实现为教学用途，未包含完整的填充机制（如 PKCS#7）
- ECB 模式无安全随机性，实际使用请结合 CBC 等模式与 IV
- 可扩展性较好，建议添加文件加密、分组填充与MAC等功能

## 算法优化
- sm4_sbox.cpp文档中对S盒进行了优化，显著减少了小对象创建（vector 的堆分配）；不必要的字节拆分与合并；提高内存局部性和指令级并行。
- T 表优化（OptimizedSM4类）原理：预计算T_TABLE，合并 S 盒与线性变换L，减少每轮计算量。
```cpp
static void buildTTable() {
    if (T_TABLE[0] == 0) { 
        for (int i = 0; i < 256; ++i) {
            unsigned int sbox_out = SBOX[i];
            unsigned int temp = sbox_out ^ shift(sbox_out, 2) ^ shift(sbox_out, 10) ^ 
                               shift(sbox_out, 18) ^ shift(sbox_out, 24);
            T_TABLE[i] = temp;
        }
    }
}

// 优化轮函数
unsigned int optimizedRound(unsigned int x0, unsigned int x1, unsigned int x2, 
                           unsigned int x3, unsigned int rk) {
    unsigned int temp = (T_TABLE[x1 >> 24] ^ T_TABLE[(x1 >> 16) & 0xFF] ^ 
                       T_TABLE[(x1 >> 8) & 0xFF] ^ T_TABLE[x1 & 0xFF]) ^ rk;
    return x0 ^ temp;
}
```
- SIMD 优化: 基于 AVX 指令,利用 128 位 SIMD 寄存器（__m128i）并行处理数据，提升批量加密效率。
核心优化点：sboxTransform：通过PSHUFB指令并行完成 16 字节 S 盒变换；linearTransform：用VPROLD指令并行实现循环左移；
```cpp
void sm4_encrypt_block(__m128i* state, const __m128i rk[32]) {
    for (int i = 0; i < 32; ++i) {
        __m128i temp = _mm_xor_si128(_mm_xor_si128(state[1], state[2]), 
                                   _mm_xor_si128(state[3], rk[i]));
        __m128i sboxOut = sboxTransform(temp); // 并行S盒变换
        __m128i L = linearTransform(sboxOut); // 并行线性变换
        __m128i newX = _mm_xor_si128(state[0], L);
        // 并行更新状态
        state[0] = state[1];
        state[1] = state[2];
        state[2] = state[3];
        state[3] = newX;
    }
}
```
