# -*- coding: utf-8 -*-
import os

# 参数同方案一
p,a,b,Gx,Gy,n = p,a,b,Gx,Gy,n

# 复用方案一中的点运算
INF = (0,1,0)
def is_inf(P): return P[2]==0
def jacobian_double(P): ...
def jacobian_add(P, Q): ...
def jacobian_to_affine(P): ...

def compute_wnaf(k, w=4):
    wnaf = []
    while k:
        if k & 1:
            zi = k & ((1 << w) - 1)
            if zi & (1 << (w-1)):
                zi = zi - (1 << w)
            wnaf.append(zi)
            k -= zi
        else:
            wnaf.append(0)
        k //= 2
    return wnaf

def precompute_wnaf_table(P, w=4):
    table = {}
    # 只算正奇数倍，负数倍直接取反
    for i in range(1, 1 << (w-1), 2):
        # 取普通二进制标量乘 —— 可以复用方案一
        table[i] = scalar_mult_jacobian(i, P)
        x,y = table[i]
        table[-i] = (x, (-y) % p)
    return table

def scalar_mult_wnaf(k, P, w=4):
    wnaf = compute_wnaf(k, w)
    tbl = precompute_wnaf_table(P, w)
    R = INF
    Q = None
    for di in reversed(wnaf):
        R = jacobian_double(R)
        if di != 0:
            # 先把 R 转到仿射，做表查，再升回 Jacobian
            x,y = tbl[di]
            Q = (x, y, 1)
            R = jacobian_add(R, Q)
    return jacobian_to_affine(R)

if __name__ == '__main__':
    d = int.from_bytes(os.urandom(32), 'big') % n
    x,y = scalar_mult_wnaf(d, (Gx,Gy), w=5)
    print("w-NAF d·G =", hex(x), hex(y))
