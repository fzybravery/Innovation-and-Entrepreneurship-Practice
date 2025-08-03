#include "sm3.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>

// Merkle Tree 节点
struct MerkleNode {
    std::vector<uint8_t> hash;
    MerkleNode* left = nullptr;
    MerkleNode* right = nullptr;
};

// 计算哈希
std::vector<uint8_t> calcHash(const std::string& data) {
    SM3 sm3;
    return sm3.hash(std::vector<uint8_t>(data.begin(), data.end()));
}

// 合并两个哈希
std::vector<uint8_t> mergeHash(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    std::vector<uint8_t> concat = a;
    concat.insert(concat.end(), b.begin(), b.end());
    return calcHash(std::string(concat.begin(), concat.end()));
}

// 构造 Merkle 树
MerkleNode* buildMerkleTree(std::vector<MerkleNode*>& leaves) {
    std::vector<MerkleNode*> current = leaves;
    while (current.size() > 1) {
        std::vector<MerkleNode*> next;
        for (size_t i = 0; i < current.size(); i += 2) {
            MerkleNode* left = current[i];
            MerkleNode* right = (i + 1 < current.size()) ? current[i + 1] : nullptr;
            MerkleNode* parent = new MerkleNode;
            parent->left = left;
            parent->right = right;
            if (right)
                parent->hash = mergeHash(left->hash, right->hash);
            else
                parent->hash = mergeHash(left->hash, left->hash);
            next.push_back(parent);
        }
        current = next;
    }
    return current.front();
}

// 构造包含证明
void generateInclusionProof(MerkleNode* node, const std::vector<uint8_t>& targetHash, std::vector<std::vector<uint8_t>>& proof) {
    if (!node->left && !node->right)
        return;

    if (node->left && node->left->hash == targetHash) {
        if (node->right) proof.push_back(node->right->hash);
    } else if (node->right && node->right->hash == targetHash) {
        if (node->left) proof.push_back(node->left->hash);
    } else {
        if (node->left && node->left->hash != targetHash) generateInclusionProof(node->left, targetHash, proof);
        if (node->right && node->right->hash != targetHash) generateInclusionProof(node->right, targetHash, proof);
    }
}

// 主函数
int main() {
    const size_t N = 100000;
    std::vector<MerkleNode*> leaves;
    for (size_t i = 0; i < N; ++i) {
        MerkleNode* leaf = new MerkleNode;
        std::string data = "leaf_" + std::to_string(i);
        leaf->hash = calcHash(data);
        leaves.push_back(leaf);
    }

    MerkleNode* root = buildMerkleTree(leaves);
    std::cout << "Merkle Root: ";
    for (auto c : root->hash) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    std::cout << std::endl;

    // 选择目标叶子
    size_t targetIdx = 12345;
    std::vector<uint8_t> targetHash = leaves[targetIdx]->hash;
    std::vector<std::vector<uint8_t>> proof;
    generateInclusionProof(root, targetHash, proof);

    std::cout << "Proof path for leaf_" << targetIdx << ":\n";
    for (const auto& p : proof) {
        for (auto c : p) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        std::cout << "\n";
    }

    // Non-inclusion 证明可通过 hash 比较验证不存在（RFC6962 形式使用 audit path + neighbor hashes）
}
