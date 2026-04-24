# Track☆Maker

Track☆Maker 是 address-history 想法的实验工作区，目标是：

- 保持原始 ChampSim trace 格式不变
- 由 PIN tracer 额外输出 sidecar `optrace`
- 编写小程序覆盖不同访存模式
- 用这些 trace 验证 address-history 信号能否区分常见访存类别

目录结构：

- `demo/`
  - 用于生成 trace 的小型独立 C 程序，访存行为清晰
- `experiments/`
  - 持久化保存的实验笔记和验证记录
- `SETUP.md`
  - 快速环境配置自检
- `traces/`
  - 保留的少量 trace；临时扫窗和 skip 调参产物不要长期存放

建议工作流：

1. 先编译一个 `demo` 程序。
2. 用 `tracer/pin/obj-intel64/champsim_tracer.so` 运行，并同时指定 `-o` 和 `-op`。
3. 联合检查主 trace 与对应的 `*.optrace`。
4. 基于动态指令流定义并迭代 address-history 分类规则。

## 我们对 ChampSim 的修改

为了支持 Track☆Maker，目前在 ChampSim 主干基础上做了这些关键改动：

- Trace 侧（PIN tracer）：
  - 保持原始 `input_instr` trace 格式不变
  - 新增可选 sidecar `optrace` 输出（`-op`）
  - 支持辅助计数模式（`-main_count`、`-count_until_ip`）用于定位 hot loop 窗口

- Trace 读取侧（ChampSim）：
  - 新增 `--op-traces` 参数
  - 支持主 trace + optrace 双流合并读取
  - 对齐检查（`instr_num` / `ip`）失败时直接报错，避免静默错配

- 指令与历史元数据：
  - 在 `ooo_model_instr` 上挂接 optrace metadata
  - 引入轻量 history 编码（token history、depth、complexity、load-derived 等）
  - 在调度阶段做 history 传播与更新

- Load 分类与统计：
  - 加入 `SIMPLE / AFFINE / DEP_1 / DEP_DEEP / COMPLEX / UNKNOWN` 分类
  - 增加 TrackMaker summary 输出（总体分布、按 PC 的 top load 分类）
  - 针对 x86 的地址源过滤修正：
    - 分类时优先忽略“本条指令会写回”的源寄存器，避免数据链污染地址链

当前这套改动的定位是：

- 先验证“地址历史是否有分类信号”
- 暂不追求完整 uop 级精确建模
