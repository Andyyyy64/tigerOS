OUTPUT_ARCH(riscv)
ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

SECTIONS
{
  . = 0x80200000;

  .text : ALIGN(16)
  {
    *(.text.init)
    *(.text .text.*)
  } :text

  .rodata : ALIGN(16)
  {
    *(.rodata .rodata.*)
  } :text

  . = ALIGN(0x1000);

  .data : ALIGN(16)
  {
    *(.sdata .sdata.*)
    *(.data .data.*)
  } :data

  .bss : ALIGN(16)
  {
    __bss_start = .;
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(16);
    . += 16K;
    __stack_top = .;
    __bss_end = .;
  } :data

  /DISCARD/ :
  {
    *(.eh_frame)
    *(.comment)
  }
}
