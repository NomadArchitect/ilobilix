// Copyright (C) 2022  ilobilo

#pragma once

#include <type_traits>
#include <main.hpp>
#include <cstdint>

constexpr uint64_t align_down(uint64_t n, uint64_t a)
{
    return (n & ~(a - 1));
}

constexpr uint64_t align_up(uint64_t n, uint64_t a)
{
    return align_down(n + a - 1, a);
}

constexpr uint64_t div_roundup(uint64_t n, uint64_t a)
{
    return align_down(n + a - 1, a) / a;
}

constexpr uint64_t jdn(uint8_t days, uint8_t months, uint16_t years)
{
    return (1461 * (years + 4800 + (months - 14) / 12)) / 4 + (367 * (months - 2 - 12 * ((months - 14) / 12))) / 12 - (3 * ((years + 4900 + (months - 14) / 12) / 100)) / 4 + days - 32075;
}

template<typename type>
constexpr type tohh(type a)
{
    return reinterpret_cast<type>(reinterpret_cast<uint64_t>(a) + hhdm_offset);
}

template<typename type>
constexpr type fromhh(type a)
{
    return reinterpret_cast<type>(reinterpret_cast<uint64_t>(a) - hhdm_offset);
}

template<typename type>
constexpr type pow(type base, type exp)
{
    int result = 1;
    for (; exp > 0; exp--) result *= base;
    return result;
}

template<typename type>
constexpr type abs(type num)
{
    return num < 0 ? -num : num;
}

template<typename type>
constexpr type sign(type num)
{
    return (num > 0) ? 1 : ((num < 0) ? -1 : 0);
}

template<typename type> requires (std::is_same<type, uint16_t>() || std::is_same<type, uint32_t>() || std::is_same<type, uint64_t>())
constexpr type bswap(type v)
{
    if constexpr(std::is_same<type, uint16_t>()) return __builtin_bswap16(v);
    else if constexpr(std::is_same<type, uint32_t>()) return __builtin_bswap32(v);
    else if constexpr(std::is_same<type, uint64_t>()) return __builtin_bswap64(v);
}

struct point
{
    size_t X = 0;
    size_t Y = 0;
};