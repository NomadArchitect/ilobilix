// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <lib/types.hpp>

namespace arch
{
    uintptr_t sys_arch_prctl(int cmd, uintptr_t addr);
} // namespace arch