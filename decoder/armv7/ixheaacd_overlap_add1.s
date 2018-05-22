@/******************************************************************************
@ *
@ * Copyright (C) 2018 The Android Open Source Project
@ *
@ * Licensed under the Apache License, Version 2.0 (the "License");
@ * you may not use this file except in compliance with the License.
@ * You may obtain a copy of the License at:
@ *
@ * http://www.apache.org/licenses/LICENSE-2.0
@ *
@ * Unless required by applicable law or agreed to in writing, software
@ * distributed under the License is distributed on an "AS IS" BASIS,
@ * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@ * See the License for the specific language governing permissions and
@ * limitations under the License.
@ *
@ *****************************************************************************
@ * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
@*/


.text
.p2align 2


    .global ixheaacd_over_lap_add1_armv7
ixheaacd_over_lap_add1_armv7:

    STMFD           sp!, {R4-R12, R14}
    VPUSH           {d8 - d15}

    LDR             R4, [SP, #104]
    LDR             R5, [SP, #108]
    LDR             R6, [SP, #112]
    MOV             R10, R5, LSL #1
    SUB             R11, R10, #1
    MOV             R10, R11, LSL #2
    ADD             R10, R0, R10
    SUB             R10, R10, #12
    MOV             R8, R11, LSL #1
    ADD             R8, R8, R3
    SUB             R8, R8, #14
    MOV             R12, #0
    VDUP.S16        D12, R12
    MOV             R12, #-16
    VDUP.16         Q11, R4
    VLD1.32         {D6, D7}, [R10], R12
    MOV             R7, #0x2000
    VREV64.32       Q3, Q3
    RSB             R7, R7, #0
    VQNEG.S32       Q0, Q3
    VDUP.32         Q10, R7
    VUZP.16         D1, D0
    SUB             R11, R5, #1
    VUZP.16         D7, D6
    SMULBB          R11, R11, R6
    MOV             R11, R11, LSL #1
    VLD2.16         {D2, D3}, [R8], R12
    ADD             R11, R11, R2
    VREV64.16       Q1, Q1
    MOV             R4, R6, LSL #1
    RSB             R4, R4, #0
    MOV             R9, R6, LSL #1
    SMULBB          R6, R5, R6
    MOV             R6, R6, LSL #1
    ADD             R6, R6, R2



    VMULL.U16       Q15, D7, D2
    VLD1.32         {D4, D5}, [R1]!
    VSHR.U32        Q15, Q15, #16

    VMLAL.S16       Q15, D6, D2
    VQSHL.S32       Q15, Q15, Q11


    VADDL.S16       Q7, D3, D12

    VMULL.S32       Q13, D4, D14
    VQMOVN.S64      D28, Q13
    VMULL.S32       Q13, D5, D15
    VQMOVN.S64      D29, Q13

    VQADD.S32       Q14, Q14, Q10
    VQSUB.S32       Q13, Q15, Q14
    VQSHL.S32       Q13, Q13, #2
    VSHR.S32        Q13, Q13, #16
    VUZP.16         D26, D27


    VMULL.U16       Q12, D1, D3
    VSHR.U32        Q12, Q12, #16
    VMLAL.S16       Q12, D0, D3
    VQSHL.S32       Q12, Q12, Q11
    VLD1.32         {D6, D7}, [R10], R12


    VADDL.S16       Q7, D2, D12

    VMULL.S32       Q0, D14, D4
    VQMOVN.S64      D16, Q0
    VMULL.S32       Q0, D15, D5
    VQMOVN.S64      D17, Q0

    VREV64.32       Q3, Q3
    VQADD.S32       Q8, Q8, Q10
    VQNEG.S32       Q0, Q3
    VUZP.16         D1, D0
    VQSUB.S32       Q9, Q12, Q8
    VUZP.16         D7, D6
    VQSHL.S32       Q9, Q9, #2
    VLD2.16         {D2, D3}, [R8], R12
    VSHR.S32        Q9, Q9, #16
    VREV64.16       Q1, Q1
    VUZP.16         D18, D19

    VLD1.32         {D4, D5}, [R1]!
    SUB             R5, R5, #8


LOOP_1:

    VST1.16         D26[0], [R11], R4
    VMULL.U16       Q15, D7, D2
    VST1.16         D26[1], [R11], R4
    VMULL.U16       Q12, D1, D3
    VST1.16         D26[2], [R11], R4
    VSHR.U32        Q15, Q15, #16
    VST1.16         D26[3], [R11], R4
    VSHR.U32        Q12, Q12, #16
    VST1.16         D18[0], [R6], R9
    VMLAL.S16       Q15, D6, D2
    VST1.16         D18[1], [R6], R9
    VMLAL.S16       Q12, D0, D3
    VST1.16         D18[2], [R6], R9
    VQSHL.S32       Q15, Q15, Q11
    VST1.16         D18[3], [R6], R9
    VQSHL.S32       Q12, Q12, Q11
    VLD1.32         {D6, D7}, [R10], R12


    VADDL.S16       Q7, D3, D12

    VMULL.S32       Q8, D4, D14
    VQMOVN.S64      D28, Q8
    VMULL.S32       Q8, D5, D15
    VQMOVN.S64      D29, Q8
    VREV64.32       Q3, Q3



    VADDL.S16       Q7, D2, D12

    VMULL.S32       Q0, D4, D14
    VQMOVN.S64      D16, Q0
    VMULL.S32       Q0, D5, D15
    VQMOVN.S64      D17, Q0

    VLD2.16         {D2, D3}, [R8], R12
    VQNEG.S32       Q0, Q3
    VLD1.32         {D4, D5}, [R1]!
    VQADD.S32       Q14, Q14, Q10
    VUZP.16         D1, D0
    VQADD.S32       Q8, Q8, Q10
    VUZP.16         D7, D6
    VQSUB.S32       Q13, Q15, Q14
    VREV64.16       Q1, Q1
    VQSUB.S32       Q9, Q12, Q8
    VQSHL.S32       Q13, Q13, #2
    VQSHL.S32       Q9, Q9, #2
    VMULL.U16       Q15, D7, D2
    VSHR.S32        Q13, Q13, #16
    VUZP.16         D26, D27
    VSHR.S32        Q9, Q9, #16
    VST1.16         D26[0], [R11], R4
    VMULL.U16       Q12, D1, D3
    VUZP.16         D18, D19
    VSHR.U32        Q15, Q15, #16
    VST1.16         D26[1], [R11], R4
    VMLAL.S16       Q15, D6, D2         @MLA
    VST1.16         D26[2], [R11], R4
    VSHR.U32        Q12, Q12, #16
    VST1.16         D26[3], [R11], R4
    VMLAL.S16       Q12, D0, D3         @MLA
    VST1.16         D18[0], [R6], R9
    VQSHL.S32       Q15, Q15, Q11
    VST1.16         D18[1], [R6], R9
    VQSHL.S32       Q12, Q12, Q11
    VST1.16         D18[2], [R6], R9


    VADDL.S16       Q7, D3, D12

    VMULL.S32       Q8, D4, D14
    VQMOVN.S64      D28, Q8
    VMULL.S32       Q8, D5, D15
    VQMOVN.S64      D29, Q8

    VST1.16         D18[3], [R6], R9


    VADDL.S16       Q7, D2, D12

    VMULL.S32       Q0, D4, D14
    VQMOVN.S64      D16, Q0
    VMULL.S32       Q0, D5, D15
    VQMOVN.S64      D17, Q0

    VLD1.32         {D6, D7}, [R10], R12
    VQADD.S32       Q14, Q14, Q10
    VREV64.32       Q3, Q3
    VQNEG.S32       Q0, Q3
    VUZP.16         D1, D0
    VQSUB.S32       Q13, Q15, Q14
    VUZP.16         D7, D6
    VQADD.S32       Q8, Q8, Q10
    VLD2.16         {D2, D3}, [R8], R12
    VQSUB.S32       Q9, Q12, Q8
    VREV64.16       Q1, Q1
    VQSHL.S32       Q13, Q13, #2
    VLD1.32         {D4, D5}, [R1]!
    VQSHL.S32       Q9, Q9, #2
    VSHR.S32        Q13, Q13, #16
    SUBS            R5, R5, #8
    VSHR.S32        Q9, Q9, #16
    VUZP.16         D26, D27
    VUZP.16         D18, D19



    BGT             LOOP_1

    VST1.16         D26[0], [R11], R4
    VMULL.U16       Q15, D7, D2
    VST1.16         D26[1], [R11], R4
    VMULL.U16       Q12, D1, D3
    VST1.16         D26[2], [R11], R4
    VSHR.U32        Q15, Q15, #16
    VST1.16         D26[3], [R11], R4
    VSHR.U32        Q12, Q12, #16

    VST1.16         D18[0], [R6], R9
    VMLAL.S16       Q15, D6, D2
    VST1.16         D18[1], [R6], R9
    VMLAL.S16       Q12, D0, D3
    VST1.16         D18[2], [R6], R9
    VQSHL.S32       Q15, Q15, Q11
    VST1.16         D18[3], [R6], R9
    VQSHL.S32       Q12, Q12, Q11




    VADDL.S16       Q7, D3, D12

    VMULL.S32       Q8, D4, D14
    VQMOVN.S64      D28, Q8
    VMULL.S32       Q8, D5, D15
    VQMOVN.S64      D29, Q8



    VADDL.S16       Q7, D2, D12

    VMULL.S32       Q13, D4, D14
    VQMOVN.S64      D16, Q13
    VMULL.S32       Q13, D5, D15
    VQMOVN.S64      D17, Q13

    VQADD.S32       Q14, Q14, Q10
    VQADD.S32       Q8, Q8, Q10
    VQSUB.S32       Q13, Q15, Q14
    VQSUB.S32       Q9, Q12, Q8
    VQSHL.S32       Q13, Q13, #2
    VQSHL.S32       Q9, Q9, #2
    VSHR.S32        Q13, Q13, #16
    VSHR.S32        Q9, Q9, #16
    VUZP.16         D26, D27

    VUZP.16         D18, D19



    VST1.16         D26[0], [R11], R4
    VST1.16         D26[1], [R11], R4
    VST1.16         D26[2], [R11], R4
    VST1.16         D26[3], [R11], R4

    VST1.16         D18[0], [R6], R9
    VST1.16         D18[1], [R6], R9
    VST1.16         D18[2], [R6], R9
    VST1.16         D18[3], [R6], R9

    VPOP            {d8 - d15}
    LDMFD           sp!, {R4-R12, R15}

