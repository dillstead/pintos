/* Default linker script, for normal executables */
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
  . = 0x08048000 + SIZEOF_HEADERS;
  .text : { 
    *(.text .text.*) 
    . = ALIGN(0x1000);
  } =0x90909090
  .rodata : { 
    *(.rodata .rodata.*) 
    . = ALIGN(0x1000); 
  }
  .data : { 
    *(.data .data.*) 
    . = ALIGN(0x1000);
  } 
  .bss : { 
    *(.bss .bss.*)
    . = ALIGN(0x1000);
  }
}
