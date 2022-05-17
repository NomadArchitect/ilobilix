// Copyright (C) 2022  ilobilo

#if defined(__x86_64__) || defined(_M_X64)
#include <arch/x86_64/uart/uart.hpp>
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arch/arm64/uart/uart.hpp>
#endif

namespace uart
{
    void printc(char c, void *arg)
    {
        #if defined(__x86_64__) || defined(_M_X64)
        arch::x86_64::uart::printc(c, arg);
        #elif defined(__aarch64__) || defined(_M_ARM64)
        arch::arm64::uart::printc(c, arg);
        #endif
    }

    void init()
    {
        #if defined(__x86_64__) || defined(_M_X64)
        arch::x86_64::uart::init();
        #elif defined(__aarch64__) || defined(_M_ARM64)
        arch::arm64::uart::init();
        #endif
    }
} // namespace uart