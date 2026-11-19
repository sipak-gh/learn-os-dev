/* elf.c — see elf.h.
 *
 * An ELF executable starts with a header that says where its program
 * headers are. Each PT_LOAD program header is a segment: copy filesz bytes
 * from offset in the file to address vaddr, then zeros up to memsz (that
 * is the .bss). `readelf -h -l build/user/count` shows both tables.
 */
#include "elf.h"

#include <stdbool.h>

#include "paging.h"
#include "pmm.h"
#include "string.h"

struct elf_header {
    uint8_t ident[16];          /* "\x7f" "ELF", class, byte order, ... */
    uint16_t type;              /* 2: executable */
    uint16_t machine;           /* 3: Intel 80386 */
    uint32_t version;
    uint32_t entry;             /* address of the first instruction */
    uint32_t phoff;             /* where the program headers are */
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;         /* size of one program header */
    uint16_t phnum;             /* how many there are */
    uint16_t shentsize, shnum, shstrndx;
} __attribute__((packed));

struct elf_phdr {
    uint32_t type;              /* 1: PT_LOAD */
    uint32_t offset;            /* in the file */
    uint32_t vaddr;             /* in memory */
    uint32_t paddr;
    uint32_t filesz;            /* bytes from the file */
    uint32_t memsz;             /* bytes in memory; the rest are zeros */
    uint32_t flags;             /* PF_X 1, PF_W 2, PF_R 4 */
    uint32_t align;
} __attribute__((packed));

#define ET_EXEC     2
#define EM_386      3
#define PT_LOAD     1
#define PF_W        2

/* Make sure every page of [va, va + n) is mapped in space, with fresh
 * zeroed frames. Writable if any segment on the page is. */
static bool map_pages(uint32_t *space, uint32_t va, uint32_t n, bool write)
{
    uint32_t end = va + n;
    for (uint32_t page = va & ~(PAGE_SIZE - 1); page < end; page += PAGE_SIZE) {
        uint32_t pte = paging_lookup_in(space, page);
        uint32_t frame = pte & PAGE_FRAME;
        if (!(pte & PAGE_PRESENT)) {
            frame = pmm_alloc();
            if (!frame)
                return false;
            memset((void *)frame, 0, PAGE_SIZE);    /* identity-mapped */
        }
        uint32_t flags = (write || (pte & PAGE_WRITE)) ? PAGE_WRITE : 0;
        if (!paging_map_user(space, page, frame, flags)) {
            if (!(pte & PAGE_PRESENT))
                pmm_free(frame);
            return false;
        }
    }
    return true;
}

/* Copy n bytes to user address va in space. The kernel writes through the
 * identity map (the frame's physical address), because space isn't the
 * address space we are running in. */
static void copy_to_space(uint32_t *space, uint32_t va, const uint8_t *src,
                          uint32_t n)
{
    while (n > 0) {
        uint32_t frame = paging_lookup_in(space, va) & PAGE_FRAME;
        uint32_t offset = va % PAGE_SIZE;
        uint32_t chunk = PAGE_SIZE - offset;
        if (chunk > n)
            chunk = n;
        memcpy((uint8_t *)frame + offset, src, chunk);
        va += chunk;
        src += chunk;
        n -= chunk;
    }
}

const char *elf_load(uint32_t *space, const uint8_t *file, uint32_t size,
                     uint32_t *entry)
{
    const struct elf_header *h = (const struct elf_header *)file;

    if (size < sizeof *h || memcmp(h->ident, "\x7f" "ELF", 4) != 0)
        return "not an ELF file";
    if (h->ident[4] != 1 || h->ident[5] != 1)
        return "not 32-bit little-endian";
    if (h->type != ET_EXEC || h->machine != EM_386)
        return "not an i386 executable";
    if (h->phentsize != sizeof(struct elf_phdr)
        || h->phoff > size
        || h->phnum > (size - h->phoff) / sizeof(struct elf_phdr))
        return "bad program headers";

    const struct elf_phdr *ph = (const struct elf_phdr *)(file + h->phoff);
    for (int i = 0; i < h->phnum; i++, ph++) {
        if (ph->type != PT_LOAD || ph->memsz == 0)
            continue;
        /* Everything comes from the file, so check everything: a bad
         * segment must not make the kernel read or map the wrong thing. */
        if (ph->offset > size || ph->filesz > size - ph->offset
            || ph->filesz > ph->memsz)
            return "segment outside the file";
        if (ph->vaddr < USER_BASE || ph->vaddr >= USER_TOP
            || ph->memsz > USER_TOP - ph->vaddr)
            return "segment outside user memory";
        if (!map_pages(space, ph->vaddr, ph->memsz, ph->flags & PF_W))
            return "out of memory";
        copy_to_space(space, ph->vaddr, file + ph->offset, ph->filesz);
    }
    if (h->entry < USER_BASE || h->entry >= USER_TOP
        || !(paging_lookup_in(space, h->entry) & PAGE_PRESENT))
        return "entry point not in a segment";
    *entry = h->entry;
    return NULL;
}
