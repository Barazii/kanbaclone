#pragma once
// Minimal uuid stub for testing
#include <cstring>
#include <cstdint>

typedef unsigned char uuid_t[16];

inline void uuid_generate(uuid_t out) {
    static uint64_t counter = 0;
    counter++;
    std::memset(out, 0, 16);
    std::memcpy(out, &counter, sizeof(counter));
}

inline void uuid_unparse_lower(const uuid_t uu, char* out) {
    static const char hex[] = "0123456789abcdef";
    int pos = 0;
    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) out[pos++] = '-';
        out[pos++] = hex[(uu[i] >> 4) & 0xF];
        out[pos++] = hex[uu[i] & 0xF];
    }
    out[pos] = '\0';
}
