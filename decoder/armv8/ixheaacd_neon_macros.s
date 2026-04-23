// decoder/armv8/ixheaacd_armv8_macros.s

// --- Internal Security Dispatchers ---
.macro BTI_ENABLE
#if defined(__ARM_FEATURE_BTI_DEFAULT)
    bti c
#endif
.endm

.macro PAC_ENTRY
#if defined(__ARM_FEATURE_PAC_DEFAULT)
    paciasp
#endif
.endm

.macro PAC_EXIT
#if defined(__ARM_FEATURE_PAC_DEFAULT)
    autiasp
#endif
.endm

// --- Main ENTRY and EXIT Macros ---
.macro ENTRY name
    .p2align 2
\name:
    BTI_ENABLE
    PAC_ENTRY
.endm

.macro EXIT_FUNC
    PAC_EXIT
.endm

// --- GNU Property Note ---
// Signals BTI and PAC support to the Android linker.
#if defined(__linux__) && defined(__aarch64__)
    .pushsection .note.gnu.property, "a"  // Switch to Note section
    .p2align 3
    .word 4           // Name size
    .word 16          // Data size
    .word 5           // NT_GNU_PROPERTY_TYPE_0
    .asciz "GNU"      // Owner
    .word 0xc0000000  // GNU_PROPERTY_AARCH64_FEATURE_1_AND
    .word 4           // Data size
    .word 3           // Value: BTI (Bit 0) | PAC (Bit 1)
    .word 0           // Padding
    .popsection                           // Switch back to previous section
#endif
