#ifndef SHA256_H
#define SHA256_H

#include <string>

struct Hash {
    unsigned long long a;
    unsigned long long b;
    unsigned long long c;
    unsigned long long d;
};

Hash hash_data(const std::string& s);

#endif