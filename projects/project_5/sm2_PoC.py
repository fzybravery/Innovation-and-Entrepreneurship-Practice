import os
import math
import struct
from hashlib import sha256

# ---- 椭圆曲线参数 ----
p  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF
a  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC
b  = 0x28E9FA9E9D9F5E344D5AEF8D20E2F6D0
Gx = 0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7
Gy = 0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0
n  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123

# ---- 模逆 ----
def mod_inv(x, m):
    if x == 0:
        raise ZeroDivisionError()
    lm, hm = 1, 0
    low, high = x % m, m
    while low > 1:
        r = high // low
        nm, new = hm - lm * r, high - low * r
        lm, low, hm, high = nm, new, lm, low
    return lm % m

# ---- 点加、倍点、标量乘 ----
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

# ---- 简化版 SM3 用 SHA-256 替代 ----
def kdf(z: bytes, klen: int) -> bytes:
    ct = 1
    res = b''
    for _ in range(math.ceil(klen / 32)):
        msg = z + struct.pack('>I', ct)
        res += sha256(msg).digest()
        ct += 1
    return res[:klen]

# ---- 签名函数 ----
def sm2_sign(msg: bytes, d: int, k: int = None) -> tuple:
    e = int.from_bytes(sha256(msg).digest(), 'big') % n
    k = k or int.from_bytes(os.urandom(32), 'big') % n
    x1, _ = scalar_mult(k, (Gx, Gy))
    r = (e + x1) % n
    s = (mod_inv(1 + d, n) * (k - r * d)) % n
    return r, s, k

# ---- 恢复私钥：k 泄露场景 ----
def recover_from_k(r, s, k):
    return ((k - s) * mod_inv(s + r, n)) % n

# ---- 恢复私钥：重复使用 k ----
def recover_from_reused_k(r1, s1, r2, s2):
    numerator = s2 - s1
    denominator = r1 - r2 + s1 - s2
    return (numerator * mod_inv(denominator, n)) % n

# ---- 恢复私钥：不同用户共享 k ----
def recover_shared_k(r, s, k):
    return ((k - s) * mod_inv(s + r, n)) % n

# ---- 演示主程序 ----
if __name__ == '__main__':
    print("----- 验证泄露 k 恢复私钥 -----")
    d = int.from_bytes(os.urandom(32), 'big') % n
    msg = b'Attack at dawn!'
    r, s, k = sm2_sign(msg, d, k=int.from_bytes(os.urandom(32), 'big') % n)
    d_recovered = recover_from_k(r, s, k)
    print(f"原私钥:     {hex(d)}")
    print(f"恢复私钥:   {hex(d_recovered)}")
    print(f"是否匹配:   {d == d_recovered}")

    print("\n----- 验证重复使用 k 恢复私钥 -----")
    msg1 = b'Message One'
    msg2 = b'Message Two'
    reused_k = int.from_bytes(os.urandom(32), 'big') % n
    r1, s1, _ = sm2_sign(msg1, d, k=reused_k)
    r2, s2, _ = sm2_sign(msg2, d, k=reused_k)
    d_recovered2 = recover_from_reused_k(r1, s1, r2, s2)
    print(f"原私钥:     {hex(d)}")
    print(f"恢复私钥:   {hex(d_recovered2)}")
    print(f"是否匹配:   {d == d_recovered2}")

    print("\n----- 验证不同用户共享 k 恢复彼此私钥 -----")
    dA = int.from_bytes(os.urandom(32), 'big') % n
    dB = int.from_bytes(os.urandom(32), 'big') % n
    shared_k = int.from_bytes(os.urandom(32), 'big') % n
    rA, sA, _ = sm2_sign(b'Alice Message', dA, k=shared_k)
    rB, sB, _ = sm2_sign(b'Bob Message',   dB, k=shared_k)
    dB_recovered = recover_shared_k(rB, sB, shared_k)
    print(f"原 Bob 私钥: {hex(dB)}")
    print(f"恢复私钥:    {hex(dB_recovered)}")
    print(f"是否匹配:    {dB == dB_recovered}")
