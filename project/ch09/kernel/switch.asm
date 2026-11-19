; switch.asm — the context switch (Chapter 8).
;
; void switch_to(uint32_t *save_esp, uint32_t new_esp);
;
; Push the registers the C calling convention says a function must
; preserve (ebp, ebx, esi, edi), store esp in *save_esp, load new_esp, and
; pop the same four registers from the other task's stack. The final ret
; returns to wherever *that* task called switch_to from: the task switch
; is just a function that returns on a different stack.
;
; eax, ecx and edx need no saving: the caller expects any call to change
; them. eip is on the stack already (the return address), and eflags is
; the caller's business (interrupts are off whenever we get here).

bits 32

section .text

global switch_to
switch_to:
    mov eax, [esp + 4]          ; save_esp
    mov edx, [esp + 8]          ; new_esp

    push ebp
    push ebx
    push esi
    push edi
    mov [eax], esp              ; this task's stack, as it is now

    mov esp, edx                ; from here on: the other task's stack
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
