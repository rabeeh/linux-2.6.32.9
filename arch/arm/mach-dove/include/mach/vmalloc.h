/*
 * include/asm-arm/arch-dove/vmalloc.h
 */

/* Dove LCD driver performs big allocations for FrameBuffer memory, we need to
 * move CONSISTENT_BASE by 32MB
 */
/* This should be deleted... */
#define DOVE_LCD_FB_MEMORY	(0x0)
/* Was 0x2000000 */

#define VMALLOC_END	(0xfd800000 - DOVE_LCD_FB_MEMORY)

