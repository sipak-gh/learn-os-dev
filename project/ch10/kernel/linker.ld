/* linker.ld — how GNU ld lays out the kernel.
 *
 * The kernel runs at 1 MiB (stage 2 copies it there), so every address
 * the linker computes starts from 0x100000. The flat binary (objcopy -O
 * binary) is a byte-for-byte image of memory from the first section to the
 * end of .ksyms; .bss takes no space in it and is zeroed by entry.asm.
 */
ENTRY(_start)

SECTIONS
{
    . = 0x100000;
    __kernel_start = .;

    .text : {
        *(.text.entry)          /* _start: must be the very first byte */
        *(.text .text.*)
    }
    __text_end = .;

    .rodata : {
        *(.rodata .rodata.*)
    }

    /* Code and constants above, writable data below, on separate pages:
     * paging_init maps everything before __data_start read-only. */
    . = ALIGN(4096);
    __data_start = .;

    .data : {
        *(.data .data.*)
    }

    /* The kernel's symbol table (Chapter 10, tools/mksyms.sh). It comes
     * after everything else that is loaded, so that the second link,
     * which adds it, moves nothing. */
    .ksyms : {
        *(.ksyms)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    __kernel_end = .;

    /* Sections a freestanding kernel has no use for. */
    /DISCARD/ : {
        *(.comment)
        *(.note .note.*)
        *(.eh_frame .eh_frame_hdr)
    }
}
