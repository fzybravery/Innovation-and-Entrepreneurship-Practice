#include "sm3.h"
#include <iostream>
#include <cassert>

/*
 * SM3 算法单元测试
 */
int main() {
    // 测试向量 1: "abc"
    std::string input1    = "abc";
    std::string expected1 = "66C7F0F462EEEDD9D1F2D46BDC10E4E24167C4875CF2F7A2297DA02B8F4BA8E0";
    std::string output1   = SM3::hashHex(input1);

    std::cout << "Test 1 - Input: " << input1 << "\n"
              << "Expected: "  << expected1 << "\n"
              << "Output:   "  << output1 << "\n\n";
    assert(output1 == expected1);

    // 测试向量 2: "test sm3 hash"
    std::string input2  = "test sm3 hash";
    std::string output2 = SM3::hashHex(input2);

    std::cout << "Test 2 - Input: " << input2 << "\n"
              << "Output:   " << output2 << "\n\n";
    // 仅展示结果，无断言

    std::cout << "所有测试通过！" << std::endl;
    return 0;
}
