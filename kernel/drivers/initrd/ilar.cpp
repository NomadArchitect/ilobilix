// Copyright (C) 2022-2024  ilobilo

#include <drivers/initrd/ilar.hpp>
#include <drivers/vfs.hpp>
#include <lib/log.hpp>

namespace ilar
{
    bool validate(uintptr_t address)
    {
        return strncmp(reinterpret_cast<header*>(address)->signature, ILAR_SIGNATURE, 5) == 0;
    }

    void init(uintptr_t address)
    {
        auto current = reinterpret_cast<header*>(address);
        while (!strncmp(current->signature, ILAR_SIGNATURE, 5))
        {
            std::string_view name(current->name);
            std::string_view target(current->link);

            auto mode = current->mode;
            auto size = current->size;

            vfs::node_t *node = nullptr;

            switch (current->type)
            {
                case ILAR_REGULAR:
                    node = vfs::create(nullptr, name, mode | s_ifreg);
                    if (node == nullptr)
                        log::errorln("ILAR: Could not create regular file '{}'", name);
                    else if (node->res->write(reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(current) + sizeof(header)), 0, size) != ssize_t(size))
                        log::errorln("ILAR: Could not write to regular file '{}'", name);
                    break;
                case ILAR_SYMLINK:
                    node = vfs::symlink(nullptr, name, target);
                    if (node == nullptr)
                        log::errorln("ILAR: Could not create symlink '{}'", name);
                    break;
                case ILAR_DIRECTORY:
                    node = vfs::create(nullptr, name, mode | s_ifdir);
                    if (node == nullptr)
                        log::errorln("ILAR: Could not creare directory '{}'", name);
                    break;
            }

            current = reinterpret_cast<header*>(reinterpret_cast<uintptr_t>(current) + sizeof(header) + size);
        }
    }
} // namespace ilar