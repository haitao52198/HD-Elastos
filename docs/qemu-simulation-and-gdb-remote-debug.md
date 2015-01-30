qemu仿真与gdb远程调试
=====================

调试是程序开发中很重要的一环，IDE之所以能提升软件开发效率，很大一方面归功于其提供的强大调试功能。

好的调试工具能够很大程度节省debug的时间。虽然在最恶劣的环境下，程序员可能不得不用最原始的方式去做调试[^1]。
但是，懒惰的人总能创造出各种方法，让调试工作变得简单。

近期，在嵌入式内核移植的工作中遇到了相关的问题。本文主要探讨如何使用qemu仿真器和gdb的远程调试功能，
帮助开发者更好地进行内核级的断点调试。

## 问题分析

我们调试的工程是一个基于seL4微内核的测试工程HD-Elastos，工程的源码可以从以下git得到：

```shell
git clone http://elastos.org/review/HD-Elastos
```

这个工程是参考了[sel4test](https://github.com/seL4/sel4test)工程的代码结构而来的，
构建之后会在`goose/images`目录中生成一个elf格式的镜像。
但是在构建的过程中，`goose/build`目录和`goose/stage`目录中保存了中间生成的一些目标。
其中`goose/stage`目录包含了几个带gdb调试信息的重要目标：

-----------------------------------  ----------------------------------------
`arm/$(PLAT)/kernel.elf`             包含所有seL4内核的调试信息
`arm/$(PLAT)/bin/sel4test-driver`    包含主线程`sel4test-driver`的调试信息
`arm/$(PLAT)/bin/sel4test-tests`     包含主线程`sel4test-tests`的调试信息
-----------------------------------  ----------------------------------------

`goose/images`下生成的最终镜像是通过`make`脚本将以上内容封装在自己的section中，
而最终镜像的`.text`段包含着`goose/tools/elfloader`的内容（含调试信息）。

我们的策略是利用qemu仿真器执行`goose/images`中的镜像，
而在gdb调试器中加载需要调试的部分的符号，然后通过gdb的远程调试协议对qemu进行调试控制。
关于qemu和gdb合作远程调试的方法参考自CSDN的一篇博客。[^2]

## qemu仿真

我们首先需要通过增加参数设置为qemu开启gdb远程调试支持：

```shell
qemu-system-arm -M <your_marchine> [-nographics] -kernel <kernel_image> -S -s
```

参数解释：

----  ------------------------------------
`-S`  qemu启动时冻结CPU

`-s`  `-gdb tcp::1234`的简写，等待gdb通过
      TCP端口1234进行远程调试连接
----  ------------------------------------

## gdb远程调试

gdb支持来自远程TCP端口的调试控制，这一般需要远程通过gdbserver进行端口监听。[^3]
而qemu已支持gdbserver的端口监听功能，通过上面的方法运行qemu便可以进行远程调试。

### arm-none-eabi-gdb

但我们的镜像是面向arm架构的，所以必须使用面向arm指令集的gdb才能进行调试控制。

如果你使用的是debian衍生的linux发行版，apt软件源里应该已包含arm-none-eabi的完整工具链。
你可以直接安装：

```shell
sudo apt-get install gdb-arm-none-eabi
```

其它linux发行版类似。

> **NOTE:** 但是似乎Ubuntu的软件源里，对于该package的封包是有问题的。[^4]
> 似乎`gdb-arm-none-eabi`包中的`arm-none-eabi-gdb`被命名成了`gdb`，
> 和标准平台的`gdb`发生了名称冲突。
> 
> 你当然可以通过对package解包重命名的方式解决冲突，
> 如果你不希望因这样的操作而导致apt的依赖链断裂，
> 你也可以下载预编译的`arm-none-eabi`工具链。

[这里](http://elastos.org/download/files/DevelopmentTools/arm-2013.11-24-arm-none-eabi-i686-pc-linux-gnu.tar.bz2)
有一份可用的预编译包。你不需要安装，解包后直接调用的`bin`目录中的`arm-none-eabi-gdb`即可使用。

如果你没有安装其它版本的`gdb-arm-none-eabi`，你可以在搜索路径中建立一个软链接来指向`arm-none-eabi-gdb`。

```shell
tar jxvf arm-2013.11-24-arm-none-eabi-i686-pc-linux-gnu.tar.bz2 /opt/
ln -s /usr/bin/arm-none-eabi-gdb /opt/arm-2013-11/bin/arm-none-eabi-gdb
```

### remote debug

启动gdb：

```
localhost:~$ arm-none-eabi-gdb
GNU gdb (Sourcery CodeBench Lite 2012.09-63) 7.4.50.20120716-cvs
Copyright (C) 2012 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "--host=i686-pc-linux-gnu --target=arm-none-eabi".
For bug reporting instructions, please see:
<https://support.codesourcery.com/GNUToolchain/>.
(gdb) 
```

### 一个实例

## 配置调试前端

待续

### gdb for eclipse

### vimgdb


### remote debug

### 一个实例

## 配置调试前端

待续

### gdb for eclipse

### vimgdb

待续

### cgdb

待续

Reference
---------

[^1]: 引自《编程人生》
[^2]: [使用Qemu+gdb来调试内核](http://blog.csdn.net/iamljj/article/details/5655169)
[^3]: <http://en.wikipedia.org/wiki/Gdbserver>
[^4]: <https://bugs.launchpad.net/ubuntu/+source/gdb-arm-none-eabi/+bug/1267680>
