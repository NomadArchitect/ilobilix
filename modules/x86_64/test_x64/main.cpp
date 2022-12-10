// Copyright (C) 2022  ilobilo

#include <lib/log.hpp>
#include <module.hpp>

// Always put DRIVER() before init and fini functions
// __init and __fini are currently just extern "C"
// test1 depends on test2. multiple dependencies are supported
DRIVER(test1, init, fini, "test2")

__init bool init()
{
    log::println("Hello from first x86_64 test driver! {}", "(this should run after test2 driver)");
    return true;
}

__fini bool fini()
{
    log::println("{}", fmt::format("Goodbye from first {} test driver!\n", "x86_64"));
    return true;
}

DRIVER(test2, init2, fini2)

__init bool init2()
{
    printf("Hello from second x86_64 test driver! %s\n", "(this should run before test1 driver)");
    return true;
}

__fini bool fini2()
{
    log::println("Goodbye from second {} test driver!\n", "x86_64");
    return true;
}