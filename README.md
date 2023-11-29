c4 - C in four functions
========================

An exercise in minimalism.

Try the following:

    gcc -o c4 c4.c
    ./c4 hello.c
    ./c4 -s hello.c

    ./c4 c4.c hello.c
    ./c4 c4.c c4.c hello.c

# c4 - C in 4 functions 虚拟机分析

## 寄存器

```C
int *pc, *sp, *bp, a, cycle; 
```

- `pc` 程序计数器/指令指针
- `sp` 堆栈寄存器,指向栈顶,栈由高地质向低地址生长
- `bp` 基址寄存器
- `a` 累加器
- `cycle` 执行指令计数

## 指令集

1. `LEA` 去局部变量地址, 以`[PC+1]`地址, 以`bp`为基址, **地址**载入累加器 `a`
2. `IMM` `[PC+1]`作为为立即数载入累加器 `a`
3. `JMP` 无条件跳转到`[PC+1]`
4. `JSR` 进入子程序, 将 `PC+2` 入栈作为返回地址,跳转到`[PC+1]`
5. `BZ`  累加器为零时分支, 当累加器 `a` 为 0 时跳转到`[PC+1]`, 不为零时跳转到 `PC+2` 继续执行
6. `BNZ` 累加器不为零时分支, 当累加器 `a` 不为 0 时跳转到`[PC+1]`, 为零时跳转到 `PC+2`
7. `ENT` 进入子程序, 将`bp`压栈, 基址`bp`指向栈顶, 然后将栈顶生长`[PC+1]`字, 作为参数传递空间
8. `ADJ`
9. `LEV` 离开子程序, 堆栈指针`sp = bp`, 从堆栈中弹出基址`bp`, `pc`
10. `LI` 以 `a` 为地址取**int**数
11. `LC` 以 `a` 为地址取**char**
12. `SI` 以栈顶为地址存**int**数并弹栈`[[sp++]]=a`
13. `SC` 以栈顶为地址存**char**并弹栈`[[sp++]]=a`
14. `PSH` 将 `a` 压栈
15. `OR` `a = [sp++] | a`
16. `XOR` `a = [sp++] ^ a`
17. `AND` `a = [sp++] & a`
18. `EQ`  `a = [sp++] == a`
19. `NE`  `a = [sp++] != a`
20. `LT`  `a = [sp++]  a`
21. `GT`  `a = [sp++] > a`
22. `LE`  `a = [sp++] <= a`
23. `GE`  `a = [sp++] >= a`
24. `SHL` `a = [sp++] << a`
25. `SHR` `a = [sp++] >> a`
26. `ADD` `a = [sp++] + a`
27. `SUB` `a = [sp++] - a`
28. `MUL` `a = [sp++] * a`
29. `DIV` `a = [sp++] / a`
30. `MOD` `a = [sp++] % a`
31. `OPEN` 调用 C 库函数`open`, 堆栈传递 2 个参数(第一个参数先进栈, 返回值存入累加器 `a`, 下同)
32. `READ` 调用 C 库函数`read`, 堆栈传递 2 个参数
33. `CLOS` 调用 C 库函数`close`, 堆栈传递 2 个参数
34. `PRTF` 调用 C 库函数`printf`, `[pc+1]`表明参数个数, 传递至多六个参数
35. `MALC` 调用 C 库函数`malloc`, 堆栈传递一个参数
36. `MSET` 调用 C 库函数`memset`, 堆栈传递 3 个参数
37. `MCMP` 调用 C 库函数`memcmp`, 堆栈传递 3 个参数
38. `EXIT` 打印虚拟机执行情况, 返回`[sp]`
