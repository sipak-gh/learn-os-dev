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
 */
#include "paging.h"

#include "interrupts.h"
#include "io.h"
#include "kprintf.h"
#include "pmm.h"
#include "string.h"

#define CR0_WP  (1u << 16)      /* write protect: read-only applies to ring 0 too */
#define CR0_PG  (1u << 31)      /* paging */

#define DIR_INDEX(va)   ((va) >> 22)
#define TABLE_INDEX(va) (((va) >> 12) & 0x3FF)

#define VECTOR_PAGE_FAULT 14

extern char __kernel_start[], __data_start[];   /* kernel/linker.ld */

static uint32_t page_dir[1024] __attribute__((aligned(PAGE_SIZE)));
static bool enabled;

/* The page table that covers va; with create, make one if there is none. */
static uint32_t *table_for(uint32_t va, bool create)
{
    uint32_t *pde = &page_dir[DIR_INDEX(va)];

    if (!(*pde & PAGE_PRESENT)) {
        if (!create)
            return 0;
        uint32_t frame = pmm_alloc();
        if (frame == 0)
            panic("paging: out of memory for a page table");
        memset((void *)frame, 0, PAGE_SIZE);    /* identity-mapped */
        /* The directory entry allows everything; each page's own entry
         * decides (the CPU uses the stricter of the two). */
        *pde = frame | PAGE_PRESENT | PAGE_WRITE;
    }
    return (uint32_t *)(*pde & PAGE_FRAME);
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags)
{
    kassert(virt % PAGE_SIZE == 0 && phys % PAGE_SIZE == 0);
    table_for(virt, true)[TABLE_INDEX(virt)] = phys | flags | PAGE_PRESENT;
    if (enabled)
        invlpg(virt);
}

void paging_unmap(uint32_t virt)
{
    uint32_t *table = table_for(virt, false);
    if (table) {
        table[TABLE_INDEX(virt)] = 0;
        if (enabled)
            invlpg(virt);
    }
}

uint32_t paging_lookup(uint32_t virt)
{
    uint32_t *table = table_for(virt, false);
    return table ? table[TABLE_INDEX(virt)] : 0;
}

/* Error code bits for a page fault (SDM Vol. 3A 4.7). */
#define PF_PRESENT  0x1         /* 0: page not present, 1: protection */
#define PF_WRITE    0x2         /* 0: read, 1: write */
#define PF_USER     0x4         /* 0: ring 0, 1: ring 3 */

static void page_fault(struct regs *r)
{
    uint32_t addr = read_cr2();     /* the address we tried to use */
    uint32_t e = r->error_code;

    kprintf("page fault: %s %s at %08x: %s\n",
            e & PF_USER ? "user" : "kernel",
            e & PF_WRITE ? "write" : "read",
            (unsigned)addr,
            e & PF_PRESENT ? "page is read-only" : "page not present");
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
    interrupt_register(VECTOR_PAGE_FAULT, page_fault);

    write_cr3((uint32_t)page_dir);
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

void paging_print(void)
{
    uint32_t start = 0, first_pte = 0, expect = 0;
    bool in_range = false;

    kprintf("  %-17s  %-8s  %10s  %s\n", "virtual", "physical", "size", "access");
    /* va is 64-bit so the loop can end after the last page. */
    for (uint64_t va = 0; va <= 0x100000000ull; va += PAGE_SIZE) {
        uint32_t pte = va < 0x100000000ull ? paging_lookup((uint32_t)va) : 0;
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
