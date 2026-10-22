// Compatibility stand-ins for Aurora's prebuilt aarch64 Dawn package (libwebgpu_dawn.a). Its
// objects were compiled on a newer distro, so they reference helpers that the aarch64 build
// image's glibc 2.31 does not export. Each definition below is a faithful replacement, which lets
// the prebuilt link (and run) on older systems without glibc 2.32/2.38. Linked into the aarch64
// build only (docs/HANDHELD_BUILD.md).
#include <cstdlib>

extern "C" {

// glibc >= 2.32: "no threads created yet" hint that newer libstdc++/Abseil inline code reads.
// 0 = treat the process as multi-threaded, i.e. always take the locked path (the safe value).
char __libc_single_threaded = 0;

// glibc >= 2.38: the C23 strtol/strtoll entry points. They differ from the pre-C23 ones only
// for "0b" binary prefixes and a few locale corner cases; the pre-C23 calls are fine here.
long __isoc23_strtol(const char* nptr, char** endptr, int base) {
    return std::strtol(nptr, endptr, base);
}

long long __isoc23_strtoll(const char* nptr, char** endptr, int base) {
    return std::strtoll(nptr, endptr, base);
}

unsigned long __isoc23_strtoul(const char* nptr, char** endptr, int base) {
    return std::strtoul(nptr, endptr, base);
}

unsigned long long __isoc23_strtoull(const char* nptr, char** endptr, int base) {
    return std::strtoull(nptr, endptr, base);
}

} // extern "C"
