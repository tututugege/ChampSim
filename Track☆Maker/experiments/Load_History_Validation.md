# Track☆Maker Load History 分类验证

## 目标

验证在 x86 上加入“地址相关源寄存器过滤”之后，Track☆Maker 的 address-history classifier 是否能够区分几类常见 load 模式。

## 关键修正

x86 上暴露出来的核心问题：

- 像 `add rdx, [rax]` 这样的宏指令同时包含：
  - 用 `rax` 生成地址
  - 从 `[rax]` 发起 load
  - 用 load 回来的数据更新 `rdx`
- 如果在做 load 分类时把所有源寄存器 history 全部 merge，`rdx` 这条“数据累加链”会污染地址 history，导致本来应当是 affine 的 load 被误判成 load-dependent。

当前在 [src/ooo_cpu.cc](/home/tututu/my_arch/ChampSim/src/ooo_cpu.cc) 中采用的修正规则：

- 做 load 地址分类时，优先只看这些源寄存器：
  - 参与这条指令
  - 不是特殊寄存器（`FLAGS`、`IP`）
  - 不会被这条指令自己写回
- 如果过滤后没有剩下合适的源寄存器，再回退到原来的 merged source history

这样做不会改变 value-history 的传播语义，只是让 load 地址分类更聚焦于真正携带地址信息的寄存器。

## Demo 程序

使用 [Track☆Maker/demo](</home/tututu/my_arch/ChampSim/Track☆Maker/demo>) 下的 load-heavy 微基准：

- `load_seq_sum.c`
- `load_stride_sum.c`
- `load_gather_sum.c`
- `load_chase_sum.c`

统一编译方式：

```bash
cc -O2 -std=c11 -Wall -Wextra -pedantic -fno-pie -no-pie ...
```

## Hot Loop 入口前动态指令数

通过 PIN tracer 的辅助计数模式测得：

- `load_seq_sum`
  - hot load IP `0x4010d8`
  - `instructions_before_target = 5377427`
- `load_stride_sum`
  - hot load IP `0x4010d8`
  - `instructions_before_target = 25300370`
- `load_gather_sum`
  - hot load IP `0x401120`
  - `instructions_before_target = 7474880`
- `load_chase_sum`
  - hot load IP `0x401198`
  - `instructions_before_target = 8261344`

## 使用的 Trace 窗口

本轮验证使用的 hot-loop 小窗口：

- `load_seq_sum`
  - `skip=5500000`, `trace=50000`
- `load_stride_sum`
  - `skip=25500000`, `trace=50000`
- `load_gather_sum`
  - `skip=7600000`, `trace=50000`
- `load_chase_sum`
  - `skip=8400000`, `trace=50000`

## 结果

### `load_seq_sum`

- `Load-classified instructions: 7470`
- `AFFINE: 7464 (99.92%)`
- `load-derived: 0`
- 热点 load PC: `0x4010d8`

解释：

- 简单顺序求和现在已经能稳定分类为 affine，而不会再被误判成 load-dependent。

### `load_stride_sum`

- `Load-classified instructions: 5948`
- `AFFINE: 5921 (99.55%)`
- `load-derived: 0`
- 热点 load PC: `0x4010d8`

解释：

- 固定 stride 的 load 也能稳定归到 affine。

### `load_gather_sum`

- `Load-classified instructions: 11895`
- `AFFINE: 5921 (49.78%)`
- `DEP_DEEP: 5919 (49.76%)`
- 热点 load PC：
  - `0x401120` dominant `AFFINE`
  - `0x401126` dominant `DEP_DEEP`

解释：

- 读取 `index[i]` 的那条 load 是 affine。
- 根据 `index[i]` 再去读取 `data[index[i]]` 的那条 load 是 load-derived，因此被归到 deep dependent。
- 这正好符合 gather 的预期行为。

### `load_chase_sum`

- `Load-classified instructions: 14869`
- `DEP_DEEP: 14800 (99.54%)`
- `load-derived: 14802`
- 热点 load PC：
  - `0x401198`
  - `0x40119c`
  两者都 dominant `DEP_DEEP`

解释：

- pointer chasing 能够被稳定识别成 load-dependent。

## 当前结论

现在这套 classifier 已经能够区分：

- affine / stride-like load
- dependent indirect load
- 同时包含 affine load 和 dependent load 的 gather kernel

这已经足够支持下一阶段工作：

- 更系统地整理实验结果，或者
- 把分类结果真正接到 prefetch selection 上

## 当前限制

- 目前仍然是宏指令级别的 x86 tracing，不是真正的 uop tracing
- 现在的 x86 处理仍然是启发式，不是完整的 operand-role 精确建模
- 如果后面要进一步提升可信度，tracer 最好显式携带这些 memory operand 角色信息：
  - base register
  - index register
  - scale
