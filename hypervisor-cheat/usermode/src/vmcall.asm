; HYPERVISOR CHEAT - VMCALL Assembly
; x64 MASM implementation for VMCALL instruction
;
; Windows x64 calling convention:
;   RCX = 1st arg (number)
;   RDX = 2nd arg (param1)
;   R8  = 3rd arg (param2)
;   R9  = 4th arg (param3)
;   RAX = return value

.code

; ===========================================
; DoVmcall - Execute VMCALL instruction
; ===========================================
; uint64_t DoVmcall(uint64_t number, uint64_t param1, uint64_t param2, uint64_t param3)
;
; Input:
;   RCX = VMCALL number
;   RDX = param1
;   R8  = param2
;   R9  = param3
;
; Output:
;   RAX = result from hypervisor
;
; VMCALL expects:
;   RAX = number | (magic << 32)
;   RBX = param1
;   RCX = param2
;   RDX = param3

DoVmcall PROC
    ; Save non-volatile register
    push rbx
    
    ; Build RAX = number | (VMCALL_MAGIC << 32)
    ; VMCALL_MAGIC = 0x48564348
    mov rax, 4856434800000000h  ; MAGIC << 32
    or rax, rcx                  ; Add number
    
    ; Setup parameters for VMCALL
    mov rbx, rdx                 ; param1 -> RBX
    mov rcx, r8                  ; param2 -> RCX  
    mov rdx, r9                  ; param3 -> RDX
    
    ; Execute VMCALL
    ; Opcode: 0F 01 C1
    db 0Fh, 01h, 0C1h
    
    ; Result is in RAX
    
    ; Restore non-volatile register
    pop rbx
    
    ret
DoVmcall ENDP

end

