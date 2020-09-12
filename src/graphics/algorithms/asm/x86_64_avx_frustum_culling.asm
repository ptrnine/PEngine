bits 64
section .text
global x86_64_avx_frustum_culling

; Transpose the interlaced matrix and move it into registers
;
; 1 - 4 -> output ymm registers / tmp xmm (pass as "mm*")
; 5 - 8 -> tmp ymm registers
; 9     -> register with data address

%macro mov_trnsp_intrl8x4 9
    vmovaps     x%1, [%9      ]
    vmovaps     x%2, [%9 +  32]
    vmovaps     x%3, [%9 +  64]
    vmovaps     x%4, [%9 +  96]

    vinsertf128 y%1, [%9 + 128], 0x1
    vinsertf128 y%2, [%9 + 160], 0x1
    vinsertf128 y%3, [%9 + 192], 0x1
    vinsertf128 y%4, [%9 + 224], 0x1

    vshufps %5, y%1, y%2, 0x44
    vshufps %7, y%1, y%2, 0xEE
    vshufps %6, y%3, y%4, 0x44
    vshufps %8, y%3, y%4, 0xEE

    vshufps y%1, %5, %6, 0x88
    vshufps y%2, %5, %6, 0xDD
    vshufps y%3, %7, %8, 0x88
   ;vshufps y%4, %7, %8, 0xDD  ; needless
%endmacro



%define plane_l 0   ; Left plane
%define plane_r 128  ; Right plane
%define plane_b 256 ; Bottom
%define plane_t 384 ; Top
%define plane_n 512 ; Near
%define plane_f 640 ; Far



; Perform distance test with plane
;
; 1 - 3  -> registers with Min AA (x, y, z)
; 4 - 6  -> registers with Max BB (x, y, z)
; 7 - 12 -> registers for computation
; 13     -> register with null numbers
; 14     -> register for output result
; 15     -> register with address to plane
; 16     -> plane type

%macro prfm_plane_iter 16
    vmovaps %7,  [%15 + %16 + 0 ] ; %7:  Plane x
    vmovaps %8,  [%15 + %16 + 32] ; %8:  Plane y
    vmovaps %9,  [%15 + %16 + 64] ; %9:  Plane z

    vmulps %10, %4, %7  ; %10:   Max x  *  Plane x
    vmulps %11, %5, %8  ; %11:   Max y  *  Plane y
    vmulps %12, %6, %9  ; %12:   Max z  *  Plane z

    vmulps %7, %1 ; %7:     Min x  *  Plane x
    vmulps %8, %2 ; %8:     Min y  *  Plane y
    vmulps %9, %3 ; %9:     Min z  *  Plane z

    vmaxps %7, %10 ; %7:    Comparison result (CR) of   Min x  and  Max x
    vmaxps %8, %11 ; %8:    Comparison result (CR) of   Min y  and  Max y
    vmaxps %9, %12 ; %9:    Comparison result (CR) of   Min z  and  Max z

    vaddps %7, %8                ; %7:   CR x  +  CR y
    vaddps %9, [%15 + %16 + 96]  ; %9:   CR z  +  Plane w
    vaddps %7, %9                ; %7:   CR x  +  CR y  +  CR z  +  Plane w  (Distance to plane)

    vcmpps %7,  %13, 1 ; If (Distance < 0)  %7.f32 = 1  else  %7.f32 = 0
    vorps  %14, %7     ; Accumulate result
%endmacro


; rdi - pointer to output array of tests results (uint32_t*)
; rsi - pointer to input AABBs values as: [xyzw(min), xyzw(max)], [xyzw(min), xyzw(max)]...
; rdx - pointer to frustum planes as floats array [6][4][4]
; rcx - count of AABBs and output array AND loop counter (decrement)
; r8d - 32bit mask

x86_64_avx_frustum_culling:
%ifdef WIN64
    push rdi
    push rsi

    ; As System V
    mov rdi, rcx
    mov rsi, rdx
    mov rdx, r8
    mov rcx, r9
    ; TODO: bit position
%endif

    vxorps ymm14, ymm14 ; ymm14: zero

    shr  rcx, 3
    test rcx, rcx
    jle  exit

; 8x calculations per loop iteration
main_loop:
    vxorps ymm15, ymm15 ; xmm15: reset result

    ; Transpose the interlaced matrices and move them into registers
    ;                   x    y    z    w            temps                    ; <--- (8x),  w not used
    mov_trnsp_intrl8x4 mm0, mm1, mm2, mm3, ymm4, ymm5, ymm6, ymm7,  rsi      ; <--- Min (AA)
    mov_trnsp_intrl8x4 mm3, mm4, mm5, mm6, ymm7, ymm8, ymm9, ymm10, rsi + 16 ; <--- Max (BB)



    ; Perform test with each plane

    ;               MinX   MinY    MinZ    MaxX  MaxY  MaxZ
    prfm_plane_iter ymm0,  ymm1,   ymm2,   ymm3, ymm4,  ymm5,  \
                    ymm6,  ymm7,   ymm8,   ymm9, ymm10, ymm11, \
                    ymm14, ymm15,  rdx,    plane_l
    ;               zero   result  planes  plane_type

    prfm_plane_iter ymm0,  ymm1,   ymm2,   ymm3, ymm4,  ymm5,  \
                    ymm6,  ymm7,   ymm8,   ymm9, ymm10, ymm11, \
                    ymm14, ymm15,  rdx,    plane_r

    prfm_plane_iter ymm0,  ymm1,   ymm2,   ymm3, ymm4,  ymm5,  \
                    ymm6,  ymm7,   ymm8,   ymm9, ymm10, ymm11, \
                    ymm14, ymm15,  rdx,    plane_b

    prfm_plane_iter ymm0,  ymm1,   ymm2,   ymm3, ymm4,  ymm5,  \
                    ymm6,  ymm7,   ymm8,   ymm9, ymm10, ymm11, \
                    ymm14, ymm15,  rdx,    plane_t

    prfm_plane_iter ymm0,  ymm1,   ymm2,   ymm3, ymm4,  ymm5,  \
                    ymm6,  ymm7,   ymm8,   ymm9, ymm10, ymm11, \
                    ymm14, ymm15,  rdx,    plane_n

    prfm_plane_iter ymm0,  ymm1,   ymm2,   ymm3, ymm4,  ymm5,  \
                    ymm6,  ymm7,   ymm8,   ymm9, ymm10, ymm11, \
                    ymm14, ymm15,  rdx,    plane_f

    pxor         xmm1,  xmm1         ; make zero
    movd         xmm1,  r8d          ; load 32 bit mask
    vpbroadcastd ymm2,  xmm1         ; broadcast bit mask for all int32
    vpand        ymm15, ymm2         ; apply bit mask for current result

    vmovdqa      ymm0,  [rdi]        ; load previous result
    vpandn       ymm2,  ymm0         ; reset bits of previous by mask
    vpor         ymm2,  ymm15        ; set result
    vmovdqa      [rdi], ymm2         ; write results to results buffer

    add      rdi,  32     ; increment result array address
    add      rsi,  256    ; increment AABBs array address
    sub      rcx,  1      ; reduce count (rcx)

    jnz main_loop ; while count (rcx) != 0

exit:
%ifdef WIN64
    pop rsi
    pop rdi
%endif

    ret
