# call the section ".init" to force ld to put this section at the beginning of the executable
.section .init

.extern higherhalf
.global loader
.global KernelStart

# the kernel entry point
KernelStart:
loader:
	# here is the trick: we load a GDT with a base address
	# of 0x40000000 for the code (0x08) and data (0x10) segments
	lgdt	(setupGDT)
	mov		$0x10,%ax
	mov		%ax,%ds
	mov		%ax,%es
	mov		%ax,%fs
	mov		%ax,%gs
	mov		%ax,%ss

	# jump to the higher half kernel
	ljmp	$0x08,$higherhalf

# our GDT for the setup-process
setupGDT:
	# GDT size
	.word		setupGDTEntries - setupGDTEntriesEnd - 1
	# Pointer to entries
	.long		setupGDTEntries

# the GDT-entries
setupGDTEntries:
	# null gate
	.long 0,0

	# code selector 0x08: base 0x40000000, limit 0xFFFFFFFF, type 0x9A, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0x40

	# data selector 0x10: base 0x40000000, limit 0xFFFFFFFF, type 0x92, granularity 0xCF
	.byte 0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0x40
setupGDTEntriesEnd:
