// Copyright (C) 2022  ilobilo

#pragma once

#if defined(__x86_64__) || defined(_M_X64)

#include <arch/x86_64/cpu/cpu.hpp>
#include <lib/function.hpp>
#include <utility>
#include <cstddef>
#include <cstdint>

namespace arch::x86_64::idt
{
    static constexpr uint8_t SYSCALL = 0x69;

    enum IRQS
    {
        IRQ0 = 32,
        IRQ1 = 33,
        IRQ2 = 34,
        IRQ3 = 35,
        IRQ4 = 36,
        IRQ5 = 37,
        IRQ6 = 38,
        IRQ7 = 39,
        IRQ8 = 40,
        IRQ9 = 41,
        IRQ10 = 42,
        IRQ11 = 43,
        IRQ12 = 44,
        IRQ13 = 45,
        IRQ14 = 46,
        IRQ15 = 47,
    };

    struct [[gnu::packed]] IDTEntry
    {
        uint16_t Offset1;
        uint16_t Selector;
        uint8_t IST;
        uint8_t TypeAttr;
        uint16_t Offset2;
        uint32_t Offset3;
        uint32_t Zero;

        void set(void *isr, uint8_t typeattr = 0x8E, uint8_t ist = 0);
    };

    struct [[gnu::packed]] IDTPtr
    {
        uint16_t Limit;
        uint64_t Base;

        void load()
        {
            asm volatile ("lidt %0" : : "memory"(*this));
        }
    };

    class int_handler
    {
        private:
        std::function<void(cpu::registers_t*)> handler;

        public:
        template<typename Func, typename ...Args>
        bool set(Func &&func, Args ...args)
        {
            if (this->handler == true) return false;
            this->handler = [func = std::forward<Func>(func), args...](cpu::registers_t *regs) mutable
            {
                func(regs, std::move(args)...);
            };
            return true;
        }

        bool get()
        {
            return this->handler == true;
        }

        bool operator()(cpu::registers_t *regs)
        {
            if (this->handler == false) return false;
            this->handler(regs);
            return true;
        }
    };

    extern int_handler handlers[];
    extern IDTEntry idt[];
    extern IDTPtr idtr;

    void init();
} // namespace arch::x86_64::idt

#endif