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

