import os
import math
import struct
from hashlib import sha256

# ----- 1. SM2 椭圆曲线参数 -----
p  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF
a  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC
b  = 0x28E9FA9E9D9F5E344D5AEF8D20E2F6D0
Gx = 0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7
Gy = 0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0
n  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123

# ----- 2. 模逆：扩展欧几里得 -----
def mod_inv(x, m):
    # 返回 x 在 mod m 上的逆元
    if x == 0:
        raise ZeroDivisionError()
    lm, hm = 1, 0
    low, high = x % m, m
    while low > 1:
        r = high // low
        nm, new = hm - lm * r, high - low * r
        lm, low, hm, high = nm, new, lm, low
    return lm % m

# ----- 3. 椭圆曲线点加/倍/标量乘 -----
def point_add(P, Q):
    if P is None: return Q
    if Q is None: return P
    x1, y1 = P; x2, y2 = Q
    if x1 == x2 and (y1 + y2) % p == 0:
        return None
    if P != Q:
        l = ((y2 - y1) * mod_inv(x2 - x1, p)) % p
    else:
        l = ((3 * x1 * x1 + a) * mod_inv(2 * y1, p)) % p
    x3 = (l * l - x1 - x2) % p
    y3 = (l * (x1 - x3) - y1) % p
    return (x3, y3)

def scalar_mult(k, P):
    R = None
    Q = P
    while k:
        if k & 1:
            R = point_add(R, Q)
        Q = point_add(Q, Q)
        k >>= 1
    return R

# ----- 4. KDF (基于 SM3 这里用 SHA-256 替代演示) -----
def kdf(z: bytes, klen: int) -> bytes:
    ct = 1
    res = b''
    for _ in range(math.ceil(klen / 32)):
        msg = z + struct.pack('>I', ct)
        res += sha256(msg).digest()
        ct += 1
    return res[:klen]

# ----- 5. SM2 加密 -----
def sm2_encrypt(pubkey: tuple, msg: bytes) -> bytes:
    k = int.from_bytes(os.urandom(32), 'big') % n
    C1 = scalar_mult(k, (Gx, Gy))
    x2, y2 = scalar_mult(k, pubkey)
    t = kdf(x2.to_bytes(32,'big') + y2.to_bytes(32,'big'), len(msg))
    C2 = bytes(mi ^ ti for mi, ti in zip(msg, t))
    C3 = sha256(x2.to_bytes(32,'big') + msg + y2.to_bytes(32,'big')).digest()
    # 输出 C1||C3||C2
    x1b = C1[0].to_bytes(32,'big'); y1b = C1[1].to_bytes(32,'big')
    return x1b + y1b + C3 + C2

# ----- 6. SM2 解密 -----
def sm2_decrypt(priv: int, cipher: bytes) -> bytes:
    x1 = int.from_bytes(cipher[:32], 'big')
    y1 = int.from_bytes(cipher[32:64], 'big')
    C3 = cipher[64:96]
    C2 = cipher[96:]
    x2, y2 = scalar_mult(priv, (x1, y1))
    t = kdf(x2.to_bytes(32,'big') + y2.to_bytes(32,'big'), len(C2))
    M = bytes(c2 ^ ti for c2, ti in zip(C2, t))
    u = sha256(x2.to_bytes(32,'big') + M + y2.to_bytes(32,'big')).digest()
    if u != C3:
        raise ValueError("解密校验失败")
    return M

# ----- 7. 演示 -----
if __name__ == '__main__':
    # 生成密钥对
    d = int.from_bytes(os.urandom(32), 'big') % n
    P = scalar_mult(d, (Gx, Gy))

    plaintext = b'The quick brown fox jumps over the lazy dog'
    ciphertext = sm2_encrypt(P, plaintext)
    recovered = sm2_decrypt(d, ciphertext)

    print("明文:", plaintext)
    print("解密:", recovered)
