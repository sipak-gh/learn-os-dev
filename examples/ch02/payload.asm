; payload.asm — one sector of data for the loaders to fetch: LBA 1.
    db "sprout: this text came from sector LBA 1", 13, 10, 0
    times 512 - ($ - $$) db 0
