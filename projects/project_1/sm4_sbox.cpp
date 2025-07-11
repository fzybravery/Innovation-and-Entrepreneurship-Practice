inline unsigned int sBoxFast(unsigned int input, const int SBOX[256]) {
    return (SBOX[(input >> 24) & 0xFF] << 24) |
           (SBOX[(input >> 16) & 0xFF] << 16) |
           (SBOX[(input >> 8) & 0xFF] << 8) |
           (SBOX[input & 0xFF]);
}
