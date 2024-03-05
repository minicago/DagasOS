# 进入S态

## 目标

1) 掌握CSR寄存器的功能，编写对CSR寄存器操作的C语言库

2) 实现从entry跳转到start的功能，在start中实现进入S态。

3) 实现从start跳转系统main函数,在main函数中实现其他功能的初步测试。

## 步骤

1) 查阅CSR寄存器的功能，了解S态下CSR寄存器的设置。

## 参考资料

[1] RISC-V 手册 - 中国科学技术大学
[2] The RISC-V Instruction Set Manual - RISC-V International

## 学习 CSR 寄存器

### M 模式

机器模式（缩写为 M 模式，M-mode）是 RISC-V 中 hart（hardware thread，硬件线程）可以执行的最高权限模式。在 M 模式下运行的 hart 对内存，I/O 和一些对于启动和配置系统来说必要的底层功能有着完全的使用权。

机器模式最重要的特性是拦截和处理异常（不寻常的运行时事件）的能力。RISC-V 将异常分为两类。一类是同步异常(synchronous exception)，这类异常在指令执行期间产生，如访问了无效的存储器地址或执行了具有无效操作码的指令时。另一类是中断，它是与指令流异步的外部事件，比如鼠标的单击。RISC-V 中实现精确异常处理：保证异常之前的所有指令都完整地执行了，而后续的指令都没有开始执行（或等同于没有执行）。

### CSR 寄存器

M模式下八个控制状态寄存器（CSR）是机器模式下异常处理的必要部分：
1) mtvec（Machine Trap Vector）它保存发生异常时处理器需要跳转到的地址。
2) mepc（Machine Exception PC）它指向发生异常的指令。
3) mcause（Machine Exception Cause）它指示发生异常的种类。
4) mie（Machine Interrupt Enable）它指出处理器目前能处理和必须忽略的中断。
5) mip（Machine Interrupt Pending）它列出目前正准备处理的中断。
6) mtval（Machine Trap Value）它保存了陷入（trap）的附加信息：地址例外中出错的地址、发生非法指令例外的指令本身，对于其他异常，它的值为 0。
7) mscratch（Machine Scratch）它暂时存放一个字大小的数据。
8) mstatus（Machine Status）它保存全局中断使能，以及许多其他的状态。

详见 [RISCV-Interrupts](https://riscv.org/wp-content/uploads/2016/07/Tue0900_RISCV-20160712-Interrupts.pdf)

在S模式下也有S模式的CSR。


### CSR 操作

#### CSRRW rd csr rs1
Atomic Read/Write CSR
``` c
atomic {
    rd = csr
    csr = rs1
}
```

#### CSRRS rd csr rs1
Atomic Read and Set Bits in CSR
``` c
atomic {
    rd = csr
    csr |= rs1
}
```

#### CSRRC rd csr rs1
Atomic Read and Clear Bits in CSR
``` c
atomic {
    rd = csr
    csr &= ~rs1
}
```

#### (CSRRWI, CSRRSI or CSRRCI) rd csr imm

上述操作I型指令版

### 内联汇编
``` c
__asm__ __volatile__ ("asm"
: "=r,m or i"(output)
: "r,m or i"(input)
: "memory" );
```

r 表示用寄存器导入
m 表示用memory导入
i 表示用立即数导入
__volatile__ 禁止优化

“memory” 表示内存会修改

### 进入S态的准备

我们将在start函数中进行CSR寄存器的设置，但需要先设置堆栈。

