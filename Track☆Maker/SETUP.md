# Track☆Maker Setup（快速上手）

这份文档的目标是：  
让新同学在一台干净的 Linux x86-64 机器上，快速确认 Track☆Maker 环境可用，并能跑通一条最小端到端流程。

## 1. 环境前提

必须满足：

- Linux x86-64
- `gcc/g++`、`make`
- Intel PIN（建议 3.22，已在本项目验证）
- 可以正常编译并运行当前仓库里的 ChampSim 修改版

建议先检查：

```bash
gcc --version
g++ --version
make --version
echo "$PIN_ROOT"
```

如果 `PIN_ROOT` 为空，需要先安装并配置 PIN。

## 2. 配置 PIN

下载并解压 PIN（示例）：

```bash
wget https://software.intel.com/sites/landingpage/pintool/downloads/pin-3.22-98547-g7a303a835-gcc-linux.tar.gz
tar zxf pin-3.22-98547-g7a303a835-gcc-linux.tar.gz
```

设置环境变量（按你的实际路径）：

```bash
export PIN_ROOT=/path/to/pin-3.22-98547-g7a303a835-gcc-linux
```

建议写入 `~/.bashrc` 或 `~/.zshrc`。

## 3. 编译必要组件

在仓库根目录：

```bash
cd /path/to/ChampSim
make -j
```

编译 PIN tracer：

```bash
cd tracer/pin
make
cd ../..
```

成功后应当有：

- `bin/champsim_*`
- `tracer/pin/obj-intel64/champsim_tracer.so`

## 4. 三条最小自检命令

### 4.1 Tracer 基本可用

```bash
$PIN_ROOT/pin -t tracer/pin/obj-intel64/champsim_tracer.so \
  -main_count 1 -- /bin/ls
```

预期：输出 `main=...` 和 `instructions_before_main=...`。

### 4.2 生成一对 trace + optrace

先编译一个最小 demo（这里用 `load_seq_sum`）：

```bash
cc -O2 -std=c11 -Wall -Wextra -pedantic -fno-pie -no-pie \
  Track☆Maker/demo/load_seq_sum.c -o /tmp/trackmaker_load_seq_sum
```

再生成 trace：

```bash
$PIN_ROOT/pin -t tracer/pin/obj-intel64/champsim_tracer.so \
  -o /tmp/load_seq_sum.champsim \
  -op /tmp/load_seq_sum.optrace \
  -s 5500000 -t 50000 \
  -- /tmp/trackmaker_load_seq_sum
```

预期：出现两个文件：

- `/tmp/load_seq_sum.champsim`
- `/tmp/load_seq_sum.optrace`

### 4.3 ChampSim 能读取 `--op-traces`

```bash
./bin/champsim_l2_stride_llc_no --hide-heartbeat -w 1000 -i 30000 \
  --op-traces /tmp/load_seq_sum.optrace \
  /tmp/load_seq_sum.champsim
```

预期：输出里出现 `TrackMaker Load History Summary`。

## 5. 常见问题

- `PIN_ROOT` 未设置或路径错误  
  现象：`pin: command not found` 或 tracer 无法加载

- tracer 编译失败  
  先确认 `tracer/pin/Makefile` 能找到 `PIN_ROOT`

- `--op-traces` 报错或对齐失败  
  必须保证 `.champsim` 和 `.optrace` 来自同一次 tracer 运行，不能混用

- GAPBS 跑得非常慢  
  先用小输入（如 `-g 10 -n 1`）并确保 `SERIAL=1`

## 6. 推荐起步路径

1. 先跑 toy demo（`Track☆Maker/demo`）
2. 再跑 `Track☆Maker/experiments/Real_Benchmark_Smoke_Test.md` 里的小窗口
3. 最后再尝试更大输入或更多 benchmark
