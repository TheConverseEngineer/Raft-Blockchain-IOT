#ifndef SHA256_H
#define SHA256_H

struct Hash {
    unsigned long long a;
    unsigned long long b;
    unsigned long long c;
    unsigned long long d;
};

Hash hash_data(const unsigned char *data, std::size_t len);

#endif