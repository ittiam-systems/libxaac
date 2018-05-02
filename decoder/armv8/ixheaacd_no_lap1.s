///******************************************************************************
// *
// * Copyright (C) 2018 The Android Open Source Project
// *
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at:
// *
// * http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// *
// *****************************************************************************
// * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
//*/


.macro push_v_regs
    stp             q8, q9, [sp, #-32]!
    stp             q10, q11, [sp, #-32]!
    stp             q12, q13, [sp, #-32]!
    stp             q14, q15, [sp, #-32]!
    stp             X8, X9, [sp, #-16]!
    stp             X10, X11, [sp, #-16]!
    stp             X12, X13, [sp, #-16]!
    stp             X14, X15, [sp, #-16]!
    stp             X16, X17, [sp, #-16]!
    stp             X29, X30, [sp, #-16]!
.endm
.macro pop_v_regs
    ldp             X29, X30, [sp], #16
    ldp             X16, X17, [sp], #16
    ldp             X14, X15, [sp], #16
    ldp             X12, X13, [sp], #16
    ldp             X10, X11, [sp], #16
    ldp             X8, X9, [sp], #16
    ldp             q14, q15, [sp], #32
    ldp             q12, q13, [sp], #32
    ldp             q10, q11, [sp], #32
    ldp             q8, q9, [sp], #32
.endm
.text
.global ixheaacd_neg_shift_spec_armv8
ixheaacd_neg_shift_spec_armv8:
    push_v_regs
    MOV             X5, #448
    SUB             X6, X5, #1
    LSL             X6, X6, #2
    ADD             X6, X6, X0
    MOV             X8, #-16
    SUB             X6, X6, #12
    LSL             X7, X3, #1
    DUP             V31.4S, W2
    MOV             W4, #0x8000
    DUP             V30.4S, W4

    LD1             {V0.4S}, [X6], X8
    SQNEG           V0.4S, V0.4S

    LD1             {V6.4S}, [X6], X8
    SQSHL           V25.4S, V0.4S, V31.4S
    SQADD           V24.4S, V25.4S, V30.4S
    SSHR            V23.4S, V24.4S, #16
    REV64           V23.4S, V23.4S
    SUB             X5, X5, #8

    UZP1            V27.8H, V23.8H, V23.8H
    SQNEG           V29.4S, V6.4S

LOOP_1:

    ST1             {V27.H}[2], [X1], X7
    SQSHL           V22.4S, V29.4S, V31.4S
    LD1             {V0.4S}, [X6], X8
    ST1             {V27.H}[3], [X1], X7
    SQADD           V21.4S, V22.4S, V30.4S
    ST1             {V27.H}[0], [X1], X7
    SQNEG           V0.4S, V0.4S
    ST1             {V27.H}[1], [X1], X7
    SSHR            V20.4S, V21.4S, #16
    REV64           V20.4S, V20.4S
    SUBS            X5, X5, #8


    UZP1            V27.8H, V20.8H, V20.8H
    SQSHL           V25.4S, V0.4S, V31.4S
    ST1             {V27.H}[2], [X1], X7
    LD1             {V6.4S}, [X6], X8
    SQADD           V24.4S, V25.4S, V30.4S
    ST1             {V27.H}[3], [X1], X7
    SSHR            V23.4S, V24.4S, #16
    ST1             {V27.H}[0], [X1], X7
    REV64           V23.4S, V23.4S
    ST1             {V27.H}[1], [X1], X7


    UZP1            V27.8H, V23.8H, V23.8H
    SQNEG           V29.4S, V6.4S

    BGT             LOOP_1

    ST1             {V27.H}[2], [X1], X7
    SQSHL           V22.4S, V29.4S, V31.4S
    ST1             {V27.H}[3], [X1], X7
    ST1             {V27.H}[0], [X1], X7
    SQADD           V21.4S, V22.4S, V30.4S
    ST1             {V27.H}[1], [X1], X7
    SSHR            V20.4S, V21.4S, #16

    REV64           V20.4S, V20.4S

    UZP1            V27.8H, V20.8H, V20.8H

    ST1             {V27.H}[2], [X1], X7
    ST1             {V27.H}[3], [X1], X7
    ST1             {V27.H}[0], [X1], X7
    ST1             {V27.H}[1], [X1], X7
    pop_v_regs
    RET
