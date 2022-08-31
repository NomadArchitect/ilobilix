// Copyright (C) 2022  ilobilo

#include <arch/x86_64/cpu/idt.hpp>
#include <arch/x86_64/lib/io.hpp>
#include <lib/time.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <atomic>

namespace timers::pit
{
    static std::atomic<uint64_t> tick = 0;

    uint64_t time_ms()
    {
        return tick * (1000 / time::frequency);
    }

    void msleep(uint64_t ms)
    {
        volatile uint64_t target = time_ms() + ms;
        while (time_ms() < target)
            asm volatile ("pause");
    }

    static void irq_handler(cpu::registers_t *regs)
    {
        tick++;
        time::timer_handler();
    }

    void init()
    {
        log::info("Initialising PIT...");

        uint64_t divisor = 1193180 / time::frequency;

        io::out<uint8_t>(0x43, 0x36);

        uint8_t l = static_cast<uint8_t>(divisor);
        uint8_t h = static_cast<uint8_t>(divisor >> 8);

        io::out<uint8_t>(0x40, l);
        io::out<uint8_t>(0x40, h);

        auto [handler, vector] = idt::allocate_handler(idt::IRQ(0));
        handler.set(irq_handler);
        idt::unmask(vector - 0x20);
    }
} // namespace timers::pit