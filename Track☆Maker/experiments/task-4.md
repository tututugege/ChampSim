# Track☆Maker 

## 1. Idea

核心想法：  
我们希望追踪 **load 地址是如何被计算出来的历史**（address/value history），来判断这条 load 更像哪一类访存模式，并据此辅助预取。

直觉上：

- 如果地址历史主要是 `ADD/LEA` 这类规则组合，通常更接近 `stride/affine`
- 如果地址历史里有明显的 `load -> load` 依赖链，通常更接近间接访存（pointer chasing / gather 的第二跳）



## 2. 问题定义

我们当前要解决的问题不是“做一个全新预取器”，而是先回答这个更基础的问题：

> 仅用轻量的地址/值历史信息，能否把不同 load 模式（规则 vs 间接）稳定区分开？

更具体地说：

- 输入：源寄存器计算op历史
- 输出：每条 load 的类别（如 `AFFINE`、`DEP_1`、`DEP_DEEP` 等）
- 目标：分类结果要和程序语义一致，使得不同的load分流触发不同的预取器，提高预取效率，减少噪声和污染

## 3. 当前的一些小实验

### 3.1 基于ChampSim

由于ChampSim本身的Trace不带op信息，我们做了两件事：

0. 在 tracer 侧输出原始 trace 之外的 `optrace` sidecar（不破坏原 ChampSim trace 格式）
1. 在 ChampSim 核心内维护一个轻量 history 状态，并对 load 做模式分类

当前原型的关键机制：

- op 分类 token（由 PIN tracer 生成）
- history 编码（固定长度、按 token 左移追加）
- `mov` 继承历史、双源操作选更“复杂”源再追加当前 op
- 对 load 的地址分类使用“地址相关源寄存器过滤”规则，避免数据寄存器污染地址链

我们目前输出的类别：

- `SIMPLE`
- `AFFINE`
- `DEP_1`
- `DEP_DEEP`
- `COMPLEX`
- `UNKNOWN`


#### 在 toy 程序上

我们已经稳定区分：

- `load_seq_sum` / `load_stride_sum` -> 以 `AFFINE` 为主
- `load_chase_sum` -> 以 `DEP_DEEP` 为主
- `load_gather_sum` -> 同时出现 `AFFINE`（第一跳）和 `DEP_DEEP`（第二跳）

这说明分类器对“规则 vs 间接”有基本辨识能力。

#### 在真实 benchmark 上（small test）

- PolyBench:
  - `atax`, `mvt`：基本以 `AFFINE` 为主
  - `bicg`：同一 kernel 内出现混合模式（`AFFINE` + `DEP_DEEP`）
- GAPBS:
  - `bfs`, `pr`：明显混合（`AFFINE` + `DEP_1` + `DEP_DEEP`）

这说明 signal 不只存在于 toy demo，在真实工作负载中也可观测。


### 3.2 基于RV32IMA小模拟器

- 跑了coremark-pro，计算历史确实能在一定程度上够区分出不同的访存模式
- 在当前的识别过程中确实存在同一个pc被分到不同的访存模式的情况（目前还没确实是程序的行为还是识别错了）
- 被识别为不同访存模式的load确实在Cache命中率上有明显区别


## 4. 相关工作

### 4.1 ASPLOS 2020

`Classifying Memory Access Patterns for Prefetching`  
Grant Ayers, Heiner Litz, Christos Kozyrakis, Parthasarathy Ranganathan  
ASPLOS 2020, DOI: `10.1145/3373376.3378498`

这篇工作的关键点是：

- 用数据流信息提取内存访问模式
- 将复杂地址生成归纳为可分类的 pattern/prefetch kernel
- 静态识别计算链 + 软预取

与我们的关系：

- 我们的核心方向与其高度一致，都是“先分类，再谈预取策略”
- 但是我们当前实现是更轻量的在版本 + 硬件预取

### 4.2 ASPLOS 2026

`PF-LLM: Large Language Model Hinted Hardware Prefetching`  
ASPLOS 2026（Best Paper）

这篇工作的代表性意义：

- 把 LLM 作为 hint 生成器，参与预取决策链路
- LLM通过识别load的局部上下文（论文里面是前后128条）选出最优预取器
- LLM识别后在运行前将hint写到硬件里，程序运行时直接查表

与我们的关系：

- 我们倾向于做硬件上更优雅的在线识别（可解释、低开销）”
- PF-LLM 代表的是“更高层模型信号（潜在更强表达）”
- 两者可以看作同一方向上不同复杂度层级的方法


## 5. 当前主要问题与技术债

### 5.1 ChampSim 原生 trace 的信息缺口

原生 ChampSim trace 不提供完整 op 语义，因此我们额外做了 `optrace` sidecar。  
这让流程多了一步“对齐与合并”，并带来工程复杂度。

### 5.2 不是 uop 级建模

ChampSim的执行Trace是 **macro-instruction 级**，不是 uop 级。  
这对 x86 尤其敏感，因为 x86 一条宏指令可同时包含地址生成、load 和算术更新。

后续实验可能还是得上Gem5

### 5.3 如果不是uop，三操作数/混合语义指令带来的歧义

典型例子：`add rdx, [rax]`

- `rax` 是地址链
- `rdx` 是数据累加链

如果不区分“源操作数角色”，会把数据链误并入地址链，造成误判。  
我们目前用启发式规则缓解（过滤会被本条指令写回的源寄存器），但这不是完整 operand-role 精确建模。


### 5.4 操作数历史的合并

即使把X86译码后的结果全部看作类似risc的双操作数uop，大部分运算都可能涉及双寄存器操作数，src1 op src2 = dest，目前dest的计算历史定义为src1和src2中更复杂的历史 + op，但是这样的合并可能会丢失一些关键信息。

### 5.5 历史信息的格式是什么样的

目前想到两种方案，一种是简单记录每个areg的历史，比如总共定义8种op，每个areg维护24bit历史，能记录8个历史，这种方案问题在于如果一个寄存器作为一个基地址被反复计算了多次（比如遍历a[100]，每次都会给一个基地址加一个偏移），历史可能塞不下，历史太长可能引入一些噪声。

另一种是记录preg的历史，固定为[load + load_info, mul + mul_info, add + add_info]，涉及到加法时可以在add_info里面记录stride，涉及乘法时同理，假如计算历史有load，目前没想好怎么处理，至少可以知道可能是一个间接访存。这种方案比上一种方案多了计算的值的信息，前面是areg级别，可以在重命名的时候和重命名表放到一起，这个方案是挂在preg上，读源操作数的时候读取，但是这些metadata会增加旁路网络的开销。

实际上计算历史有load也不一定就是间接访存，例如load过来一个基地址，然后顺序遍历，每次访存都有load历史，但是后面都是干净的顺序访问，也有一些解决办法，比如load_info加一个复用bit，如果历史有load，并且已经被load了一次，那么就标记这个load的结果被复用了。

### 5.6 历史信息区分load访问类型，准确率多高

需要进一步实验验证，好在这一步只需要的简单单周期CPU就能测，根据`Classifying Memory Access Patterns for Prefetching`来看好像也不需要很准？


### 5.7 识别了load类型，然后呢？

参考`PF-LLM`，能够把不同load分流到不同的预取器的收益还是很大，但是我们硬件在线识别，没法像它那样“作弊”直接看前后上下文，收益肯定缩水不少，可能考虑多利用这些metadata去优化预取器，比如我们可以在计算的时候提前识别stride

### 5.8 相较于其他研究有啥优势

前两篇ASPLOS都算软硬件结合的办法，我们纯硬件更“优雅”一些。
另外，我们动态识别一个很大的优势在于，如果一个指令在动态运行过程中属于不同的类型，比如可能一次是间接访存，一次是普通stride，静态分析的办法无法区分，但是我们基于计算历史的方式可以区分，当然这种应该还是少数事件。

## 6. 下一步做什么

- 进一步跑GAPBS，SPEC 2006部分程序中的部分片段，对照每个load的源代码标记对应的访存模式，以此确认我们的分类的准确率

- 确认我们分类后分流到不同预取器能带来性能收益

- 确认同PC但是具有不同访存行为的现象是不是存在（这是我们动态识别最大的优势）
