/* user.c — see user.h. */
#include "user.h"

#include "elf.h"
#include "kprintf.h"
#include "paging.h"
#include "pmm.h"
#include "string.h"
#include "task.h"

/* kernel/usermode.asm */
void enter_user_mode(uint32_t eip, uint32_t esp) __attribute__((noreturn));

/* A user task's kernel half: the first thing it runs, in ring 0, on its
 * kernel stack, in its own address space (schedule() loaded CR3). */
static int user_main(void)
{
    enter_user_mode(task_current()->arg, USER_TOP);
}

int user_spawn(const char *name, const uint8_t *file, uint32_t size)
{
    uint32_t *space = paging_new_space();
    if (!space) {
        kprintf("%s: out of memory\n", name);
        return -1;
    }

    uint32_t entry;
    const char *error = elf_load(space, file, size, &entry);
    for (uint32_t va = USER_TOP - USER_STACK_SIZE; !error && va < USER_TOP;
         va += PAGE_SIZE) {
        uint32_t frame = pmm_alloc();
        if (frame)
            memset((void *)frame, 0, PAGE_SIZE);    /* no old secrets */
        if (!frame || !paging_map_user(space, va, frame, PAGE_WRITE)) {
            if (frame)
                pmm_free(frame);
            error = "out of memory";
        }
    }
    int id = error ? -1 : task_spawn_in(name, user_main, space, entry);
    if (id < 0) {
        kprintf("%s: %s\n", name, error ? error : "too many tasks");
        paging_free_space(space);
    }
    return id;
}

bool user_access_ok(const void *p, uint32_t n, bool write)
{
    uint32_t start = (uint32_t)p;
    if (start < USER_BASE || start >= USER_TOP || n > USER_TOP - start)
        return false;
    uint32_t need = PAGE_PRESENT | PAGE_USER | (write ? PAGE_WRITE : 0);
    for (uint32_t page = start & ~(PAGE_SIZE - 1); page < start + n;
         page += PAGE_SIZE)
        if ((paging_lookup_in(task_current()->space, page) & need) != need)
            return false;
    return true;
}

void user_kill(const struct regs *r)
{
    struct task *t = task_current();
    kprintf("task %d (%s) killed at eip %08x\n",
            t->id, t->name, (unsigned)r->eip);
    task_exit(TASK_KILLED);
}
