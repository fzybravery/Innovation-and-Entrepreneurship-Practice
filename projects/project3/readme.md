# 基于 Circom 的 Poseidon2 哈希算法电路实现与 Groth16 证明技术报告


## 1. 项目概述

### 1.1. 项目目标

本项目旨在利用 Circom 语言设计和实现一个基于 **Poseidon2 哈希算法** 的算术电路。该电路的核心功能是生成一个零知识证明（Zero-Knowledge Proof, ZKP），以证明某个秘密输入（哈希原象 `preImage`）经过 Poseidon2 哈希运算后，能够得到一个公开的哈希值 `hash`，而在此过程中完全不泄露 `preImage` 的具体数值。

我们采用 **Groth16** 证明系统来生成和验证证明，并提供了完整的端到端实现流程，包括电路编译、可信设置、证据生成和链下验证。

### 1.2. 技术选型

* **零知识证明框架**: `Circom` & `snarkjs`
    * **Circom**: 一种用于编写算术电路的领域特定语言（DSL），非常适合定义 ZKP 中的数学约束。
    * **snarkjs**: 一个 JavaScript 库，提供了编译 Circom 代码、执行可信设置、生成和验证证明所需的全套工具。
* **哈希算法**: `Poseidon2`
    * Poseidon2 是为 ZK-SNARKs/STARKs 等零知识证明系统特别优化的哈希算法。相比于 SHA-256 等传统哈希函数，它在转换为算术电路时产生的约束（constraints）数量极少，从而大幅提高了证明生成的效率。
* **证明系统**: `Groth16`
    * Groth16 是目前最高效（证明尺寸最小、验证速度最快）的非通用性 zk-SNARK 协议之一，尽管它需要为每个电路进行特定的可信设置。

---

## 2. 核心技术背景

### 2.1. 零知识证明 (ZKP)

零知识证明允许一方（证明者 Prover）向另一方（验证者 Verifier）证明其知道某个秘密信息，而无需透露该秘密信息本身。本项目所用的 zk-SNARK (Zero-Knowledge Succinct Non-Interactive Argument of Knowledge) 是 ZKP 的一种，它生成的证明具有以下特点：
* **简洁性 (Succinct)**: 证明的尺寸非常小，远小于原始计算的尺寸。
* **非交互性 (Non-Interactive)**: 证明者只需向验证者发送一条消息即可完成证明。
* **知识论证 (Argument of Knowledge)**: 证明者必须真正“知道”这个秘密才能生成有效的证明。

### 2.2. 算术电路与 R1CS

在 ZKP 系统中，任何复杂的计算逻辑都必须被“拍平”并转换为一系列基础的数学方程式，这个过程称为**算术化 (Arithmetization)**。Circom 的作用就是将高级代码转换为一种称为 **R1CS (Rank-1 Constraint System)** 的中间表示。

一个 R1CS 约束的形式为：
$$
(\sum_{i=0}^{m} a_i \cdot w_i) \times (\sum_{i=0}^{m} b_i \cdot w_i) = (\sum_{i=0}^{m} c_i \cdot w_i)
$$
其中，$w_i$ 是电路中的“信号线”（witness），包括私有输入、公共输入和所有中间计算值。$a_i, b_i, c_i$ 是常数系数。本质上，任何复杂的计算都被分解成了大量的“二次”约束。电路的约束数量是衡量其复杂度和证明生成成本的关键指标。

### 2.3. ZK-Friendly 哈希函数的重要性

像 SHA-256 这样的传统哈希函数，其内部充满了位运算（如 XOR, AND, ROTATE），这些运算在有限域算术中表示起来非常“昂贵”，会产生巨量的 R1CS 约束。例如，一次 SHA-256 运算可能需要超过 20000 个约束。

而像 Poseidon 和 Poseidon2 这样的 **ZK-Friendly** 哈希函数，其设计完全基于有限域上的简单代数运算（加法、乘法和求幂），因此可以用极少的约束来表示，极大地降低了 ZKP 的开销。

---

## 3. Poseidon2 算法深度解析

Poseidon2 是对 Poseidon 算法的优化，其核心思想是在保持安全性的前提下，进一步减少证明生成中的计算开销。

### 3.1. Poseidon 算法结构

Poseidon 的核心是一个置换函数（Permutation），它在一个宽度为 `t` 的状态（state）上进行迭代运算。这个置换由多轮（rounds）组成，每一轮都包含三个步骤：
1.  **AddRoundConstants**: 将状态的每个元素与一个预定义的轮常数相加。
2.  **S-Box**: 对状态的每个（或部分）元素进行非线性变换，通常是求幂运算 $x^d$。这是保证哈希安全性的关键。
3.  **Linear Layer**: 将状态向量与一个 **MDS (Maximum Distance Separable) 矩阵**相乘，以确保状态元素之间充分混合。

为了优化性能，Poseidon 采用了 **全轮（Full rounds）** 和 **半轮（Partial rounds）** 的混合策略。
* **全轮**: S-Box 应用于状态的所有 `t` 个元素。
* **半轮**: S-Box 仅应用于状态的第一个元素。

### 3.2. Poseidon2 的核心优化

Poseidon2 的研究者发现，在 Poseidon 中，开销最大的部分是每一轮都要进行的 MDS 矩阵乘法。尽管这个操作对于安全性至关重要，但在电路中实现矩阵乘法会引入大量约束。

Poseidon2 的突破性优化在于，它用两个不同的、计算成本极低的线性层替换了原来单一的、昂贵的 MDS 矩阵。

* **外部矩阵 ($M_E$)**: 用于全轮。
* **内部矩阵 ($M_I$)**: 用于半轮。

对于我们项目中的参数 `t=3`，这两个矩阵及其高效计算方法如下：

#### 3.2.1. 矩阵结构与数学表示

* **内部矩阵 $M_I$**:
    $$
    M_I = \begin{pmatrix} 2 & 1 & 1 \\ 1 & 2 & 1 \\ 1 & 1 & 2 \end{pmatrix}
    $$
    对状态向量 $s = (s_0, s_1, s_2)^T$ 的乘法可以被优化。令 $s_{sum} = s_0 + s_1 + s_2$，则：
    $$
    M_I \cdot s = \begin{pmatrix} s_0 + s_{sum} \\ s_1 + s_{sum} \\ s_2 + s_{sum} \end{pmatrix}
    $$
    这个计算只需要 2 次加法（计算 $s_{sum}$）和 3 次加法（计算新状态），成本极低。

* **外部矩阵 $M_E$**:
    $$
    M_E = \begin{pmatrix} 6 & 5 & 5 \\ 5 & 6 & 5 \\ 5 & 5 & 6 \end{pmatrix}
    $$
    同样，令 $s_{sum} = s_0 + s_1 + s_2$，则：
    $$
    M_E \cdot s = \begin{pmatrix} 5 \cdot s_{sum} + s_0 \\ 5 \cdot s_{sum} + s_1 \\ 5 \cdot s_{sum} + s_2 \end{pmatrix}
    $$
    这个计算只需要 2 次加法，1 次与常数5的乘法，和 3 次加法。

这种优化极大地减少了线性层的约束数量，使得 Poseidon2 比原版 Poseidon 更快。

### 3.3. Poseidon2 置换流程

整个置换流程如下，其中 `R_F` 是全轮数，`R_P` 是半轮数：
1.  **初始全轮**: 执行 `R_F / 2` 次全轮。每一轮包含：`AddRoundConstants` -> `S-Box` -> `LinearLayer(M_E)`。
2.  **中间半轮**: 执行 `R_P` 次半轮。每一轮包含：`AddRoundConstants` -> `Partial S-Box` -> `LinearLayer(M_I)`。
3.  **最终全轮**: 执行 `R_F / 2` 次全轮。每一轮包含：`AddRoundConstants` -> `S-Box` -> `LinearLayer(M_E)`。

---

## 4. Circom 电路实现思路

我们将上述算法逻辑转化为 Circom 电路。

### 4.1. 模板化设计

* **`SBox()` 模板**: 这是一个基础组件，实现了非线性层 $out \leftarrow in^5$。通过中间信号 `in2` 和 `in4`，将一次五次幂运算分解为两次乘法，这在 R1CS 中是最高效的方式。
    ```circom
    template SBox() {
        signal input in;
        signal output out;
        signal in2 = in * in;
        signal in4 = in2 * in2;
        out <== in4 * in;
    }
    ```

* **`Poseidon2_t3()` 模板**: 这是实现置换函数的核心。
    1.  **状态初始化**: 输入 `in[0]` 被放置在 `state[1]`，而 `state[0]` 和 `state[2]` 被初始化为0。这是一种标准的填充（padding）方式，用于处理单个元素的输入。
    2.  **轮循环**: 一个 `for` 循环遍历所有 `R_F + R_P` 轮。
    3.  **条件化轮类型**: 在循环内部，通过 `var is_full_round = (r < R_F/2) || (r >= R_F/2 + R_P);` 来判断当前是全轮还是半轮。
    4.  **S-Box 应用**: 根据 `is_full_round` 的值，S-Box 被应用于所有状态元素或仅第一个元素。
    5.  **线性层实现**: 这是实现的关键。我们没有进行真正的矩阵乘法，而是直接使用了上一节推导出的高效计算公式。
        ```circom
        // ...
        var s_sum = state[0] + state[1] + state[2];
        if (is_full_round) {
            // 实现 M_E 矩阵乘法
            state[0] <== 5 * s_sum + s0;
            // ...
        } else {
            // 实现 M_I 矩阵乘法
            state[0] <== s_sum + s0;
            // ...
        }
        ```
        这种直接的代数表达式转换能生成最少的 R1CS 约束。

* **`Main()` 模板**: 这是电路的入口。它实例化 `Poseidon2_t3`，将私有输入 `preImage` 连接到哈希器，并添加最终的核心约束：
    ```circom
    hash === hasher.out;
    ```
    这个约束强制要求电路计算出的哈希结果必须与公开的哈希输入 `hash` 相等。如果证明者提供的 `preImage` 是错误的，这个约束将无法满足，witness 生成会失败。

---

## 5. Groth16 证明工作流

`scripts/run_groth16.sh` 脚本完整地展示了从电路到证明的生命周期。

1.  **编译 (Compilation)**:
    `circom circuits/poseidon2.circom --r1cs --wasm --sym`
    * `--r1cs`: 生成 R1CS 约束文件 (`.r1cs`)。
    * `--wasm`: 生成 WebAssembly 版本的 witness 计算器，速度比 JS 版本快得多。
    * `--sym`: 生成符号文件 (`.sym`)，用于调试。

2.  **可信设置 (Trusted Setup)**:
    这是 Groth16 的一个关键（也是有争议的）环节，它生成证明密钥（Proving Key）和验证密钥（Verification Key）。
    * **阶段一 (Powers of Tau)**: 这是一个通用的、与电路无关的仪式。我们从网络下载一个已经完成的 `pot14_final.ptau` 文件。这个文件包含了计算所需的“有毒废料”($\tau$)的加密幂次。**必须信任这个文件的生成过程是安全的**。
    * **阶段二 (Circuit-Specific)**:
        * `snarkjs groth16 setup`: 将电路的 R1CS 和 Powers of Tau 文件结合，生成初始证明密钥 `poseidon2_0000.zkey`。
        * `snarkjs zkey contribute`: 模拟多方计算（MPC）的贡献环节。在真实场景中，应由多个互不信任的参与方接力完成，只要有一方是诚实的，最终的密钥就是安全的。这里我们用一个随机熵源简单模拟。
        * `snarkjs zkey export verificationkey`: 从最终的证明密钥中提取出验证密钥 `verification_key.json`。**证明密钥必须严格保密**，而验证密钥可以公开发布。

3.  **计算 Witness**:
    `node generate_witness.js ... input.json witness.wtns`
    * Prover 提供私有输入 `preImage`（在 `input.json` 中）。
    * WASM 计算器执行电路中的所有计算，为电路中的每一个信号线（包括中间值和最终的 `hash` 输出）生成一个具体的值。
    * 所有这些值的集合就是 **witness**，保存在 `witness.wtns` 文件中。

4.  **生成证明 (Proof Generation)**:
    `snarkjs groth16 prove ... witness.wtns proof.json public.json`
    * Prover 使用 **证明密钥** (`.zkey`) 和 **witness** (`.wtns`) 来生成一个非常小巧的证明文件 (`proof.json`)。
    * 同时，所有公开信号（这里是 `hash`）的值被提取到 `public.json` 中。

5.  **验证证明 (Proof Verification)**:
    `snarkjs groth16 verify verification_key.json public.json proof.json`
    * Verifier 使用公开的 **验证密钥**、**公开输入**和 **证明**。
    * `verify` 函数执行一系列数学运算（主要是配对检查）。如果返回 `OK`，则证明有效，意味着 Verifier 可以确信：Prover 知道一个 `preImage`，其哈希值等于 `public.json` 中给出的 `hash` 值。

---

## 6. 结论与展望

本项目成功地使用 Circom 实现了一个高效的 Poseidon2 哈希函数电路，并围绕它构建了完整的 Groth16 证明和验证流程。通过单元测试确保了电路逻辑的正确性，并通过脚本自动化了端到端的 ZKP 工作流。

**未来工作**:
* **使用官方参数**: 本项目中的轮常数是为演示目的生成的。在生产环境中，必须使用由官方工具或安全仪式生成的标准轮常数和 MDS 矩阵。
* **多输入哈希**: 当前电路只处理单个输入。可以将其扩展为一个可吸收多个输入的 sponge 结构，以支持对任意长度消息的哈希。
* **链上验证**: `snarkjs` 可以导出一个 Solidity 验证器合约，允许将验证过程部署到以太坊等智能合约平台，实现链上 ZKP 验证。
* **更换证明系统**: 可以尝试将电路与其它证明系统（如 PLONK 或 Marlin）结合，它们具有不同的特性（如通用的可信设置）。
