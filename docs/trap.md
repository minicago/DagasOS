# 中断和异常处理

## 目标

1) 能正常进入并退出处理程序。

## 步骤

1) start中实现将中断交给S-mode代理。

2) 在kernelvec中实现上下文的保存并进入处理程序。

3) 设置CSR，以进入stvec。

## S-mode代理

### CSR 寄存器

medeleg : 机器异常委托寄存器

mideleg : 机器中断委托寄存器

sie : S-mode下能处理的中断

## kernelvec

### kernelvec 为什么必须用RISCV64汇编实现？

因为正常的C函数会将函数地址压入系统栈，破坏context。

## 进入stvec

设置csr中stvec的值



