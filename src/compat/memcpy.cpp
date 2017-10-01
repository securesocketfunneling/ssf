#if defined( __GNUC__ )  &&  defined( __LP64__ )  &&  __LP64__ >= 1

#include <string.h>

// wrap memcpy and force use 2.2.5 version

__asm__(".symver memcpy, memcpy@GLIBC_2.2.5");

extern "C" {
  void *__wrap_memcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
  }
}

#endif  // 64 bits