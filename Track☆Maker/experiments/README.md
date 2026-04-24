# Track☆Maker 实验记录

这个目录用于存放 Track☆Maker 原型的实验笔记和验证结果。

当前文档：

- `Load_History_Validation.md`
  - load-history 分类验证结果
  - hot loop 入口前的动态指令数
  - x86 地址历史处理中的特殊问题和修正
- `Benchmark_Suites_Notes.md`
  - PolyBench / GAPBS 是否适合当前实验
  - 推荐编译配置
  - 推荐优先尝试的 benchmark
  - 两个测试集的程序清单和大致行为
- `Real_Benchmark_Smoke_Test.md`
  - `atax` / `bfs` 的最小真实 benchmark 验证
  - hot loop 入口、skip 和 trace 位置
  - 在真实 workload 上的第一轮分类结果
- `Intern_Project_Brief.md`
  - 项目说明（idea / 问题定义 / 发现 / 相关工作 / 当前问题）

这里尽量只放长期保留的实验记录。
临时 trace、扫窗输出和各种 skip 调参产物不要长期留在这个目录里。
