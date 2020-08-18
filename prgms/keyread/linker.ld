OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY (__internal_start)

SECTIONS
{
	. = 0x100000;
	.text : { *(.text) }
	.data : { *(.data) }
	.rodata : { *(.rodata) }
}
