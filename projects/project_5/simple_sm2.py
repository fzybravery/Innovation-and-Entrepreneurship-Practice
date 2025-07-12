from gmssl import sm2, func

# SM2 推荐参数（也可以用随机私钥）
private_key = '00A1B2C3D4E5F6...ABCD'  # 64 hex chars
public_key = '04B1C2D3E4F5...1234ABCD'  # 前缀04 + 64*2 hex chars

# 初始化加密对象
sm2_crypt = sm2.CryptSM2(public_key=public_key, private_key=private_key)

# 原文（bytes）
message = b'Hello, SM2'

# 加密
cipher_text = sm2_crypt.encrypt(message)
print("密文（hex）:", cipher_text.hex())

# 解密
plain_text = sm2_crypt.decrypt(cipher_text)
print("解密结果:", plain_text.decode())
