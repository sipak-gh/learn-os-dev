/* user.ld — how GNU ld lays out a user program (Chapter 9).
 *
 * Every program starts at USER_BASE (kernel/paging.h), in its own address
 * space. Two PT_LOAD segments: the ELF headers, code and constants (read,
 * execute), then data and .bss (read, write) on a page of their own, so
 * the kernel can map the first one read-only. `readelf -l` shows them.
 *
 * The layout is the one Linux expects too: a segment's file offset and
 * its address are equal modulo 4096, because Linux maps file pages
 * straight into memory. That is why the code starts after the headers,
 * not at 0x40000000. The data starts on a page boundary, so its addresses
 * don't depend on how much code the compiler made; if a program has
 * .data, ld pads the file to put it at a multiple of 4096 too (.bss takes
 * no room in the file).
 */
ENTRY(_start)

PHDRS
{
    text PT_LOAD FILEHDR PHDRS FLAGS(5);    /* PF_R | PF_X */
    data PT_LOAD FLAGS(6);                  /* PF_R | PF_W */
}

SECTIONS
{
    . = 0x40000000 + SIZEOF_HEADERS;

    .text : { *(.text .text.*) } :text
    .rodata : { *(.rodata .rodata.*) } :text

    . = ALIGN(4096);
    .data : { *(.data .data.*) } :data
    .bss : { *(.bss .bss.*) *(COMMON) } :data

    /DISCARD/ : {
        *(.comment)
        *(.note .note.*)
        *(.eh_frame .eh_frame_hdr)
    }
}
