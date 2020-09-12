bits 64
section .text
global x86_64_sse_frustum_culling

; Transpose the interlaced matrix and move it into registers
;
; 1 - 4 -> output registers
; 5     -> tmp register
; 6     -> register with data address

%macro mov_trnsp_intrl 6
    movaps %1, [%6 + 0 ] ; %1: 1x 1y 1z 1w
    movaps %3, [%6 + 32] ; %3: 2x 2y 2z 2w
    movaps %5, [%6 + 64] ; %5: 3x 3y 3z 3w
    movaps %2, [%6 + 96] ; %2: 4x 4y 4z 4w

    movaps   %4, %1
    unpcklps %1, %5
    unpckhps %4, %5

    movaps   %5, %3
    unpcklps %3, %2
    unpckhps %5, %2

    movaps   %2, %1
    unpcklps %1, %3 ; %1: 1x 2x 3x 4x
    unpckhps %2, %3 ; %2: 1y 2y 3y 4y

    movaps   %3, %4
    unpcklps %3, %5 ; %3: 1z 2z 3z 4z
   ;unpckhps %4, %5 ; %4: 1w 2w 3w 4w  <- needless
%endmacro



%define plane_l 0   ; Left plane
%define plane_r 64  ; Right plane
%define plane_b 128 ; Bottom
%define plane_t 192 ; Top
%define plane_n 256 ; Near
%define plane_f 320 ; Far



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
    movaps %7,  [%15 + %16 + 0 ] ; %7:  Plane x
    movaps %8,  [%15 + %16 + 16] ; %8:  Plane y
    movaps %9,  [%15 + %16 + 32] ; %9:  Plane z

    movaps %10, %4  ; %10: Max x
    movaps %11, %5  ; %11: Max y
    movaps %12, %6  ; %12: Max z

    mulps %10, %7  ; %10:   Max x  *  Plane x
    mulps %11, %8  ; %11:   Max y  *  Plane y
    mulps %12, %9  ; %12:   Max z  *  Plane z

    mulps %7, %1 ; %7:     Min x  *  Plane x
    mulps %8, %2 ; %8:     Min y  *  Plane y
    mulps %9, %3 ; %9:     Min z  *  Plane z

    maxps %7, %10 ; %7:    Comparison result (CR) of   Min x  and  Max x
    maxps %8, %11 ; %8:    Comparison result (CR) of   Min y  and  Max y
    maxps %9, %12 ; %9:    Comparison result (CR) of   Min z  and  Max z

    addps %7, %8                ; %7:   CR x  +  CR y
    addps %9, [%15 + %16 + 48]  ; %9:   CR z  +  Plane w
    addps %7, %9                ; %7:   CR x  +  CR y  +  CR z  +  Plane w  (Distance to plane)

    cmpps %7,  %13, 1 ; If (Distance < 0)  %7.f32 = 1  else  %7.f32 = 0
    orps  %14, %7     ; Accumulate result
%endmacro


; rdi - pointer to output array of tests results
; rsi - pointer to input AABBs values as: [xyzw(min), xyzw(max)], [xyzw(min), xyzw(max)]...
; rdx - pointer to frustum planes as floats array [6][4][4]
; rcx - count of AABBs and output array AND loop counter (decrement)
; r8d - 32bit mask

x86_64_sse_frustum_culling:
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

    xorps xmm14, xmm14 ; xmm14: zero

    shr  rcx, 2
    test rcx, rcx
    jle  exit

; 4x calculations per loop iteration
main_loop:
    xorps xmm15, xmm15 ; xmm15: reset result

    ; Transpose the interlaced matrices and move them into registers
    ;                x     y     z     w    temp            ; <--- (4x),  w not used
    mov_trnsp_intrl xmm0, xmm1, xmm2, xmm3, xmm4, rsi       ; <--- Min (AA)
    mov_trnsp_intrl xmm3, xmm4, xmm5, xmm6, xmm7, rsi + 16  ; <--- Max (BB)



    ; Perform test with each plane

    ;               MinX   MinY    MinZ    MaxX  MaxY  MaxZ
    prfm_plane_iter xmm0,  xmm1,   xmm2,   xmm3, xmm4, xmm5,   \
                    xmm6,  xmm7,   xmm8,   xmm9, xmm10, xmm11, \
                    xmm14, xmm15,  rdx,    plane_l
    ;               zero   result  planes  plane_type

    prfm_plane_iter xmm0,  xmm1,   xmm2,   xmm3, xmm4, xmm5,   \
                    xmm6,  xmm7,   xmm8,   xmm9, xmm10, xmm11, \
                    xmm14, xmm15,  rdx,    plane_r

    prfm_plane_iter xmm0,  xmm1,   xmm2,   xmm3, xmm4, xmm5,   \
                    xmm6,  xmm7,   xmm8,   xmm9, xmm10, xmm11, \
                    xmm14, xmm15,  rdx,    plane_b

    prfm_plane_iter xmm0,  xmm1,   xmm2,   xmm3, xmm4, xmm5,   \
                    xmm6,  xmm7,   xmm8,   xmm9, xmm10, xmm11, \
                    xmm14, xmm15,  rdx,    plane_t

    prfm_plane_iter xmm0,  xmm1,   xmm2,   xmm3, xmm4, xmm5,   \
                    xmm6,  xmm7,   xmm8,   xmm9, xmm10, xmm11, \
                    xmm14, xmm15,  rdx,    plane_n

    prfm_plane_iter xmm0,  xmm1,   xmm2,   xmm3, xmm4, xmm5,   \
                    xmm6,  xmm7,   xmm8,   xmm9, xmm10, xmm11, \
                    xmm14, xmm15,  rdx,    plane_f

    pxor         xmm1,  xmm1         ; make zero
    movd         xmm1,  r8d          ; load 32 bit mask
    vpbroadcastd xmm2,  xmm1         ; broadcast bit mask for all int32
    pand         xmm15, xmm2         ; apply bit mask for current result

    movdqa       xmm0,  [rdi]        ; load previous result
    pandn        xmm2,  xmm0         ; reset bits of previous by mask
    por          xmm2,  xmm15        ; set result
    movdqa       [rdi], xmm2         ; write results to results buffer

    add     rdi,  16     ; increment result array address
    add     rsi,  128    ; increment AABBs array address
    sub     rcx,  1      ; reduce count (rcx)

    jnz main_loop ; while count (rcx) != 0

exit:
%ifdef WIN64
    pop rsi
    pop rdi
%endif

    ret
