
# SM4 加密算法 C++ 实现说明文档

## 简介

本项目实现了国家商用密码 SM4 分组加密算法的 C++ 版本。SM4 是由中国国家密码管理局发布的国密标准，广泛用于无线局域网、数据加密与安全通信等领域。

本实现参考了博客园 kentle 博主的 SM4 算法原理说明及 Java 版本实现，核心包括：
- S盒替换（S-Box）
- 密钥扩展（Key Schedule）
- 32轮迭代加解密
- 支持 ECB 模式下的加密与解密（不含分组填充）

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
sm4_sbox.cpp文档中对S盒进行了优化，显著减少了小对象创建（vector 的堆分配）；不必要的字节拆分与合并；提高内存局部性和指令级并行。
