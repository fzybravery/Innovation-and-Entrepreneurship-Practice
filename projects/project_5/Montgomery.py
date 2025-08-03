# -*- coding: utf-8 -*-
import os

# 参数同上
p,a,b,Gx,Gy,n = p,a,b,Gx,Gy,n

INF = (0,1,0)
def is_inf(P): return P[2]==0
def jacobian_double(P): ...
def jacobian_add(P, Q): ...
def jacobian_to_affine(P): ...

def montgomery_ladder(k, P):
    R0 = INF
    R1 = (P[0], P[1], 1)
    for bit in bin(k)[2:]:
        # 用算术避免分支：利用 Python 三元表达式也可
        if bit == '0':
            R1 = jacobian_add(R0, R1)
            R0 = jacobian_double(R0)
        else:
            R0 = jacobian_add(R0, R1)
            R1 = jacobian_double(R1)
    return jacobian_to_affine(R0)

if __name__ == '__main__':
    d = int.from_bytes(os.urandom(32), 'big') % n
    x,y = montgomery_ladder(d, (Gx,Gy))
    print("Ladder d·G =", hex(x), hex(y))
