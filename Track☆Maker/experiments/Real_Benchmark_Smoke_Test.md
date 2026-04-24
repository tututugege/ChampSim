# Track☆Maker 真实 Benchmark 冒烟验证

## 目标

在不引入 SimPoint 的前提下，先用几组真实 benchmark 做最小验证：

- `PolyBench/atax`：规则 affine 访存
- `PolyBench/bicg`：规则 dense kernel，但同时包含多条不同角色的 load
- `PolyBench/mvt`：规则矩阵向量乘加访存
- `GAPBS/bfs`：图遍历中的不规则、load-dependent 访存
- `GAPBS/pr`：图迭代中的混合型访存

这一步的目的不是给出最终评测结果，而是确认 Track☆Maker 在真实程序上仍然能看到有意义的 load 分类信号。

## 编译与输入

### PolyBench `atax`

编译产物：

- `/tmp/trackmaker_atax`

编译方式：

```bash
gcc -O3 -fno-pie -no-pie \
  -I Track☆Maker/benchmark/polybench-c-3.2/utilities \
  -I Track☆Maker/benchmark/polybench-c-3.2/linear-algebra/kernels/atax \
  Track☆Maker/benchmark/polybench-c-3.2/utilities/polybench.c \
  Track☆Maker/benchmark/polybench-c-3.2/linear-algebra/kernels/atax/atax.c \
  -o /tmp/trackmaker_atax
```

### GAPBS `bfs`

使用小图输入，避免图生成和建图阶段把前置成本拉得过大：

- 程序：`Track☆Maker/benchmark/gapbs/bfs`
- 输入：`-g 10 -n 1`

编译方式：

```bash
make clean
make SERIAL=1 CXX='g++ -fno-pie -no-pie' bfs
```

### PolyBench `bicg`

编译产物：

- `/tmp/trackmaker_bicg`

编译方式：

```bash
gcc -O3 -fno-pie -no-pie \
  -I Track☆Maker/benchmark/polybench-c-3.2/utilities \
  -I Track☆Maker/benchmark/polybench-c-3.2/linear-algebra/kernels/bicg \
  Track☆Maker/benchmark/polybench-c-3.2/utilities/polybench.c \
  Track☆Maker/benchmark/polybench-c-3.2/linear-algebra/kernels/bicg/bicg.c \
  -o /tmp/trackmaker_bicg
```

### GAPBS `pr`

使用小图输入：

- 程序：`Track☆Maker/benchmark/gapbs/pr`
- 输入：`-g 10 -n 1`

编译方式：

```bash
make clean
make SERIAL=1 CXX='g++ -fno-pie -no-pie' pr
```

### PolyBench `mvt`

编译产物：

- `/tmp/trackmaker_mvt`

编译方式：

```bash
gcc -O3 -fno-pie -no-pie \
  -I Track☆Maker/benchmark/polybench-c-3.2/utilities \
  -I Track☆Maker/benchmark/polybench-c-3.2/linear-algebra/kernels/mvt \
  Track☆Maker/benchmark/polybench-c-3.2/utilities/polybench.c \
  Track☆Maker/benchmark/polybench-c-3.2/linear-algebra/kernels/mvt/mvt.c \
  -o /tmp/trackmaker_mvt
```

## Hot Loop / Hot Function 入口

### `atax`

`objdump` 显示 `main` 中的关键内层循环位于：

- `0x401320`: `movsd (%r15,%rax,1), %xmm0`
- `0x401326`: `mulsd 0x0(%r13,%rax,1), %xmm0`

这是一段很典型的规则双 load 乘加内环。

通过 PIN 计数模式测得：

- `target_ip = 0x401320`
- `instructions_before_target = 60225767`

本轮窗口：

- `skip = 60225000`
- `trace = 50000`

trace 文件：

- [Track☆Maker/traces/real/atax_hot.champsim](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/atax_hot.champsim>)
- [Track☆Maker/traces/real/atax_hot.optrace](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/atax_hot.optrace>)

### `bfs -g 10 -n 1`

先测得：

- `main = 0x403a20`
- `instructions_before_main = 2185065`

再用符号表和反汇编定位 BFS 核心：

- `DOBFS = 0x4057c0`
- `BUStep = 0x405330`
- `TDStep = 0x405420`

对小图输入测得：

- `target_ip = 0x405420`
- `instructions_before_target = 13026160`

本轮窗口：

- `skip = 13026000`
- `trace = 50000`

trace 文件：

- [Track☆Maker/traces/real/bfs_g10_tdstep_hot.champsim](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/bfs_g10_tdstep_hot.champsim>)
- [Track☆Maker/traces/real/bfs_g10_tdstep_hot.optrace](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/bfs_g10_tdstep_hot.optrace>)

### `bicg`

`objdump` 显示 `main` 中的关键热段位于：

- `0x401340`: `movsd (%rcx), %xmm0`
- `0x401344`: `mulsd (%r14,%rax,1), %xmm0`
- `0x40134a`: `addsd (%r15,%rax,1), %xmm0`
- `0x401356`: `movsd (%r14,%rax,1), %xmm0`
- `0x40135c`: `mulsd (%r12,%rax,1), %xmm0`
- `0x401366`: `addsd (%rdx), %xmm0`

通过 PIN 计数模式测得：

- `target_ip = 0x401340`
- `instructions_before_target = 60238363`

本轮窗口：

- `skip = 60238000`
- `trace = 50000`

trace 文件：

- [Track☆Maker/traces/real/bicg_hot.champsim](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/bicg_hot.champsim>)
- [Track☆Maker/traces/real/bicg_hot.optrace](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/bicg_hot.optrace>)

### `pr -g 10 -n 1`

先测得：

- `main = 0x403af0`
- `instructions_before_main = 2185363`

再用符号表和反汇编定位 PageRank 核心：

- `PageRankPullGS = 0x405270`

本轮选取的热段入口：

- `target_ip = 0x4054a8`
- `instructions_before_target = 13023915`

本轮窗口：

- `skip = 13023000`
- `trace = 50000`

trace 文件：

- [Track☆Maker/traces/real/pr_g10_hot.champsim](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/pr_g10_hot.champsim>)
- [Track☆Maker/traces/real/pr_g10_hot.optrace](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/pr_g10_hot.optrace>)

### `mvt`

`objdump` 显示 `main` 中有两段规则热循环，本轮选取第一段：

- `0x401300`: `movsd (%rdx,%rax,1), %xmm0`
- `0x401305`: `mulsd (%r12,%rax,1), %xmm0`

通过 PIN 计数模式测得：

- `main = 0x401170`
- `instructions_before_main = 132982`
- `target_ip = 0x401300`
- `instructions_before_target = 56241665`

本轮窗口：

- `skip = 56241000`
- `trace = 50000`

trace 文件：

- [Track☆Maker/traces/real/mvt_hot.champsim](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/mvt_hot.champsim>)
- [Track☆Maker/traces/real/mvt_hot.optrace](</home/tututu/my_arch/ChampSim/Track☆Maker/traces/real/mvt_hot.optrace>)

## ChampSim 验证方式

两组 trace 都用下面的形式跑：

```bash
./bin/champsim_l2_stride_llc_no --hide-heartbeat -w 1000 -i 30000 \
  --op-traces <trace>.optrace <trace>.champsim
```

## 结果

### `atax`

- `Load-classified instructions: 8421`
- `load-derived: 3 (0.03563%)`
- `AFFINE: 8394 (99.68%)`
- `Top load PCs`
  - `0x401326`: dominant `AFFINE (99.69%)`
  - `0x401320`: dominant `AFFINE (99.69%)`
  - `0x401396`: dominant `AFFINE (100%)`
  - `0x401390`: dominant `AFFINE (100%)`

结论：

- 真实规则数值 kernel 上，Track☆Maker 仍然把主导 load 稳定识别为 affine。
- 这和 toy demo 里的 `load_seq_sum` / `load_stride_sum` 一致。

### `bfs -g 10 -n 1`

- `Load-classified instructions: 7822`
- `load-derived: 6640 (84.89%)`
- 分类分布：
  - `SIMPLE: 571 (7.3%)`
  - `AFFINE: 3286 (42.01%)`
  - `DEP_1: 3175 (40.59%)`
  - `DEP_DEEP: 784 (10.02%)`
  - `COMPLEX: 6`
- `Top load PCs`
  - `0x40539d`: dominant `AFFINE (99.33%)`
  - `0x4053a8`: dominant `DEP_1 (100%)`
  - `0x4053ac`: dominant `DEP_DEEP (100%)`
  - `0x4053b0`: dominant `AFFINE (99.39%)`
  - `0x4053c9`: dominant `DEP_1 (99.37%)`
  - `0x4053d3`: dominant `DEP_1 (100%)`

结论：

- BFS 小图窗口里已经能看到明显的混合模式：
  - 一部分 load 是规则索引/位图访问，偏 `AFFINE`
  - 一部分 load 带明显的先前 load 依赖，落在 `DEP_1` / `DEP_DEEP`
- 这说明 Track☆Maker 在真实图 workload 上也能看到比 toy demo 更复杂、但仍然可解释的分类结构。

### `bicg`

- `Load-classified instructions: 16258`
- `load-derived: 2700 (16.61%)`
- 分类分布：
  - `SIMPLE: 2770 (17.04%)`
  - `AFFINE: 10788 (66.36%)`
  - `DEP_1: 2`
  - `DEP_DEEP: 2698 (16.59%)`
- `Top load PCs`
  - `0x40135c`: dominant `AFFINE (99.52%)`
  - `0x401356`: dominant `AFFINE (99.52%)`
  - `0x40134a`: dominant `AFFINE (99.52%)`
  - `0x401344`: dominant `AFFINE (99.52%)`
  - `0x401340`: dominant `SIMPLE (100%)`
  - `0x401366`: dominant `DEP_DEEP (99.56%)`

结论：

- `bicg` 不是纯单一 affine 模式，而是同一个 kernel 里混有不同角色的 load：
  - 从规则向量或矩阵读取的部分偏 `AFFINE`
  - 对累加值或前序 load 更敏感的部分会落到 `DEP_DEEP`
- 这说明在真实 dense linear algebra kernel 里，Track☆Maker 也能按 load PC 拉开差异。

### `pr -g 10 -n 1`

- `Load-classified instructions: 11321`
- `load-derived: 10001 (88.34%)`
- 分类分布：
  - `SIMPLE: 54 (0.477%)`
  - `AFFINE: 4870 (43.02%)`
  - `DEP_1: 4761 (42.05%)`
  - `DEP_DEEP: 1636 (14.45%)`
- `Top load PCs`
  - `0x4054c7`: dominant `DEP_1 (80.43%)`
  - `0x4054c0`: dominant `AFFINE (77.75%)`
  - `0x4054a8`: dominant `AFFINE (100%)`
  - `0x405506`: dominant `DEP_DEEP (100%)`
  - `0x4054e2`: dominant `AFFINE (99.6%)`
  - `0x4054d7`: dominant `DEP_DEEP (100%)`

结论：

- PageRank 小图窗口里的模式和 BFS 类似，也是混合型：
  - 一部分 load 像规则图结构扫描，偏 `AFFINE`
  - 一部分 load 明显带有前序 load 依赖，落到 `DEP_1` / `DEP_DEEP`
- 和 BFS 相比，PR 的 `DEP_1` 比例更高，符合迭代式图算法中“读邻居贡献再归并”的特点。

### `mvt`

- `Load-classified instructions: 8506`
- `load-derived: 1 (0.01176%)`
- 分类分布：
  - `SIMPLE: 10 (0.1176%)`
  - `AFFINE: 8495 (99.87%)`
  - `DEP_1: 1`
  - `DEP_DEEP: 0`
- `Top load PCs`
  - `0x401305`: dominant `AFFINE (99.86%)`
  - `0x401300`: dominant `AFFINE (99.88%)`

结论：

- `mvt` 和 `atax` 一样，主导 load 几乎全都被识别为 `AFFINE`
- 这进一步说明 Track☆Maker 在规则 dense kernel 上的表现是稳定的，而不是只在单个 benchmark 上偶然成立。

## 当前判断

这轮冒烟验证说明：

- 不用上 SimPoint，也能先看到真实 benchmark 上的有效信号
- `PolyBench/atax` / `bicg` 和 `GAPBS/bfs` / `pr` 都适合作为下一阶段更系统实验的入口
- `PolyBench/atax` / `bicg` / `mvt` 和 `GAPBS/bfs` / `pr` 都适合作为下一阶段更系统实验的入口

但目前仍然只是最小验证，不应过度解读：

- `atax` 只截了单个内层热窗口
- `bfs` 使用的是很小的图输入
- 还没有按 phase 做更系统的抽样

下一步更合理的方向：

- GAPBS 再补更大一点的 `bfs` 或 `pr`
- 等分类规则稳定后，再决定是否引入 SimPoint
