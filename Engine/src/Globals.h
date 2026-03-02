// Globals.h
#pragma once
#define _HAS_STD_BYTE 0

#include <random>

typedef unsigned long long UID;
inline UID GenerateUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<UID> dis;
    return dis(gen);
}
