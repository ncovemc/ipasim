// Stubs for exception handling functions (as defined in cxxabi.h, https://libcxxabi.llvm.org/spec.html).
// These are required to make linker happy. Having stubs of course means we do not support exceptions
// right now (see docs at "objc.md" for more information). Note also that some unresolved symbols (e.g.
// _Unwind_resume) are generated by clang, not referenced explicitely. That means clang generates
// Itanium ABI exception handling instead of Windows one - but I was not able to change that. It has
// probably something to do with compiling Objective-C (also for macOS runtime), where Windows exception
// handling might not be supported by clang.

// All complex types were changed to simpler equivalents (e.g. all pointers to "void *").

#include <cstdint>

extern "C" {
    // These are actually not from libcxxabi, but from libunwind (unwind.h).
    void _Unwind_Resume(void *) { }
    uintptr_t _Unwind_GetCFA(void *) { return 0; }
    uintptr_t _Unwind_GetIP(void *) { return 0; }

    int __gxx_personality_v0(int, int, uint64_t, void *, void *) { return 0; }
    void *__cxa_allocate_exception(size_t) { return nullptr; }
    void __cxa_throw(void *, void *, void (*)(void *)) { }
    void __cxa_rethrow() { }
    void *__cxa_begin_catch(void *) { return nullptr; }
    void __cxa_end_catch() { }
    void *__cxa_current_exception_type() { return nullptr; }
}
