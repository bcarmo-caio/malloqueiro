# We are working with m32 here, install the following packages
# apt install gcc-multilib g++-multilib libc6-dev-i386

# Sources to syscalls using int 0x80
# https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.6/+/refs/heads/jb-mr1-dev/sysroot/usr/include/asm/unistd_32.h
# https://github.com/spotify/linux/blob/master/arch/x86/include/asm/unistd_32.h
# https://www.ime.usp.br/~kon/MAC211/syscalls.html
# https://syscalls32.paolostivanin.com/


# Each memory chunk is designed as follows:
#  +-------------------------------------------+ high
#  |   Allocated space                         | 0x0008 + n \
#  |   destinated                              |  ...       |  USER SPACE (n bytes)
#  |   to caller (n bytes)                     |  ...       /
#  +-------------------------------------------+ 0x0008  <--- this address is returned to user
#  |   Chunk size excluding metadata (3 bytes) | 0x0005     \
#  |   Next chunk (pointer) (4 bytes)          | 0x0001     |  METADATA (8 bytes)
#  |   Chunk in use (1 byte)                   | 0x0000     /
#  +-------------------------------------------+ low

# Chunk size metadata is chosen to be 3 bytes to make
# metadata consumes 8 bytes and get memory alignment. However
# we're going to allow only 2 bytes.
# Note that 2 bytes give us at most 2^(8*2) = 2^16 = 65KB of userspace
# chunk and that's enough for the purpose of this program =)
#
# The memory management will work as a simple linked list
# HEAD -> chunk_1 -> chunk_2 -> ... -> chunk_n -> NULL
# * Memory allocation strategy
#     for chunk in {chunk_1 .. chunk_n}
#       if chunk is not free
#         if there's a next chunk -> continue with next chunk
#         else -> allocate new chunk and return it
#       if chunk is free
#         if chunk_userspace_size < desired size
#           if there's a next chunk -> continue with next chunk
#           else -> allocate new chunk and return it
#         if chunk_userspace_size = desired size -> return it
#         if chunk_userspace_size < desired size + (metadata_size + 1)
#           return it (we cannot break it into two, not enough space for metadata)
#         if chunk_userspace_size >= deired size + (metadata_size + 1)
#           break it into 2 chunks:
#             chunk_A having desired size and
#             chunk_B having chunk_userspace_size < deired size + (metadata_size + 1) and
#             return chunk_A
# * Memory deallocation strategy
#     mark chunk_to_free as free (there can only be at most 3 contigous free chunks)
#     first_free_chunk := null |
#     last_free_chunk := null  | these two must be contiguous
#     for chunk in {chunk_to_free .. chunk_n}
#       if chunk is free
#         first_free_chunk := chunk
#         if there's a next chunk
#           if next chunk is free -> last

.section .rodata
log_already_init: .string "Malloqueiro has already been initialized\n"
log_not_init: .string "Malloqueiro has not yet been initialized\n"
log_debug: .string "Hi\n"
fmt_too_much: .string "Malloqueiro can not allocate more than 0x0000ffff bytes at once\n"
fmt_new_line: .string "\n"
metadata_size: .long 8

.section .data
initial_brk: .long 0 # null
head: .long 0 # null

fmt_initial_brk_addr: .string "Inital brk memory address: 0xzzzzzzzz\n\n"
fmt_new_brk: .string "New brk at 0xzzzzzzzz\n"
fmt_malloca: .string "Trying to alocate 0xzzzz byte(s)\n"


# +--------------------+
# |       ...          |
# |   previous frame   |
# |   (params or not)  |
# |--------------------|
# |   return addr      |
# |--------------------|
# |    pushed ebp      |
# |--------------------| <- new ebp
# |    local mess      |
# | (pushed registers) |
# |   (local vars)     |
# +--------------------+ <- esp

# DO NOT USE PRINTF, it will use malloc and mess with malloqueiro
# Use sys_write instead
.macro printf
  err_do_not_use_printf
.endm

# leave eax alone
.macro pusha
  err_do_not_use_pusha_or_popa
.endm
.macro popa
  pusha
.endm

.macro pushad
  push %ecx # |  It makes no sens pushing eax and esp
  push %edx # |
  push %ebx # |
  push %ebp # |
  push %esi # |
  push %edi # | increases esp in 6 x 4 = 24 bytes when using this push
.endm

.macro popad
  pop %edi
  pop %esi
  pop %ebp
  pop %ebx
  pop %edx
  pop %ecx
.endm

.macro get_cur_brk
  mov $45, %eax   # 45 is brk syscall
  mov $0, %ebx    # 0 means current brk
  int $0x80
  # new value is already in eax, return in eax
.endm

# This macro already considers metadata size, only pass me how many bytes
# the user wants
.macro increase_brk size
  get_cur_brk                 # |
  mov %eax, %ebx              # |
  addl \size, %ebx            # |
  addl (metadata_size), %ebx  # |
  mov $45, %eax               # |
  int $0x80                   # | set brk to current_brk + n_size + metadata size
  # new value is already in eax, return in eax
.endm

.macro debug
  pushad
  mov $4, %eax         # write sys call
  mov $1, %ebx         # stdout
  mov $fmt_debug, %ecx # const char *
  mov $3, %edx         # size_t
  int $0x80            # ssize_t write(stdout, "Hi\n", 3) MESSES WITH eax
  popad
.endm

# A memory address 0xzzzzzzzz needs 4 bytes but in ascii
# we need 8 bytes. That's why we need to break into 2 arguments for memory address.
#
# First argument is a pointer INSIDE the format string we're gonna work on
# Second is a 4 bytes data encoded in ascii (most significant memory address)
# Third is a 4 bytes data encoded in ascii (least significant memory address)
#
# Example: If we want to format the string called foo = "Hi, memory at 0xzzzzzzzz\n"
# and we want to fill it with 0xf00f00ca, first encode 0xf00f00ca to ascii
# using 2 regions of memory of 4bytes being:
#
# most significant part will be 0xf00f (encoded to 0x66303066)
# least significant part will be 0x00ca (encoded to 0x30306361)
#
# then (assuming foo is located at 0x00001000 and
#       16 is the offset in foo we have to begin our replacement)
#
# insert_ascii_into_string 0x000010010 most_significant_part least_significant_part
.macro insert_ascii_into_string2 fmt_with_offset most least
  # most
  mov \most, %eax
  mov %eax, 0(\fmt_with_offset)
  # least
  mov \least, %eax
  mov %eax, 4(\fmt_with_offset)
.endm

.macro insert_ascii_into_string fmt_with_offset number
  mov \number, %eax
  mov %eax, (\fmt_with_offset)
.endm

.macro print char_p len
  mov $4, %eax
  mov $1, %ebx
  mov \char_p, %ecx
  mov \len, %edx
  int $0x80
.endm

.macro new_line
  print $fmt_new_line $1
.endm

.section .text
.globl malloca, malloca_free

# 2 arguments:
#   least significant part of memory address in coded in ascii
#   most significant part of memory address in coded in ascii
# no local vars, no return value
print_initial_brk_addr: # void f(most, least)
    mov $fmt_initial_brk_addr, %ebx
    add $29, %ebx # 29 is our offset here
    insert_ascii_into_string2 %ebx, 8(%esp), 4(%esp)
    print $fmt_initial_brk_addr, $39
    ret

# 2 arguments:
#   least significant part of memory address in coded in ascii
#   most significant part of memory address in coded in ascii
# no local vars, no return value
print_new_brk:
    mov $fmt_new_brk, %ebx
    add $13, %ebx # 13 is our offset here
    insert_ascii_into_string2 %ebx, 8(%esp), 4(%esp)
    print $fmt_new_brk, $22
    ret

# 1 arguments:
#   4 bytes integer coded in ascii
# no local vars, no return value
print_malloca:
    mov $fmt_malloca, %ebx
    add $20, %ebx # 14 is our offset here
    insert_ascii_into_string %ebx, 4(%esp)
    print $fmt_malloca, $33
    ret

# Converts the byte in dl to ascii and returns in al
_byte2ascii:
    push %edx
    and $0x0000000f, %edx
    xor %eax, %eax
    mov %dl, %al
    sub $10, %dl
    jge _bytes2ascii_letter
    add $0x30, %al # 0x30 is ascii 0
    pop %edx
    ret
  _bytes2ascii_letter:
    add $0x57, %al # 0x57 = 0x61 (ascii a) - 10 (10 chars previously handled, the numbers))
    pop %edx
    ret

# Receives a memory address in edx and returns the
# most significant part in ebx and the least significant part in ecx
bytes2ascii:
    xor %ebx, %ebx
    xor %ecx, %ecx

    # most significant
    call _byte2ascii
    shl  $24, %eax   # Little endian here. Store in it reverse
    or   %eax, %ebx

    shr  $4, %edx
    call _byte2ascii
    shl  $16, %eax
    or   %eax, %ebx

    shr  $4, %edx
    call _byte2ascii
    shl  $8, %eax
    or   %eax, %ebx

    shr  $4, %edx
    call _byte2ascii
    or   %eax, %ebx

    # least significant
    shr  $4, %edx
    call _byte2ascii
    shl  $24, %eax
    or   %eax, %ecx

    shr  $4, %edx
    call _byte2ascii
    shl  $16, %eax
    or   %eax, %ecx

    shr  $4, %edx
    call _byte2ascii
    shl  $8, %eax
    or   %eax, %ecx

    shr  $4, %edx
    call _byte2ascii
    or   %eax, %ecx

    ret

malloca_init: # no arguments, no local vars, no return value
    cmpl $0, (initial_brk)   # |
    je malloca_init_proceed  # | if initial_brk variable is 0, proceed. Log and abort otherwise
    print $log_already_init, $41
    jmp malloca_init_END
  malloca_init_proceed:
    get_cur_brk
    mov %eax, (initial_brk)    # save initial brk value to initial_brk variable
    movl %eax, %edx
    call bytes2ascii
    push %ecx
    push %ebx
    call print_initial_brk_addr
    add $8, %esp
  malloca_init_END:
    ret

get_free_chunk: # 1 4b argument, no local var, pointer return value (may be null)
    cmpl $0, head          #
    je no_free_chunk      # if head is null, return null
  no_free_chunk:  # no free chunk, return null
    movl $0, %eax
    ret

malloca: # 1 4b argument, no local var, pointer return value (may be null)
    pushad                   # | we are comming from C, push all registers
    mov %esp, %ebp           # | init frame pointer

    cmpl $0, initial_brk # |
    jne  initialized     # | if initial_brk is variable 0, init. Proceed otherwise
    call malloca_init

  initialized:
    # remember that we pushed all registers but eax and esp , so 24 + 4 (ret address) = 28
    mov 28(%ebp), %eax       # get bytes argument
    mov %eax, %edx
    call bytes2ascii
    push %ebx
    call print_malloca
    add $4, %esp

    mov 28(%ebp), %eax
    sub $0x0000FFFF, %eax
    jg too_much

    mov 28(%ebp), %eax

    push 28(%ebp)            # |
    call get_free_chunk      # | get_free_chunk(size_t n_size)
    add $4, %esp

    cmpl $0, %eax            # |
    jne malloca_END          # | if get_free_chunk could get a chunk, return it
                             # | (head already exists at this point)

    increase_brk 28(%ebp)
    movl %eax, %edx
    call bytes2ascii
    push %ecx
    push %ebx
    call print_new_brk
    add $8, %esp

    cmpl $0, (head)          # |
    jne malloca_END          # |
    mov (initial_brk), %ebx  # |
    mov %ebx, (head)         # | if head is null, set it to initial_brk
    jmp malloca_END

  too_much:
    print $fmt_too_much $64
    mov $0, %eax
  malloca_END: # eax must already hold return value
    new_line
    popad                    #
    ret                      # | leave frame pointer

.section .note.GNU-stack,"",@progbits  # Prevent stack from being executable


# BOTH A and B produces the same machine code:
#
# A                                           movl (initial_brk), %eax
# 274  80492a2:       a1 10 c0 04 08          mov      0x804c010, %eax
#
# B                                           movl initial_brk, %eax
# 275  80492a7:       a1 10 c0 04 08          mov    0x804c010, %eax
#
# This will move THE CONTENT, not the value. To get the value, do
#
# ----------------------------------------->  movl $initial_brk, %eax
# 276  80492ac:       b8 10 c0 04 08          mov    $0x804c010, %eax
#
# The same when comparing
#                                             cmpl (initial_brk), %eax
# 274  80492a2:       3b 05 10 c0 04 08       cmp      0x804c010, %eax
#
#                                             cmpl initial_brk, %eax
# 275  80492a8:       3b 05 10 c0 04 08       cmp    0x804c010, %eax
#
#                                             cmpl $initial_brk, %eax
# 276  80492ae:       3d 10 c0 04 08          cmp    $0x804c010, %eax
#
# However, we cannot do
# cmpl $initial_brk, $0 (move to a register first)
