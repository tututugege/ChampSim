# Track☆Maker 基准套件使用说明

## 目的

给 Track☆Maker 后续实验固定一套可复用的基准套件使用方式，重点解决两个问题：

- 编译配置是否会引入不需要的并行行为
- 哪些 benchmark 更适合当前的单核 load-history 分类实验

## 总结

### PolyBench/C 3.2

结论：

- 适合当前实验
- 默认可以按串行方式使用
- 只要**不要加 `-fopenmp`**，就不会启用 OpenMP 并行执行

原因：

- 在 `utilities/polybench.c` 中，OpenMP 相关代码都被 `#ifdef _OPENMP` 包裹
- 不加 `-fopenmp` 时，`_OPENMP` 不会定义
- README 给出的样例编译命令也都是普通串行 `gcc`

### GAPBS

结论：

- 适合当前实验中的不规则访存部分
- 但默认 Makefile 会启用 OpenMP
- 必须显式使用 **`SERIAL=1`**

原因：

- `Makefile` 里默认会把 `-fopenmp` 加进 `CXX_FLAGS`
- 只有在 `SERIAL=1` 时才会关闭 OpenMP

## 推荐编译原则

为了和当前 Track☆Maker 小程序的使用方式保持一致，建议统一采用：

- 单线程
- 不开 OpenMP
- `-O3`
- `-fno-pie -no-pie`
- 不开 PAPI
- 不做额外 profiling / instrumentation

## PolyBench 推荐用法

目录：

- `Track☆Maker/benchmark/polybench-c-3.2`

推荐 benchmark：

- `linear-algebra/kernels/atax`
- `linear-algebra/kernels/bicg`
- `linear-algebra/kernels/mvt`
- `stencils/jacobi-2d-imper`

这些程序适合作为：

- affine / regular load 模式基线
- 多数组访存
- stencil 邻域访问

示例编译命令：

```bash
cd Track☆Maker/benchmark/polybench-c-3.2

gcc -O3 -fno-pie -no-pie \
  -I utilities \
  -I linear-algebra/kernels/atax \
  utilities/polybench.c \
  linear-algebra/kernels/atax/atax.c \
  -o atax

gcc -O3 -fno-pie -no-pie \
  -I utilities \
  -I linear-algebra/kernels/bicg \
  utilities/polybench.c \
  linear-algebra/kernels/bicg/bicg.c \
  -o bicg

gcc -O3 -fno-pie -no-pie \
  -I utilities \
  -I linear-algebra/kernels/mvt \
  utilities/polybench.c \
  linear-algebra/kernels/mvt/mvt.c \
  -o mvt

gcc -O3 -fno-pie -no-pie \
  -I utilities \
  -I stencils/jacobi-2d-imper \
  utilities/polybench.c \
  stencils/jacobi-2d-imper/jacobi-2d-imper.c \
  -o jacobi-2d-imper
```

注意：

- 不要额外加 `-fopenmp`
- 不要先启用 `POLYBENCH_PAPI`
- 初期尽量使用默认数据集，先看 trace 长度和分类结果

## GAPBS 推荐用法

目录：

- `Track☆Maker/benchmark/gapbs`

推荐 benchmark：

- `bfs`
- `pr`
- `sssp`

这些程序适合作为：

- 间接访存
- 图遍历
- load-dependent 地址链

推荐编译命令：

```bash
cd Track☆Maker/benchmark/gapbs
make clean
make SERIAL=1 CXX='g++ -fno-pie -no-pie'
```

如果想保守一点，也可以保留默认编译器设置，只关掉 OpenMP：

```bash
cd Track☆Maker/benchmark/gapbs
make clean
make SERIAL=1
```

建议优先从这类输入开始：

```bash
./bfs -g 20 -n 1
./pr -g 20 -n 1
./sssp -g 20 -n 1 -d 2
```

这里先使用生成图而不是超大真实图，原因是：

- 更快拿到 trace
- 更容易控制 trace 长度
- 更适合先验证分类质量

## 当前建议的实验顺序

### 第一步：规则访存

先跑 PolyBench：

- `atax`
- `bicg`
- `mvt`
- `jacobi-2d-imper`

目标：

- 验证 affine / regular load 是否稳定归类

### 第二步：不规则访存

再跑 GAPBS：

- `bfs`
- `pr`
- `sssp`

目标：

- 看是否会出现明显更多的 dependent / deep-dependent load

## 目前不建议

- 不建议直接跑 GAPBS 默认 OpenMP 版本
- 不建议一上来用 GAPBS 的超大真实图
- 不建议在 PolyBench 上同时开 PAPI、OpenMP、额外 profiling

先把单线程、纯净配置下的 load-history 分类信号看清楚，再决定是否扩大规模。

## 程序清单与大致行为

下面这部分不追求算法教材式定义，只强调对 Track☆Maker 更有用的直观行为特征：

- 规则 affine 访存
- 多数组 / 多流混合
- stencil 邻域访问
- 图遍历 / 间接访存 / load-dependent

### PolyBench/C 3.2

PolyBench 整体特点：

- 绝大多数程序都是规则循环和规则数组访问
- 很适合当 `AFFINE` / regular-load 的基线
- 少数程序虽然仍规则，但依赖和数据流更复杂，会出现混合模式

#### Datamining

- `correlation`
  - 相关系数矩阵
  - 多次按列扫描矩阵，规则 dense array 访问
- `covariance`
  - 协方差矩阵
  - 与 `correlation` 类似，也是规则矩阵扫描

#### Linear Algebra / Kernels

- `2mm`
  - 两次矩阵乘
  - 典型 dense GEMM 风格，规则块状/行列扫描
- `3mm`
  - 三次矩阵乘
  - 仍然是规则 dense 访存，但数据流更长
- `atax`
  - `A^T * (A * x)`
  - 规则矩阵-向量乘链式组合，适合看 affine load
- `bicg`
  - BiCG kernel
  - 整体规则，但同一热段里常有不同角色的 load，混合度比 `atax/mvt` 高
- `cholesky`
  - Cholesky 分解
  - 规则矩阵访问，但有三角边界
- `doitgen`
  - 多维数组上的小矩阵计算
  - 规则嵌套循环，访存比较规整
- `gemm`
  - 经典矩阵乘
  - 最标准的 dense affine 访存
- `gemver`
  - 多个矩阵向量更新组合
  - 规则，但多流更杂
- `gesummv`
  - 两个矩阵向量乘再求和
  - 规则访存
- `mvt`
  - 矩阵-向量乘加
  - 很典型的规则 affine kernel
- `symm`
  - 对称矩阵乘
  - 规则，但只访问矩阵的一部分
- `syr2k`
  - 对称 rank-2k 更新
  - 规则矩阵访问
- `syrk`
  - 对称 rank-k 更新
  - 规则矩阵访问
- `trisolv`
  - 三角求解
  - 规则，但 loop-carried dependency 更强
- `trmm`
  - 三角矩阵乘
  - 规则，但三角边界明显

#### Linear Algebra / Solvers

- `durbin`
  - Toeplitz 相关求解
  - 规则数组访问，但递推依赖较强
- `dynprog`
  - 动态规划
  - 规则表格更新，但依赖链更重，不像单纯 stride 那么干净
- `gramschmidt`
  - 正交化
  - 规则 dense 访存，带多轮扫描
- `lu`
  - LU 分解
  - 规则矩阵更新，依赖较重
- `ludcmp`
  - LU decomposition + solve
  - 和 `lu` 类似，但流程更完整

#### Medley

- `floyd-warshall`
  - 全源最短路动态规划
  - 三重循环，规则表格访问
- `reg_detect`
  - 区域检测
  - 比纯线代更杂，但整体仍偏规则多维数组访问

#### Stencils

- `adi`
  - ADI PDE kernel
  - 多轮规则扫描，偏 stencil
- `fdtd-2d`
  - 2D FDTD
  - 规则 stencil 更新
- `fdtd-apml`
  - FDTD 变体
  - 也是规则 stencil/PDE 访存
- `jacobi-1d-imper`
  - 1D Jacobi
  - 非常典型的规则 stencil
- `jacobi-2d-imper`
  - 2D Jacobi
  - 很适合作为规则邻域访问基线
- `seidel-2d`
  - 2D Seidel
  - 规则 stencil，但时间相关性更强

#### 对 Track☆Maker 的直观分组

- 最干净的规则 affine：
  - `gemm`
  - `atax`
  - `mvt`
  - `gesummv`
- 规则但更可能出现混合模式：
  - `bicg`
  - `gemver`
  - `trisolv`
  - `dynprog`
- 规则 stencil：
  - `jacobi-2d-imper`
  - `seidel-2d`
  - `adi`
  - `fdtd-2d`

### GAPBS

GAPBS 整体特点：

- 核心是图算法
- 常见行为包括邻接表遍历、间接访问、位图/队列访问、load-dependent 地址链
- 更适合验证 `DEP_1` / `DEP_DEEP` / mixed 模式

- `bfs`
  - 宽度优先搜索
  - 方向优化 BFS，典型图遍历，不规则 load 很多
- `sssp`
  - 单源最短路
  - delta-stepping，图边遍历 + 松弛，依赖和不规则性都很强
- `pr`
  - PageRank
  - 迭代式 pull 计算，通常是 `AFFINE + DEP_1 + DEP_DEEP` 的混合
- `pr_spmv`
  - PageRank 的 SpMV 风格实现
  - 更偏稀疏矩阵乘视角
- `cc`
  - 连通分量
  - 图遍历 + union/link 风格，不规则
- `cc_sv`
  - 连通分量的 Shiloach-Vishkin 风格实现
  - 更偏并行图连通算法
- `bc`
  - Betweenness Centrality
  - 多轮 BFS 和依赖累积，访存和依赖都比较重
- `tc`
  - Triangle Counting
  - 邻居列表交集/计数，局部性和不规则性混合
- `converter`
  - 图格式转换工具
  - 不是 benchmark kernel

#### 对 Track☆Maker 的直观分组

- 遍历型图算法：
  - `bfs`
  - `sssp`
  - `cc`
  - `cc_sv`
- 迭代聚合型：
  - `pr`
  - `pr_spmv`
- 更复杂分析型：
  - `bc`
  - `tc`

## 当前最值得优先尝试的程序

如果只从“对当前 load-history 分类最有帮助”来选，优先级建议如下。

### PolyBench

- `atax`
  - 规则 affine 基线
- `mvt`
  - 规则 affine 基线
- `bicg`
  - 规则 kernel 中带一定混合模式
- `jacobi-2d-imper`
  - 规则 stencil 基线

### GAPBS

- `bfs`
  - 典型图遍历，不规则性强
- `pr`
  - 典型混合型图迭代
- `sssp`
  - 依赖和不规则性都很强
