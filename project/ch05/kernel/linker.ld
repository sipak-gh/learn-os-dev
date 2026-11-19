/* linker.ld — how GNU ld lays out the kernel.
 *
 * The kernel runs at 1 MiB (stage 2 copies it there), so every address
 * the linker computes starts from 0x100000. The flat binary (objcopy -O
 * binary) is a byte-for-byte image of memory from the first section to the
 * end of .data; .bss takes no space in it and is zeroed by entry.asm.
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

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
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
