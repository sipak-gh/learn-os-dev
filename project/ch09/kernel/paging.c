/* paging.c — see paging.h.
 *
 * Two-level 32-bit paging: CR3 holds the physical address of the page
 * directory, 1024 entries that each point to a page table (or nothing);
 * a page table holds 1024 entries that each map one 4 KiB page. A virtual
 * address splits into 10 + 10 + 12 bits:
 *
 *     31        22 21        12 11          0
 *     | directory |   table    |   offset    |
 *
 * sprout identity-maps RAM: virtual address X is physical address X. That
 * is what lets this file write to a page table it got from pmm_alloc():
 * the frame's physical address is also a virtual address that works.
 *
 * Every user task has its own page directory (Chapter 9). The kernel's
 * page tables are shared: each directory points at the same tables for
 * the kernel's addresses, so a change to a kernel page shows up in every
 * address space. Only the entries between USER_BASE and USER_TOP differ.
 */
#include "paging.h"

#include "interrupts.h"
#include "io.h"
#include "kheap.h"
#include "kprintf.h"
#include "pmm.h"
#include "string.h"
#include "user.h"

#define CR0_WP  (1u << 16)      /* write protect: read-only applies to ring 0 too */
#define CR0_PG  (1u << 31)      /* paging */

#define DIR_INDEX(va)   ((va) >> 22)
#define TABLE_INDEX(va) (((va) >> 12) & 0x3FF)

#define VECTOR_PAGE_FAULT 14

extern char __kernel_start[], __data_start[];   /* kernel/linker.ld */

/* The kernel's page directory: the address space of kernel tasks, and
 * the model for every user address space. */
static uint32_t kernel_dir[1024] __attribute__((aligned(PAGE_SIZE)));
static bool enabled;

static bool is_user(uint32_t va)
{
    return va >= USER_BASE && va < USER_TOP;
}

/* The page table in dir that covers va; with create, make one if there is
 * none. Returns NULL if there is none (or no memory for one). */
static uint32_t *table_for(const uint32_t *dir, uint32_t va, bool create)
{
    uint32_t *pde = (uint32_t *)&dir[DIR_INDEX(va)];

    if (!(*pde & PAGE_PRESENT)) {
        if (!create)
            return NULL;
        uint32_t frame = pmm_alloc();
        if (frame == 0)
            return NULL;
        memset((void *)frame, 0, PAGE_SIZE);    /* identity-mapped */
        /* The directory entry allows everything; each page's own entry
         * decides (the CPU uses the stricter of the two). Ring 3 needs
         * PAGE_USER at both levels, so only user tables get it here. */
        *pde = frame | PAGE_PRESENT | PAGE_WRITE
             | (is_user(va) ? PAGE_USER : 0);
    }
    return (uint32_t *)(*pde & PAGE_FRAME);
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags)
{
    kassert(virt % PAGE_SIZE == 0 && phys % PAGE_SIZE == 0);
    kassert(!is_user(virt));
    uint32_t *table = table_for(kernel_dir, virt, true);
    if (!table)
        panic("paging: out of memory for a page table");
    table[TABLE_INDEX(virt)] = phys | flags | PAGE_PRESENT;
    if (enabled)
        invlpg(virt);
}

void paging_unmap(uint32_t virt)
{
    uint32_t *table = table_for(kernel_dir, virt, false);
    if (table) {
        table[TABLE_INDEX(virt)] = 0;
        if (enabled)
            invlpg(virt);
    }
}

uint32_t paging_lookup_in(const uint32_t *dir, uint32_t virt)
{
    uint32_t *table = table_for(dir ? dir : kernel_dir, virt, false);
    return table ? table[TABLE_INDEX(virt)] : 0;
}

uint32_t paging_lookup(uint32_t virt)
{
    return paging_lookup_in(kernel_dir, virt);
}

uint32_t *paging_new_space(void)
{
    uint32_t *dir = (uint32_t *)pmm_alloc();
    if (!dir)
        return NULL;
    /* All of the kernel's directory entries (the user part is empty). */
    memcpy(dir, kernel_dir, PAGE_SIZE);
    return dir;
}

bool paging_map_user(uint32_t *dir, uint32_t virt, uint32_t phys,
                     uint32_t flags)
{
    kassert(virt % PAGE_SIZE == 0 && phys % PAGE_SIZE == 0);
    kassert(is_user(virt));
    uint32_t *table = table_for(dir, virt, true);
    if (!table)
        return false;
    table[TABLE_INDEX(virt)] = phys | flags | PAGE_USER | PAGE_PRESENT;
    /* dir is not loaded in CR3 while we build it: no invlpg needed. */
    return true;
}

void paging_free_space(uint32_t *dir)
{
    kassert((uint32_t)dir != read_cr3());       /* not while we use it */
    for (uint32_t i = DIR_INDEX(USER_BASE); i < DIR_INDEX(USER_TOP); i++) {
        if (!(dir[i] & PAGE_PRESENT))
            continue;
        uint32_t *table = (uint32_t *)(dir[i] & PAGE_FRAME);
        for (int j = 0; j < 1024; j++)
            if (table[j] & PAGE_PRESENT)
                pmm_free(table[j] & PAGE_FRAME);
        pmm_free((uint32_t)table);
    }
    pmm_free((uint32_t)dir);
}

void paging_switch(uint32_t *dir)
{
    uint32_t cr3 = (uint32_t)(dir ? dir : kernel_dir);
    if (read_cr3() != cr3)
        write_cr3(cr3);         /* also flushes the TLB */
}

/* Error code bits for a page fault (SDM Vol. 3A 4.7). */
#define PF_PRESENT  0x1         /* 0: page not present, 1: protection */
#define PF_WRITE    0x2         /* 0: read, 1: write */
#define PF_USER     0x4         /* 0: ring 0, 1: ring 3 */

static void page_fault(struct regs *r)
{
    uint32_t addr = read_cr2();     /* the address we tried to use */
    uint32_t e = r->error_code;

    /* A protection fault in ring 3 is either a kernel page or a write to
     * a read-only one: the page table entry tells which. */
    const char *why = "page not present";
    if (e & PF_PRESENT)
        why = (e & PF_USER) && !(paging_lookup_in((uint32_t *)read_cr3(), addr)
                                 & PAGE_USER)
            ? "page is kernel-only" : "page is read-only";

    kprintf("page fault: %s %s at %08x: %s\n",
            e & PF_USER ? "user" : "kernel",
            e & PF_WRITE ? "write" : "read",
            (unsigned)addr, why);
    if (e & PF_USER)
        user_kill(r);   /* only the program dies */
    regs_dump(r);
    panic("unhandled page fault");
}

void paging_init(void)
{
    uint32_t ro_start = (uint32_t)__kernel_start;
    uint32_t ro_end = (uint32_t)__data_start;

    /* Page 0 stays unmapped, so a NULL pointer faults. */
    for (uint32_t va = PAGE_SIZE; va < pmm_top(); va += PAGE_SIZE) {
        bool code = va >= ro_start && va < ro_end;  /* .text, .rodata */
        paging_map(va, va, code ? 0 : PAGE_WRITE);
    }
    /* Every address space gets a copy of the kernel's directory entries.
     * A page table the kernel created later would be missing from the
     * copies, so make all of the heap's tables now (64 KiB for 64 MiB). */
    for (uint32_t va = KHEAP_START; va - KHEAP_START < KHEAP_MAX;
         va += 1024 * PAGE_SIZE)
        if (!table_for(kernel_dir, va, true))
            panic("paging: out of memory for a page table");
    interrupt_register(VECTOR_PAGE_FAULT, page_fault);

    write_cr3((uint32_t)kernel_dir);
    write_cr0(read_cr0() | CR0_PG | CR0_WP);
    enabled = true;
}

/* Print one run of pages: virtual range, where it starts in physical
 * memory, size, and r/w + u/k. */
static void print_range(uint32_t start, uint32_t end, uint32_t pte)
{
    kprintf("  %08x-%08x  %08x  %6u KiB  %s %s\n",
            (unsigned)start, (unsigned)(end - 1), (unsigned)(pte & PAGE_FRAME),
            (unsigned)((end - start) >> 10),
            pte & PAGE_WRITE ? "rw" : "r-",
            pte & PAGE_USER ? "user" : "kernel");
}

void paging_print(const uint32_t *dir)
{
    uint32_t start = 0, first_pte = 0, expect = 0;
    bool in_range = false;

    kprintf("  %-17s  %-8s  %10s  %s\n", "virtual", "physical", "size", "access");
    /* va is 64-bit so the loop can end after the last page. */
    for (uint64_t va = 0; va <= 0x100000000ull; va += PAGE_SIZE) {
        uint32_t pte = va < 0x100000000ull
                     ? paging_lookup_in(dir, (uint32_t)va) : 0;
        /* A run continues while the flags match and the frames follow
         * each other. */
        bool same = in_range && (pte & PAGE_PRESENT)
                 && (pte & (PAGE_WRITE | PAGE_USER))
                        == (first_pte & (PAGE_WRITE | PAGE_USER))
                 && (pte & PAGE_FRAME) == expect;
        if (in_range && !same) {
            print_range(start, (uint32_t)va, first_pte);
            in_range = false;
        }
        if (!in_range && (pte & PAGE_PRESENT)) {
            in_range = true;
            start = (uint32_t)va;
            first_pte = pte;
        }
        expect = (pte & PAGE_FRAME) + PAGE_SIZE;
    }
}
