#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
using namespace std;
using ll = long long;

// 快速幂模运算
ll modexp(ll base, ll exp, ll mod) {
    long long res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) res = (res * base) % mod;
        base = (base * base) % mod;
        exp >>= 1;
    }
    return res;
}

// 扩展欧几里得求逆
ll modinv(ll a, ll m) {
    ll m0 = m, x0 = 0, x1 = 1;
    if (m == 1) return 0;
    while (a > 1) {
        ll q = a / m;
        ll t = m;
        m = a % m; a = t;
        t = x0; x0 = x1 - q * x0; x1 = t;
    }
    if (x1 < 0) x1 += m0;
    return x1;
}

// 计算lcm
ll lcm(ll a, ll b) {
    return a / __gcd(a, b) * b;
}

// 生成Paillier密钥参数 (简化版)
void paillier_setup(ll &n, ll &n2, ll &g, ll &lambda, ll &mu) {
    ll p = 53, q = 59;
    n = p * q;
    n2 = n * n;
    g = n + 1;
    lambda = lcm(p - 1, q - 1);
    // mu = (L(g^lambda mod n^2))^{-1} mod n
    ll x = modexp(g, lambda, n2);
    ll L = (x - 1) / n;
    mu = modinv(L % n, n);
}

// Paillier 加密 m
ll paillier_encrypt(ll m, ll n, ll n2, ll g) {
    ll r = 2;
    while (std::gcd((ll)2, n) != 1) r++;
    ll c = (modexp(g, m, n2) * modexp(r, n, n2)) % n2;
    return c;
}

// Paillier 解密 c
ll paillier_decrypt(ll c, ll n, ll n2, ll lambda, ll mu) {
    ll x = modexp(c, lambda, n2);
    ll L = (x - 1) / n;
    ll m = (L % n) * mu % n;
    return m;
}

int main() {
    // 群参数 (哈希映射)
    ll P = 211, g = 2;
    auto H = [&](ll x) { return modexp(g, x, P); };

    // 随机示例数据
    vector<ll> V = {2, 4, 6};         // P1 的元素
    vector<ll> W = {4, 6, 10};        // P2 的元素
    vector<ll> T = {7, 13, 5};        // 对应值

    // 1. 密钥生成
    ll k1 = 11, k2 = 19;  // 私钥 (示例值)
    ll n, n2, pai_g, lambda, mu;
    paillier_setup(n, n2, pai_g, lambda, mu);

    // 2. 第一轮 (P1 -> P2)
    vector<ll> Z1;
    for (auto v : V) {
        ll z = modexp(H(v), k1, P);
        Z1.push_back(z);
    }
    random_shuffle(Z1.begin(), Z1.end());

    // 3. 第二轮 (P2 -> P1)
    vector<ll> Z2;
    for (auto z : Z1) {
        ll z2 = modexp(z, k2, P);
        Z2.push_back(z2);
    }
    random_shuffle(Z2.begin(), Z2.end());
    // P2计算 (H(w_j)^k2, Enc(t_j))
    vector<pair<ll, ll>> enc_pairs;
    for (size_t i = 0; i < W.size(); i++) {
        ll h = modexp(H(W[i]), k2, P);
        ll c = paillier_encrypt(T[i], n, n2, pai_g);
        enc_pairs.push_back({h, c});
    }
    random_shuffle(enc_pairs.begin(), enc_pairs.end());

    // 4. 第三轮 (P1)
    ll enc_sum = 1;  // 同态相加（乘积）
    vector<ll> inter_ids;
    for (auto &pr : enc_pairs) {
        ll h2 = pr.first;
        ll c = pr.second;
        ll h12 = modexp(h2, k1, P);
        // 判断是否在Z2中
        if (find(Z2.begin(), Z2.end(), h12) != Z2.end()) {
            inter_ids.push_back(h12);
            enc_sum = (enc_sum * c) % n2;
        }
    }

    // 5. 输出
    ll S = paillier_decrypt(enc_sum, n, n2, lambda, mu);
    // 恢复实际的交集元素ID (示例中直接从原始V,W匹配)
    vector<ll> real_inter;
    for (auto w : W) if (find(V.begin(), V.end(), w) != V.end())
        real_inter.push_back(w);
    cout << "P1集合 V: ";
    for (auto v : V) cout << v << " ";
    cout << "\nP2集合 W: ";
    for (size_t i = 0; i < W.size(); i++) cout << W[i] << "(" << T[i] << ") ";
    cout << "\n交集元素: ";
    for (auto w : real_inter) cout << w << " ";
    cout << "\n交集元素的值求和: " << S << endl;

    return 0;
}
