div_to_zero:
        mov     eax, edi   ; eax = x
        xor     edx, edx   ; edx = 0
        div     esi        ; edx = edx:eax / y
        ret                ; return edx
