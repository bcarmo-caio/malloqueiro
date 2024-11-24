# We are working with m32 here, install the following packages
# apt install gcc-multilib g++-multilib libc6-dev-i386

# Sources to syscalls using int 0x80
# https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.6/+/refs/heads/jb-mr1-dev/sysroot/usr/include/asm/unistd_32.h
# https://github.com/spotify/linux/blob/master/arch/x86/include/asm/unistd_32.h
# https://www.ime.usp.br/~kon/MAC211/syscalls.html
# https://syscalls32.paolostivanin.com/


# Each memory chunk is designed as follows:
#  +-------------------------------------------+ high
#  |   Allocated                               | 0x0008 + n \
#  |   space                                   |  ...       |  USER SPACE (n bytes)
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
#         if chunk_userspace_size >= desired size + (metadata_size + 1)
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
str_already_init: .string "Malloqueiro has already been initialized\n"
str_not_init: .string "Malloqueiro has not yet been initialized\n"
str_debug: .string "Hi\n"
str_too_much: .string "Malloqueiro can not allocate more than 0x0000ffff bytes at once\n"
str_new_line: .string "\n"
str_printing_list: .string "Printing chunk list\n"
str_list_is_empty: .string "Chunk list is empty\n\n"
str_end_list: .string "End of chunk list\n\n"
metadata_size: .byte 8
debug_enabled: .byte 0

.section .data
initial_brk: .long 0 # null
head: .long 0 # null
tail: .long 0 # null

fmt_initial_brk_addr: .string "Initial brk @[0xZZZZZZZZ]\n\n" # 16, 27
fmt_brk_head_tail: .string "Brk @[0xZZZZZZZZ] - Head @[0xYYYYYYYY] - Tail @[0xHHHHHHHH]\n\n" # 8, 29, 50, 61
fmt_malloca: .string "Trying to allocate 0xZZZZ byte(s)\n" # 21, 34
fmt_chunk_info: .string "Chunk [0xYYYYYYYY] @[0xZZZZZZZZ] size (0xZZZZ + metadata) is (F)ree/(U)used: (Z)\n" # 9, 23, 41, 78, 81
fmt_freeing_mem: .string "Freeing chunk @[0xZZZZZZZZ]\n" # 18, 28
fmt_already_freed: .string "Error: Double free or corruption: 0xZZZZZZZZ\n" # 36, 45


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
  push %ecx # |  It makes no sense pushing eax and esp
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
  movl $45, %eax   # 45 is brk syscall
  movl $0, %ebx    # 0 means current brk
  int $0x80
  # new value is already in eax, return in eax
.endm

# This macro already considers metadata size, only pass me how many bytes
# the user wants
.macro increase_brk size
  get_cur_brk                 # |
  movl %eax, %ebx             # |
  addl \size, %ebx            # |
  addl (metadata_size), %ebx  # |
  movl $45, %eax              # |
  int $0x80                   # | set brk to current_brk + n_size + metadata size
  # new value is already in eax, return in eax
.endm

.macro reset_brk
  movl $0, (head)           # reset head
  movl $0, (tail)           # reset tail
  movl $45, %eax            # |
  movl (initial_brk), %ebx  # |
  int $0x80                 # | set brk to initial_brk
.endm

.macro debug
  pushad
  movl $4, %eax         # write sys call
  movl $1, %ebx         # stdout
  movl $str_debug, %ecx # const char *
  movl $3, %edx         # size_t
  int $0x80             # ssize_t write(stdout, "Hi\n", 3) MESSES WITH eax
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
  movl \most, %eax
  movl %eax, 0(\fmt_with_offset)
  # least
  movl \least, %eax
  movl %eax, 4(\fmt_with_offset)
.endm

.macro insert_ascii_into_string fmt_with_offset number
  movl \number, %eax
  movl %eax, (\fmt_with_offset)
.endm

.macro print char_p len
  movl $4, %eax
  movl $1, %ebx
  movl \char_p, %ecx
  movl \len, %edx
  int $0x80
.endm

.macro new_line
  print $str_new_line $1
.endm

.section .text

.globl malloca
.globl malloca_free

print_chunk_list: # no parameter, 2 4bytes var, no return value
    cmpb $0, (debug_enabled)
    je print_chunk_list_end

    push %ebp
    movl %esp, %ebp
    subl $8, %esp      # -4(ebp) = current chunk base address (node of the linked list)
                       # -8(ebp) = chunk index


    movl (head), %eax  # |
    cmpl $0, %eax      # |
    je  list_is_empty  # | if head is null, list is empty

    movl %eax, -4(%ebp) # move head to current chunk of linked list
    movl $1, -8(%ebp)   # init chunk index with 1

    print $str_printing_list, $20

  print_chunk_list_loop:
    movl -4(%ebp), %edx                        # get next chunk
    cmpl $0, %edx                              # |
    je print_chunk_list_end_prelude            # | if next is null, end function
    call bytes2ascii                           # get chunk address from edx in ascii (ebx)(ecx)
    movl $fmt_chunk_info, %edx                 # |
    addl $23, %edx                             # | fmt_chunk_info + 23 is our 0xzzzzzzzz location in string
    insert_ascii_into_string2 %edx %ecx %ebx

    movl -8(%ebp), %edx                        # |
    call bytes2ascii                           # |
    movl $fmt_chunk_info, %edx                 # |
    addl $9, %edx                              # |
    insert_ascii_into_string2 %edx, %ecx, %ebx # | put chunk index (ascii) into string

    movl -4(%ebp), %edx                        # |
    addl $5, %edx                              # | chunk size location
    movl (%edx), %edx                          # | put chunk size in edx
    andl $0x0000ffff, %edx                     # chunk size location has 3 bytes and we use 2
    call bytes2ascii                           # get chunk size in ascii (...)(ecx)
    movl $fmt_chunk_info, %edx                 # |
    addl $41, %edx                             # | fmt_chunk_info + 41 is our 0xZZZZ location in string
    insert_ascii_into_string %edx %ebx

    movl $fmt_chunk_info, %edx                 # |
    addl $78, %edx                             # | fmt_chunk_info + 78 is our (Z) location

    movl -4(%ebp), %eax                        # |
    movb (%eax), %al                           # |
    cmpb $0, %al                               # |
    je format_free                             # | get if chunk is free or in use

    movb $0x55, (%edx)                         # not free, U = 0x55
    jmp next_chunk

  format_free:
    movb $0x46, (%edx)                         # free, F = 0x46

  next_chunk:
    print $fmt_chunk_info $81                  # print string with current chunk info

    movl -4(%ebp), %eax                        # |
    addl $1, %eax                              # |
    cmpl $0, (%eax)                            # | if next chunk is null, print end of list and leave
    jne list_has_next                          # | else get next chunk

    print $str_end_list $19                    # |
    jmp print_chunk_list_end_prelude           # |

  list_has_next:
    addl $1, -8(%ebp)                          # chunk index++
    movl (%eax), %eax                          # |
    movl %eax, -4(%ebp)                        # | set next chunk variable (cur_node = cur_node->next)
    jmp print_chunk_list_loop                  # continue

  list_is_empty:
    print $str_list_is_empty $21

  print_chunk_list_end_prelude:
    movl %ebp, %esp # |
    pop %ebp        # |
  print_chunk_list_end:
    ret             # | leave frame

# 2 arguments:
#   least significant part of memory address in coded in ascii
#   most significant part of memory address in coded in ascii
# no local vars, no return value
print_initial_brk_addr: # void f(most, least)
    cmpb $0, (debug_enabled)
    je print_initial_brk_addr_end

    movl $fmt_initial_brk_addr, %ebx
    addl $16, %ebx # 16 is our offset here
    insert_ascii_into_string2 %ebx, 8(%esp), 4(%esp)
    print $fmt_initial_brk_addr, $27
  print_initial_brk_addr_end:
    ret

print_brk_head_tail: # no arguments, no local vars, no return value
    cmpb $0, (debug_enabled)
    je print_brk_head_tail_end

    get_cur_brk
    movl %eax, %edx                                  # |
    call bytes2ascii                                 # |
    movl $fmt_brk_head_tail, %edx                    # |
    addl $8, %edx                                    # 8 is 0xZZZZZZZZ
    insert_ascii_into_string2 %edx, %ecx, %ebx       # fill str with current brk

    movl (head), %edx                                # |
    call bytes2ascii                                 # |
    movl $fmt_brk_head_tail, %edx                    # |
    addl $29, %edx                                   # | 29 is 0xYYYYYYYY
    insert_ascii_into_string2 %edx, %ecx, %ebx       # fill str with head

    movl (tail), %edx                                # |
    call bytes2ascii                                 # |
    movl $fmt_brk_head_tail, %edx                    # |
    addl $50, %edx                                   # | 50 is 0xHHHHHHHH
    insert_ascii_into_string2 %edx, %ecx, %ebx       # fill str with tail

    print $fmt_brk_head_tail, $61
  print_brk_head_tail_end:
    ret

# 1 argument:
#   4 bytes integer coded in ascii
# no local vars, no return value
print_malloca:
    cmpb $0, (debug_enabled)
    je print_malloca_end

    push %ebp
    movl %esp, %ebp

    movl $fmt_malloca, %ebx
    addl $21, %ebx # 21 is our offset here
    insert_ascii_into_string %ebx, 8(%ebp)
    print $fmt_malloca, $34

    movl %ebp, %esp
    pop %ebp

  print_malloca_end:
    ret

# Converts the byte in dl to ascii and returns in al
_byte2ascii:
    push %edx
    andl $0x0000000f, %edx
    xorl %eax, %eax
    movb %dl, %al
    subb $10, %dl
    jge _bytes2ascii_letter
    addb $0x30, %al # 0x30 is ascii 0
    pop %edx
    ret
  _bytes2ascii_letter:
    addb $0x57, %al # 0x57 = 0x61 (ascii a) - 10 (10 chars previously handled, the numbers))
    pop %edx
    ret

# Receives a memory address in edx and returns the
# least significant part in ebx and the most significant part in ecx
bytes2ascii:
    xorl %ebx, %ebx
    xorl %ecx, %ecx

    # least significant
    call _byte2ascii
    shll  $24, %eax   # Little endian here. Store in it reverse
    orl   %eax, %ebx

    shrl  $4, %edx
    call _byte2ascii
    shll  $16, %eax
    orl   %eax, %ebx

    shrl  $4, %edx
    call _byte2ascii
    shll  $8, %eax
    orl   %eax, %ebx

    shrl  $4, %edx
    call _byte2ascii
    orl   %eax, %ebx

    # most significant
    shrl  $4, %edx
    call _byte2ascii
    shll  $24, %eax
    orl   %eax, %ecx

    shrl  $4, %edx
    call _byte2ascii
    shll  $16, %eax
    orl   %eax, %ecx

    shrl  $4, %edx
    call _byte2ascii
    shll  $8, %eax
    orl   %eax, %ecx

    shrl  $4, %edx
    call _byte2ascii
    orl   %eax, %ecx

    ret

# 1 argument: 4 bytes - Current chunk memory address
# No local var
# Return pointer to previous chunk or 0 if current chunk is head
get_previous_chunk:
    push %ebp
    movl %esp, %ebp

    movl 8(%ebp), %eax    # put cur_chunk in eax
    cmpl (head), %eax     # |
    je cur_chunk_is_head  # | if cur_chunk is head, return 0 (null)

    movl (head), %edx     # initialize loop with head
  gpc_loop_init:
    cmpl 1(%edx), %eax  # |
    je found_prev       # if edx->next is cur_chunk, return edx
    movl 1(%edx), %edx  # |
    jmp gpc_loop_init   # put edx->next in edx and loop again (if user has sent us wrong value, just go on and let it explode)

  found_prev:
    mov %edx, %eax    # |
    jmp get_prev_end  # | put edx (which is prev_chunk) in eax to return it

  cur_chunk_is_head:
    movl $0, %eax

  get_prev_end:
    movl %ebp, %esp
    pop %ebp
    ret

# 1 argument: 4 bytes - Current chunk memory address
# no local var
# Return pointer to previous chunk or 0 if current chunk is head
remove_chunk:
    push %ebp
    movl %esp, %ebp

    push 8(%ebp)             # | current chunk argument
    call get_previous_chunk  # |
    addl $4, %esp            # |
    movl %eax, %esi          # | get previous chunk in eax and backup it in esi

    cmpl $0, %eax       # |
    je merge_with_next  # | cannot merge with previous chunk, current chunk is head

    cmpb $0, (%eax)     # |
    jne merge_with_next # | cannot merge with previous chunk, it is not free

  merge_with_previous:
    # BEGIN merge previous and current chunks
    movl (head), %ebx   # |
    movl 1(%ebx), %ebx  # | ebx = head -> next
    cmpl %ebx, (tail)   # |
    jne L1              # |
    reset_brk           # |
    jmp RC_END          # | if head -> next == tail, reset brk, head, tail and leave

  L1:
    movl 8(%ebp), %ebx  # ebx = cur_chunk
    movl 1(%ebx), %ecx  # ecx = cur_chunk -> next
    movl %ecx, 1(%esi)  # prev_chunk -> next = cur_chunk -> next

    movl $0, %ecx
    movl $0, %edx
    movw 5(%eax), %cx   # cx = prev_chunk -> size
    movw 5(%ebx), %dx   # dx = cur_chunk -> size

    # TODO: CHECK FOR OVERFLOW LATTER
    addw %cx, %dx             # |
    addw (metadata_size), %dx # |
    movw %dx, 5(%eax)         # | previous_chunk -> size = (prev_chunk -> size) + (cur_chunk -> size) + metadata_size

    movl %esi, 8(%ebp)  # update current_chunk argument before going to post_merge_prev
    cmpl (tail), %ebx   # |
    jne merge_with_next # | if current chunk is not tail, do not change tail or brk

    movl %eax, (tail) # tail = prev_chunk
    movl %eax, %ebx   # |
    movl $45, %eax    # |
    int $0x80         # | lower brk to new tail value

    jmp RC_END
    # END merge previous and current chunks

  merge_with_next:
    movl 8(%ebp), %eax  # put cur_chunk in eax
    cmpl %eax, (tail)   # |
    je cur_is_tail      # | cannot merge with next chunk, current chunk is tail. Lower brk and set new tail

    movl 1(%eax), %ebx # put next_chunk in ebx
    cmpb $0, (%ebx)    # |
    jne RC_END         # | cannot merge with next chunk, it is not free

    # BEGIN merge current and next chunks
    # Note that as next chunk is free, it cannot be the tail hence we are not going
    # to lower the brk
    movl 1(%ebx), %ecx  # ecx = cur_chunk -> next
    movl %ecx, 1(%eax)  # cur_chunk -> next = next_chunk -> next

    movl $0, %ecx
    movl $0, %edx
    movw 5(%eax), %cx   # cx = cur_chunk -> size
    movw 5(%ebx), %dx   # dx = next_chunk -> size

    # TODO: CHECK FOR OVERFLOW LATTER
    addw %cx, %dx             # |
    addw (metadata_size), %dx # |
    movw %dx, 5(%eax)         # | cur_chunk -> size = (cur_chunk -> size) + (next_chunk -> size) + metadata_size
    jmp RC_END

  cur_is_tail:
    movl %esi, (tail)    # set tail as prev_chunk
    movl $0, 1(%esi)     # |
    movl -4(%ebp), %ebx  # |
    movl $45, %eax       # |
    int $0x80            # | lower brk to cur_chunk
    # END merge current and next chunks

  RC_END:
    movl %ebp, %esp
    pop %ebp
    ret

malloca_free: # 1 argument, no local var, no return value. void f(void *ptr)
    pushad                                     # | we are comming from C, push all registers
    movl %esp, %ebp                            # | init frame pointer
    subl $4, %esp                              # store chunk address (ptr - metadata_size)

    movl 28(%ebp), %ebx                        # get pointer argument
    subl (metadata_size), %ebx                 # set memory address to 'chunk in use' place
    movl %ebx, -4(%ebp)

    cmpb $0, (debug_enabled)
    je skip_print_malloca_free

    movl %ebx, %edx                            # |
    call bytes2ascii                           # |
    movl $fmt_freeing_mem, %edx                # |
    addl $18, %edx                             # |
    insert_ascii_into_string2 %edx, %ecx, %ebx # |
    print $fmt_freeing_mem, $28                # | print debug msg

  skip_print_malloca_free:
    movl -4(%ebp), %ebx                        # |
    movb (%ebx), %al                           # |
    cmpb $0, %al                               # |
    je already_freed                           # | if already freed, print err and abort

    movl (head), %ebx                          # |
    cmpl %ebx, (tail)                          # |
    je reset_brk_and_leave                     # | checking if heap has only 1 chunk

    movl -4(%ebp), %ebx                        # |
    movb $0, (%ebx)                            # | set chunk as free

    push %ebx
    call remove_chunk
    addl $4, %esp
    jmp malloca_free_end


  reset_brk_and_leave:
    reset_brk

  malloca_free_end:
    call print_brk_head_tail
    call print_chunk_list
    movl %ebp, %esp
    popad
    ret

  already_freed:
    movl 28(%ebp), %edx                        # get pointer argument
    call bytes2ascii                           # |
    movl $fmt_already_freed, %edx              # |
    addl $36, %edx                             # |
    insert_ascii_into_string2 %edx, %ecx, %ebx # | format string
    print $fmt_already_freed, $45              # print error message
    movl $1, %eax                              # |
    movl $1, %ebx                              # |
    int $0x80                                  # | abort program

malloca_init: # no arguments, no local vars, no return value
    cmpl $0, (initial_brk)   # |
    je malloca_init_proceed  # | if initial_brk variable is 0, proceed. Log and abort otherwise
    print $str_already_init, $41
    jmp malloca_init_end
  malloca_init_proceed:
    get_cur_brk
    movl %eax, (initial_brk)    # save initial brk value to initial_brk variable
    movl %eax, %edx
    call bytes2ascii
    push %ecx
    push %ebx
    call print_initial_brk_addr
    addl $8, %esp
  malloca_init_end:
    ret

get_free_chunk: # 1 4b argument, no local var, pointer return value (may be null)
    cmpl $0, head         #
    je no_free_chunk      # if head is null, return null
  no_free_chunk:  # no free chunk, return null
    movl $0, %eax
    ret

malloca: # 1 4b argument, 1 4b local var, pointer return value (may be null)
    pushad                    # | we are comming from C, push all registers
    movl %esp, %ebp           # | init frame pointer
    subl $4, %esp             # value to be returned

    cmpl $0, initial_brk # |
    jne  initialized     # | if initial_brk is variable 0, init. Proceed otherwise
    call malloca_init

  initialized:
    # remember that we pushed all registers but eax and esp , so 24 + 4 (ret address) = 28
    movl 28(%ebp), %eax       # get bytes argument
    movl %eax, %edx
    call bytes2ascii
    push %ebx
    call print_malloca
    addl $4, %esp

    movl 28(%ebp), %eax        # |
    subl $0x0000ffff, %eax     # |
    jg too_much                # | assure user did not ask for a block greater than 0x0000ffff

    push 28(%ebp)              # |
    call get_free_chunk        # |
    addl $4, %esp              # | get_free_chunk(size_t n_size)

    cmpl $0, %eax              # |
    jne malloca_end            # | if get_free_chunk could get a chunk, return it
                               # | (head already exists at this point)

    increase_brk 28(%ebp)      # increase brk by the ammount user wants (28 ebp) (plus metadata)

    movl %eax, %edx            # |
    subl 28(%ebp), %edx        # | actual user pointer is brk - size
    movl %edx, -4(%ebp)        # store actual user pointer at return value

    movl 28(%ebp), %ecx        # |
    movb $0, -1(%edx)          # | (zero this part we won't use from a 3 bytes data)
    movw %cx, -3(%edx)         # | set chunk size excluding metadata into chunk metadata
    movl $0, -7(%edx)          # set next chunk as null into chunk metadata
    movb $1, -8(%edx)          # set chunk in use into chunk metadata

    cmpl $0, (head)            # |
    jne set_tail               # |
    movl (initial_brk), %ebx   # |
    movl %ebx, (head)          # |
    movl %ebx, (tail)          # | if head is null, set both head and tail to initial_brk
    jmp malloca_end

  set_tail:
    movl -4(%ebp), %ebx        # |
    subl (metadata_size), %ebx # | memory address where chunk begins
    movl (tail), %ecx          #
    movl %ebx, 1(%ecx)         # | tail->next = new chunk
    movl %ebx, (tail)          # | tail = new chunk
    jmp malloca_end

  too_much:
    print $str_too_much $64
    movl $0, -4(%ebp)          # puts null to return value

  malloca_end:
    call print_brk_head_tail
    call print_chunk_list

    movl -4(%ebp), %eax        # put return value into eax
    movl %ebp, %esp            # |
    popad                      # |
    ret                        # | leave frame pointer

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
