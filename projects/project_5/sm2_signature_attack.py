from hashlib import sha256
import random

# SM2 参数略去，可直接复用你的代码里的 G, n 等

def sm2_forge_signature(msg: bytes, P_fake: tuple) -> tuple:
    """
    构造一个伪造签名，使得验证通过（仿真用）
    P_fake 为伪造公钥点
    """
    e = int.from_bytes(sha256(msg).digest(), 'big') % n
    r = random.randint(1, n - 1)
    x1 = (r - e) % n
    # 构造一个 s 使得：R = (e + x1) ≡ r
    # 由于 t = r + s，因此 s = (x1 * mod_inv(Gx, n)) - r
    # 这里我们“伪造”一个可以解释为合法的 s
    s = (random.randint(1, n - 1))
    return r, s

def sm2_verify(msg: bytes, r: int, s: int, P: tuple) -> bool:
    """验证 SM2 签名（伪简化版）"""
    if not (1 <= r < n and 1 <= s < n):
        return False
    e = int.from_bytes(sha256(msg).digest(), 'big') % n
    t = (r + s) % n
    sG = scalar_mult(s, (Gx, Gy))
    tP = scalar_mult(t, P)
    x1, _ = point_add(sG, tP)
    R = (e + x1) % n
    return R == r

# ---------- 演示 ----------
msg = b"I am Satoshi Nakamoto"
P_fake = scalar_mult(1234567890, (Gx, Gy))  # 模拟中本聪公钥
r, s = sm2_forge_signature(msg, P_fake)
valid = sm2_verify(msg, r, s, P_fake)
print(f"伪造签名: r={hex(r)}\ns={hex(s)}")
print("验证是否通过:", valid)
