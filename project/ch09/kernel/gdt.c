/* gdt.c — see gdt.h.
 *
 * Stage 2's GDT (boot/gdt.inc) has ring 0 code and data only. Ring 3
 * needs its own code and data descriptors (DPL 3), and the CPU needs a
 * TSS to know which stack to use when it enters ring 0 from ring 3. The
 * TSS's address is only known once the kernel is linked, so the kernel
 * builds a new GDT here and switches to it.
 */
#include "gdt.h"

#include "string.h"

/* One 8-byte descriptor; the fields are scattered for 80286
 * compatibility (see boot/gdt.inc). */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags_limit_high;
    uint8_t base_high;
} __attribute__((packed));

/* The 32-bit task state segment (Intel SDM Vol. 3A 8.2.1). It was made
 * for hardware task switching, which sprout doesn't use: only esp0/ss0
 * (the ring 0 stack) and iomap_base matter. */
struct tss {
    uint32_t prev_task;
    uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs, ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));

/* Access bytes: present (0x80), DPL (bits 5-6), code/data (0x10),
 * executable (0x08), readable code or writable data (0x02). */
#define ACCESS_KERNEL_CODE  0x9A
#define ACCESS_KERNEL_DATA  0x92
#define ACCESS_USER_CODE    0xFA        /* 0x9A with DPL 3 */
#define ACCESS_USER_DATA    0xF2        /* 0x92 with DPL 3 */
#define ACCESS_TSS          0x89        /* present, 32-bit TSS, not busy */
#define FLAGS_4K_32BIT      0xC         /* limit in 4 KiB pages, 32-bit */

static struct gdt_entry gdt[6];
static struct tss tss;

static void set_entry(int i, uint32_t base, uint32_t limit, uint8_t access,
                      uint8_t flags)
{
    gdt[i].limit_low = limit & 0xFFFF;
    gdt[i].base_low = base & 0xFFFF;
    gdt[i].base_mid = (base >> 16) & 0xFF;
    gdt[i].access = access;
    gdt[i].flags_limit_high = (flags << 4) | ((limit >> 16) & 0x0F);
    gdt[i].base_high = base >> 24;
}

void gdt_init(void)
{
    set_entry(0, 0, 0, 0, 0);                   /* the null descriptor */
    set_entry(1, 0, 0xFFFFF, ACCESS_KERNEL_CODE, FLAGS_4K_32BIT);
    set_entry(2, 0, 0xFFFFF, ACCESS_KERNEL_DATA, FLAGS_4K_32BIT);
    set_entry(3, 0, 0xFFFFF, ACCESS_USER_CODE, FLAGS_4K_32BIT);
    set_entry(4, 0, 0xFFFFF, ACCESS_USER_DATA, FLAGS_4K_32BIT);
    set_entry(5, (uint32_t)&tss, sizeof tss - 1, ACCESS_TSS, 0);

    memset(&tss, 0, sizeof tss);
    tss.ss0 = KERNEL_DS;
    /* An I/O permission bitmap would start here; pointing past the end
     * of the TSS means there is none, so ring 3 may use no I/O ports. */
    tss.iomap_base = sizeof tss;

    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) ptr = { sizeof gdt - 1, (uint32_t)gdt };

    /* lgdt alone changes nothing until a segment register is loaded:
     * reload CS with a far jump, then the data segment registers. */
    __asm__ volatile (
        "lgdt %0\n\t"
        "ljmp %1, $1f\n"
        "1:\n\t"
        "mov %2, %%ds\n\t"
        "mov %2, %%es\n\t"
        "mov %2, %%fs\n\t"
        "mov %2, %%gs\n\t"
        "mov %2, %%ss\n\t"
        "ltr %3"
        : : "m"(ptr), "i"(KERNEL_CS), "r"((uint16_t)KERNEL_DS),
            "r"((uint16_t)TSS_SEL)
        : "memory");
}

void tss_set_kernel_stack(uint32_t esp0)
{
    tss.esp0 = esp0;
}
