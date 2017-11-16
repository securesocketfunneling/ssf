#if defined(__GNUC__) && \
    (__GNUC__ >= 5 || (__GNUC__ == 4  &&  __GNUC_MINOR__ >= 7)) && \
    defined(__LP64__) && (__LP64__ >= 1) && defined(__x86_64__)

#include <string.h>

// wrap memcpy and force use 2.2.5 version

__asm__(".symver memcpy, memcpy@GLIBC_2.2.5");

extern "C" {
  void *__wrap_memcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
  }
}
#endif  // GCC >= 4.7 and 64 bits
