  #################################################
# squares.asm – 计算 0..200 的平方表
# 运行到末尾自动退出，不再无限循环
#################################################

    .text
    .globl main

main:
    # 寄存器约定：
    # $s0 = i
    # $s1 = last_square
    # $s4 = limit (201)
    # $s5 = base address of array

    li   $s0, 0          # i = 0
    li   $s1, 0          # last_square = 0
    li   $s4, 201        # limit = 201
    la   $s5, array      # 加载 array 的基址

    # 存入 0^2 到 array[0]
    sw   $s1, 0($s5)

loop:
    addi $s0, $s0, 1     # i++
    beq  $s0, $s4, done  # 如果 i == 201，跳到 done

    # 计算 2*i - 1
    addu $t0, $s0, $s0   # t0 = 2 * i
    addi $t0, $t0, -1    # t0 = 2*i - 1

    # new_square = last_square + (2*i - 1)
    addu $s2, $s1, $t0   # s2 = 新平方值

    # 计算写入地址 = base + i*4
    sll  $t1, $s0, 2     # t1 = i * 4
    add  $t1, $t1, $s5   # t1 = base + offset
    sw   $s2, 0($t1)     # 存入 array[i]

    move $s1, $s2        # last_square = s2
    j    loop            # 回到 loop

done:
    # 调用 syscall 10 退出
    li   $v0, 10
    syscall

    .data
    .align 2
array:
    .space 804           # 201 * 4 字节，为 0..200 的平方表预留空间
