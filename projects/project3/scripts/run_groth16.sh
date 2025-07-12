#!/bin/bash

# 这是一个完整的脚本，用于编译电路、执行可信设置、生成和验证 Groth16 证明。

# --- 目录和变量设置 ---
CIRCUIT_NAME=poseidon2
BUILD_DIR=build
INPUT_JSON=input.json
PRIVATE_INPUT=123 # 这是我们的私密输入 preImage

# --- 清理并创建构建目录 ---
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
echo "✅ Build directory cleaned and created."

# --- 1. 电路编译 ---
# 将 .circom 文件编译成 .r1cs (约束系统) 和 .wasm (用于计算 witness)
echo "1️⃣ Compiling circuit..."
circom circuits/$CIRCUIT_NAME.circom --r1cs --wasm --sym -o $BUILD_DIR
echo "✅ Circuit compiled successfully."

# --- 2. 可信设置 (Groth16) ---
# 这部分分为两个阶段：
# 阶段 1: Powers of Tau (与电路无关，可复用)
# 阶段 2: 电路特定设置

echo "2️⃣ Performing trusted setup..."

# 下载一个已经完成的 Powers of Tau 文件 (这里用 2^14)
# 在生产中，您需要确保这个 .ptau 文件的来源是可信的
if [ ! -f pot14_final.ptau ]; then
    echo "Downloading Powers of Tau file..."
    wget https://hermez.s3-eu-west-1.amazonaws.com/powersOfTau28_hez_final_14.ptau -O pot14_final.ptau
fi
echo "✅ Powers of Tau file is ready."

# 阶段 2: 为我们的电路生成 .zkey 文件
snarkjs groth16 setup $BUILD_DIR/$CIRCUIT_NAME.r1cs pot14_final.ptau $BUILD_DIR/${CIRCUIT_NAME}_0000.zkey
echo "✅ Initial .zkey file created."

# 添加一个"贡献"，生成最终的 .zkey 文件。在生产中，这应该是一个多方计算仪式。
snarkjs zkey contribute $BUILD_DIR/${CIRCUIT_NAME}_0000.zkey $BUILD_DIR/${CIRCUIT_NAME}_final.zkey --name="Contributor 1" -v -e="random entropy"
echo "✅ Final .zkey file created."

# 导出验证密钥 (verification_key.json)，这个密钥可以公开给验证者
snarkjs zkey export verificationkey $BUILD_DIR/${CIRCUIT_NAME}_final.zkey $BUILD_DIR/verification_key.json
echo "✅ Verification key exported."

# --- 3. 计算 Witness ---
# Witness 是满足电路约束的一组信号值，包括私有输入、中间值和公共输出。

echo "3️⃣ Calculating witness..."

# 创建 input.json 文件
# 注意：公开输入（hash）在这里不需要，因为 witness 计算器会根据私有输入自动计算它。
echo "{\"preImage\": \"$PRIVATE_INPUT\"}" > $INPUT_JSON

# 使用 wasm 计算器和输入文件来生成 witness.wtns
node $BUILD_DIR/${CIRCUIT_NAME}_js/generate_witness.js $BUILD_DIR/${CIRCUIT_NAME}_js/$CIRCUIT_NAME.wasm $INPUT_JSON $BUILD_DIR/witness.wtns
echo "✅ Witness calculated and saved to witness.wtns."

# --- 4. 生成证明 ---
echo "4️⃣ Generating Groth16 proof..."
snarkjs groth16 prove $BUILD_DIR/${CIRCUIT_NAME}_final.zkey $BUILD_DIR/witness.wtns $BUILD_DIR/proof.json $BUILD_DIR/public.json
echo "✅ Proof generated and saved to proof.json."
echo "✅ Public signals saved to public.json."

# --- 5. 验证证明 ---
echo "5️⃣ Verifying the proof..."
VERIFICATION_RESULT=$(snarkjs groth16 verify $BUILD_DIR/verification_key.json $BUILD_DIR/public.json $BUILD_DIR/proof.json)

echo "$VERIFICATION_RESULT"

# 检查验证结果
if [[ "$VERIFICATION_RESULT" == *"OK!"* ]]; then
  echo "✅✅✅ Proof is valid!"
else
  echo "❌❌❌ Proof is invalid!"
  exit 1
fi

# --- 清理临时文件 ---
rm $INPUT_JSON
echo "✅ Cleanup complete."
echo "🎉 All steps completed successfully!"
