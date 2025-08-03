import random
import math

# 简单的模幂运算
def modexp(base, exp, mod):
    res = 1
    base %= mod
    while exp > 0:
        if exp & 1:
            res = (res * base) % mod
        base = (base * base) % mod
        exp >>= 1
    return res

# 生成Paillier密钥
def generate_paillier_key():
    # 为简单起见，选用小素数p,q
    p = 53
    q = 59
    n = p * q
    n2 = n * n
    # g取n+1
    g = n + 1
    # lam = lcm(p-1, q-1)
    lam = (p-1)*(q-1) // math.gcd(p-1, q-1)
    # 计算mu = (L(g^lam mod n^2))^{-1} mod n
    x = modexp(g, lam, n2)
    L = (x - 1) // n
    # 扩展欧几里得求模逆
    mu = pow(L, -1, n)
    return (n, n2, g, lam, mu)

def paillier_encrypt(m, n, n2, g):
    # 选择随机r：这里简化选择固定r=2
    r = 2
    while math.gcd(r, n) != 1:
        r += 1
    # ciphertext = g^m * r^n mod n^2
    c = (modexp(g, m, n2) * modexp(r, n, n2)) % n2
    return c

def paillier_decrypt(c, n, n2, lam, mu):
    x = modexp(c, lam, n2)
    L = (x - 1) // n
    m = (L * mu) % n
    return m

# -------------- 协议实现 --------------
# 群参数（用于哈希）：使用小素数和基元
P = 211  # 群的素数
g = 2
def H(x): return modexp(g, x, P)

# 随机生成示例数据
random.seed(1)
V = [2, 4, 6]                  # P1 的元素集合
W_vals = {4: 7, 6: 13, 10: 5}  # P2 的元素和值
W = list(W_vals.keys())

# 1. 密钥生成
k1 = random.randint(1, P-2)
k2 = random.randint(1, P-2)
n, n2, g_pai, lam, mu = generate_paillier_key()

# 2. Round1 (P1 -> P2)
Z1 = [modexp(H(v), k1, P) for v in V]
random.shuffle(Z1)

# 3. Round2 (P2 -> P1)
Z2 = [modexp(z, k2, P) for z in Z1]
random.shuffle(Z2)
enc_pairs = []
for w in W:
    h_w = modexp(H(w), k2, P)
    enc_t = paillier_encrypt(W_vals[w], n, n2, g_pai)
    enc_pairs.append((h_w, enc_t))
random.shuffle(enc_pairs)

# 4. Round3 (P1)
intersection = []
encrypted_values = []
for (h_w, enc_t) in enc_pairs:
    h_w_k = modexp(h_w, k1, P)
    if h_w_k in Z2:
        intersection.append(h_w_k)
        encrypted_values.append(enc_t)
# 同态累乘生成交集和值的密文
if encrypted_values:
    c_sum = 1
    for c in encrypted_values:
        c_sum = (c_sum * c) % n2
else:
    c_sum = 1  # 空集之和为0，对应密文1
# 5. 输出
S = paillier_decrypt(c_sum, n, n2, lam, mu)
print("P1集合 V:", V)
print("P2集合 W:", W_vals)
# 恢复交集元素 ID（这里直接通过原始数据对比恢复）
true_inter = [w for w in W if w in V]
print("交集元素:", true_inter)
print("交集元素的值求和:", S)
