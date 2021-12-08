#pragma once

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#else

#include <byteswap.h>

#endif

namespace Protocon {

class Util {
  public:
    static bool IsBigEndian() {
        union {
            uint32_t i;
            char c[4];
        } bint = {0x01020304};

        return bint.c[0] == 1;
    }

    static uint16_t BigEndian(uint16_t v) {
        if (!IsBigEndian())
            return bswap_16(v);
        return v;
    }

    static uint32_t BigEndian(uint32_t v) {
        if (!IsBigEndian())
            return bswap_32(v);
        return v;
    }

    static uint64_t BigEndian(uint64_t v) {
        if (!IsBigEndian())
            return bswap_64(v);
        return v;
    }

  private:
    Util() {}
};

}  // namespace Protocon
