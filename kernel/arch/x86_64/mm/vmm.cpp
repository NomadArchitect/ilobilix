// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/cpu/cpu.hpp>
#include <init/kernel.hpp>
#include <lib/misc.hpp>
#include <lib/log.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

namespace vmm
{
    enum
    {
        Present = (1 << 0),
        Write = (1 << 1),
        UserSuper = (1 << 2),
        PWT = (1 << 3), // PWT
        PCD = (1 << 4), // PCD
        Accessed = (1 << 5),
        LargerPages = (1 << 7),
        PAT4k = (1 << 7), // PAT lvl1
        Global = (1 << 8),
        Custom0 = (1 << 9),
        Custom1 = (1 << 10),
        Custom2 = (1 << 11),
        PATlg = (1 << 12), // PAT lvl2+
        NoExec = (1UL << 63)
    };

    struct [[gnu::packed]] ptable { ptentry entries[512]; };
    static bool gib1_pages = false;

    namespace arch
    {
        uintptr_t pa_mask = 0x000FFFFFFFFFF000;
        uintptr_t new_table_flags = Present | Write | UserSuper;

        void *alloc_ptable()
        {
            return new ptable;
        }
    } // namespace arch

    bool ptentry::is_valid()
    {
        return this->getflags(Present);
    }

    bool ptentry::is_large()
    {
        return this->getflags(LargerPages);
    }

    ptentry *pagemap::virt2pte(uint64_t vaddr, bool allocate, uint64_t psize, bool checkll)
    {
        size_t pml5_entry = (vaddr & (0x1FFULL << 48)) >> 48;
        size_t pml4_entry = (vaddr & (0x1FFULL << 39)) >> 39;
        size_t pml3_entry = (vaddr & (0x1FFULL << 30)) >> 30;
        size_t pml2_entry = (vaddr & (0x1FFULL << 21)) >> 21;
        size_t pml1_entry = (vaddr & (0x1FFULL << 12)) >> 12;

        ptable *pml4, *pml3, *pml2, *pml1;
        if (this->toplvl == nullptr)
            return nullptr;

        pml4 = static_cast<ptable*>(if_max_pgmode(this->get_next_lvl(this->toplvl->entries[pml5_entry], allocate)) : this->toplvl);
        if (pml4 == nullptr)
            return nullptr;

        pml3 = static_cast<ptable*>(this->get_next_lvl(pml4->entries[pml4_entry], allocate));
        if (pml3 == nullptr)
            return nullptr;

        if (psize == this->llpage_size || (checkll && pml3->entries[pml3_entry].is_large()))
            return &pml3->entries[pml3_entry];

        pml2 = static_cast<ptable*>(this->get_next_lvl(pml3->entries[pml3_entry], allocate, vaddr, this->llpage_size, psize));
        if (pml2 == nullptr)
            return nullptr;

        if (psize == this->lpage_size || (checkll && pml2->entries[pml2_entry].is_large()))
            return &pml2->entries[pml2_entry];

        pml1 = static_cast<ptable*>(this->get_next_lvl(pml2->entries[pml2_entry], allocate, vaddr, this->lpage_size, psize));
        if (pml1 == nullptr)
            return nullptr;

        return &pml1->entries[pml1_entry];
    }

    /*
     * None:              PAT0:  PAT = 0, PCD = 0, PWT = 0
     * None:              PAT1:  PAT = 0, PCD = 0, PWT = 1
     * UncacheableStrong: PAT2:  PAT = 0, PCD = 1, PWT = 0
     * WriteCombining:    PAT3:  PAT = 0, PCD = 1, PWT = 1
     * WriteThrough:      PAT4:  PAT = 1, PCD = 0, PWT = 0
     * WriteProtected:    PAT5:  PAT = 1, PCD = 0, PWT = 1
     * WriteBack:         PAT6:  PAT = 1, PCD = 1, PWT = 0
     * Uncacheable:       PAT7:  PAT = 1, PCD = 1, PWT = 1
     */

    static uint64_t cache2flags(caching cache, bool largepages)
    {
        uint64_t patbit = (largepages ? PATlg : PAT4k);
        uint64_t ret = 0;
        switch (cache)
        {
            case uncacheable_strong:
                ret |= PCD;
                break;
            case write_combining:
                ret |= PCD | PWT;
                break;
            case write_through:
                ret |= patbit;
                break;
            case write_protected:
                ret |= patbit | PWT;
                break;
            case write_back:
                ret |= patbit | PCD;
                break;
            case uncacheable:
                ret |= patbit | PCD | PWT;
                break;
        }
        return ret;
    }

    uintptr_t pagemap::virt2phys(uintptr_t vaddr, size_t flags)
    {
        std::unique_lock guard(lock);

        auto psize = this->get_psize(flags);
        ptentry *pml_entry = this->virt2pte(vaddr, false, psize, true);
        if (pml_entry == nullptr || !pml_entry->getflags(Present))
            return invalid_addr;

        return pml_entry->getaddr() + (vaddr % psize);
    }

    bool pagemap::map_nolock(uintptr_t vaddr, uintptr_t paddr, size_t flags, caching cache)
    {
        auto map_one = [this](uintptr_t vaddr, uintptr_t paddr, size_t flags, caching cache, size_t psize)
        {
            ptentry *pml_entry = this->virt2pte(vaddr, true, psize, false);
            if (pml_entry == nullptr)
            {
                if (print_errors)
                    log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
                return false;
            }

            auto realflags = flags2arch(flags) | cache2flags(cache, psize > this->page_size);

            pml_entry->reset();
            pml_entry->setaddr(paddr);
            pml_entry->setflags(realflags, true);
            return true;
        };

        auto psize = this->get_psize(flags);
        if (psize == this->llpage_size && gib1_pages == false)
        {
            flags &= ~llpage;
            flags |= lpage;
            for (size_t i = 0; i < gib1; i += mib2)
                if (!map_one(vaddr + i, paddr + i, flags, cache, mib2))
                    return false;
            return true;
        }

        return map_one(vaddr, paddr, flags, cache, psize);
    }

    bool pagemap::unmap_nolock(uintptr_t vaddr, size_t flags)
    {
        auto unmap_one = [this](uintptr_t vaddr, size_t psize)
        {
            ptentry *pml_entry = this->virt2pte(vaddr, false, psize, true);
            if (pml_entry == nullptr)
            {
                if (print_errors)
                    log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
                return false;
            }

            pml_entry->reset();
            cpu::invlpg(vaddr);
            return true;
        };

        auto psize = this->get_psize(flags);
        if (psize == this->llpage_size && gib1_pages == false)
        {
            flags &= ~llpage;
            flags |= lpage;
            for (size_t i = 0; i < gib1; i += mib2)
                if (!unmap_one(vaddr + i, mib2))
                    return false;
            return true;
        }

        return unmap_one(vaddr, psize);
    }

    bool pagemap::setflags_nolock(uintptr_t vaddr, size_t flags, caching cache)
    {
        auto psize = this->get_psize(flags);
        ptentry *pml_entry = this->virt2pte(vaddr, true, psize, true);
        if (pml_entry == nullptr)
        {
            if (print_errors)
                log::errorln("VMM: Could not get page map entry for address 0x{:X}", vaddr);
            return false;
        }

        auto realflags = flags2arch(flags) | cache2flags(cache, psize > this->page_size);
        auto addr = pml_entry->getaddr();

        pml_entry->reset();
        pml_entry->setaddr(addr);
        pml_entry->setflags(realflags, true);
        return true;
    }

    void pagemap::load(bool)
    {
        wrreg(cr3, fromhh(reinterpret_cast<uint64_t>(this->toplvl)));
    }

    void pagemap::save()
    {
        this->toplvl = reinterpret_cast<ptable*>(tohh(rdreg(cr3)));
    }

    pagemap::pagemap() : toplvl(new ptable)
    {
        this->llpage_size = gib1;
        this->lpage_size = mib2;
        this->page_size = kib4;

        if (kernel_pagemap == nullptr)
        {
            for (size_t i = 256; i < 512; i++)
                get_next_lvl(this->toplvl->entries[i], true);

            cpu::enablePAT();
            return;
        }

        for (size_t i = 256; i < 512; i++)
            this->toplvl->entries[i] = kernel_pagemap->toplvl->entries[i];
    }

    bool is_canonical(uintptr_t addr)
    {
        return if_max_pgmode((addr <= 0x00FFFFFFFFFFFFFFULL) || (addr >= 0xFF00000000000000ULL)) :
                             (addr <= 0x00007FFFFFFFFFFFULL) || (addr >= 0xFFFF800000000000ULL);
    }

    uintptr_t flags2arch(size_t flags)
    {
        uintptr_t ret = Present;
        if (flags & write)
            ret |= Write;
        if (!(flags & exec))
            ret |= NoExec;
        if (flags & user)
            ret |= UserSuper;
        if (flags & global)
            ret |= Global;
        if (islpage(flags))
            ret |= LargerPages;
        return ret;
    }

    std::pair<size_t, caching> arch2flags(uintptr_t flags, bool lpages)
    {
        size_t ret1 = 0;
        if (flags & Present)
            ret1 |= read;
        if (flags & Write)
            ret1 |= write;
        if (!(flags & NoExec))
            ret1 |= exec;
        if (flags & UserSuper)
            ret1 |= user;
        if (flags & Global)
            ret1 |= global;

        uint64_t patbit = (lpages ? PATlg : PAT4k);
        caching ret2;

        if ((flags & (patbit | PCD | PWT)) == (patbit | PCD | PWT))
            ret2 = uncacheable;
        else if ((flags & (patbit | PCD)) == (patbit | PCD))
            ret2 = write_back;
        else if ((flags & (patbit | PWT)) == (patbit | PWT))
            ret2 = write_protected;
        else if ((flags & (patbit)) == (patbit))
            ret2 = write_through;
        else if ((flags & (PCD | PWT)) == (PCD | PWT))
            ret2 = write_combining;
        else if ((flags & (PCD)) == (PCD))
            ret2 = uncacheable_strong;

        return { ret1, ret2 };
    }

    static void destroy_level(pagemap *pmap, ptable *pml, size_t start, size_t end, size_t level)
    {
        if (level == 0 || pml == nullptr)
            return;

        for (size_t i = start; i < end; i++)
        {
            auto next = static_cast<ptable*>(pmap->get_next_lvl(pml->entries[i], false));
            if (next == nullptr)
                continue;

            destroy_level(pmap, next, 0, 512, level - 1);
        }
        delete pml;
    }

    void arch_destroy_pmap(pagemap *pmap)
    {
        destroy_level(pmap, pmap->get_raw(), 0, 256, if_max_pgmode(5) : 4);
    }

    void arch_init()
    {
        uint32_t a, b, c, d;
        gib1_pages = cpu::id(0x80000001, 0, a, b, c, d) && ((d & 1 << 26) == 1 << 26);
    }
} // namespace vmm