# -*- coding: utf-8 -*-
import os

# SM2 域参数
p  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF
a  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC
b  = 0x28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93
Gx = 0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7
Gy = 0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0
n  = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123

# 无限远点 (Jacobian 表示)：Z=0
INF = (0, 1, 0)

def is_inf(P):
    _, _, Z = P
    return Z == 0

def modinv(x):
    return pow(x, p-2, p)

def jacobian_double(P):
    X1, Y1, Z1 = P
    if Z1 == 0:
        return P
    S = (4 * X1 * pow(Y1, 2, p)) % p
    M = (3 * pow(X1, 2, p) + a * pow(Z1, 4, p)) % p
    X3 = (pow(M, 2, p) - 2 * S) % p
    Y3 = (M * (S - X3) - 8 * pow(Y1, 4, p)) % p
    Z3 = (2 * Y1 * Z1) % p
    return (X3, Y3, Z3)

def jacobian_add(P, Q):
    if is_inf(P): return Q
    if is_inf(Q): return P
    X1, Y1, Z1 = P
    X2, Y2, Z2 = Q
    U1 = X1 * pow(Z2, 2, p) % p
    U2 = X2 * pow(Z1, 2, p) % p
    S1 = Y1 * pow(Z2, 3, p) % p
    S2 = Y2 * pow(Z1, 3, p) % p
    if U1 == U2:
        if S1 != S2:
            return INF
        else:
            return jacobian_double(P)
    H = (U2 - U1) % p
    R = (S2 - S1) % p
    H2 = (H * H) % p
    H3 = (H2 * H) % p
    X3 = (R * R - H3 - 2 * U1 * H2) % p
    Y3 = (R * (U1 * H2 - X3) - S1 * H3) % p
    Z3 = (H * Z1 * Z2) % p
    return (X3, Y3, Z3)

def jacobian_to_affine(P):
    X, Y, Z = P
    if Z == 0:
        return (None, None)
    z_inv = modinv(Z)
    z2 = (z_inv * z_inv) % p
    z3 = (z2 * z_inv) % p
    x = (X * z2) % p
    y = (Y * z3) % p
    return (x, y)

def scalar_mult_jacobian(k, P):
    R = INF
    Q = (P[0], P[1], 1)
    for bit in bin(k)[2:]:
        R = jacobian_double(R)
        if bit == '1':
            R = jacobian_add(R, Q)
    return jacobian_to_affine(R)

if __name__ == '__main__':
    # 测试：生成随机 d，计算 d·G
    d = int.from_bytes(os.urandom(32), 'big') % n
    x_aff, y_aff = scalar_mult_jacobian(d, (Gx, Gy))
    print("d·G =", hex(x_aff), hex(y_aff))
