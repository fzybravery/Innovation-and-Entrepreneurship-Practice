# 项目简介
本项目使用 Circom 语言实现 Poseidon2 哈希算法的零知识证明电路。Poseidon2 是为 ZK-friendly 场景优化的哈希算法，比原版 Poseidon 更高效。
电路的功能是证明你知道一个隐私的数字 preImage，使其经过 Poseidon2 哈希后能得到一个公开的哈希值 hash，而无需暴露 preImage 本身。

# 项目特点
- 算法实现：完整实现了 Poseidon2 的置换层，包括外部和内部轮函数、轮常数加法、S-Box 和高效的线性层。
- 参数配置： t=3, d=5, R_F=8, R_P=22。
- 测试驱动：包含了使用 circom_tester 的单元测试，确保电路逻辑与JS参考实现一致。
- 端到端流程：提供了从电路编译、可信设置、证据生成到链下验证的完整 Groth16 流程脚本。

# 项目结构说明
```
project3/
├── circuits/
│   └── poseidon2.circom          # Circom 电路源代码
├── test/
│   └── poseidon2.test.js         # 电路单元测试脚本
├── scripts/
│   └── run_groth16.sh            # Groth16 证明生成与验证脚本
├── package.json                  # Node.js 项目配置文件
└── README.md                     # 项目说明文档
```

# 运行和测试

在开始之前，请确保您已经安装了 Node.js 和 circom & snarkjs。
- 步骤 1: 安装依赖
在项目根目录下打开终端，运行以下命令：
```bash
npm install
```
- 步骤 2: 运行单元测试
执行以下命令来运行 mocha 测试。这个命令会编译 poseidon2.circom 电路，并用 poseidon2.test.js 中的逻辑来验证它。
```bash
npm test
```
如果一切顺利，您应该会看到测试通过的输出，表明您的 Circom 电路实现是正确的。

# 生成证明
测试通过后，我们就可以使用 Groth16 协议来为这个电路生成一个真实的零知识证明。
```
scripts/run_groth16.sh
```
这个脚本自动化了整个流程：编译、可信设置、计算 witness、生成证明和验证证明。
如何运行脚本:

将上述代码保存到 scripts/run_groth16.sh。

在终端中，首先给脚本执行权限：
```bash
chmod +x scripts/run_groth16.sh
```
然后运行脚本：
```
./scripts/run_groth16.sh
```
脚本会自动执行所有步骤。如果最后输出 ✅✅✅ `Proof is valid!` 和 🎉 `All steps completed successfully!`，则表示您已成功地为 Poseidon2 电路创建并验证了一个零知识证明。所有生成的文件（r1cs, zkey, proof.json 等）都将位于 `build/` 目录下。
