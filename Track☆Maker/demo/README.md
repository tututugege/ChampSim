# Track☆Maker Demos

These small C programs are intended to generate distinct memory access patterns that are easy to reason about.

- `seq_stride.c`: regular linear and fixed-stride loads
- `pointer_chase.c`: dependent linked-list style pointer chasing
- `gather.c`: indirect indexed loads through an index array
- `stencil2d.c`: regular 2D stencil with multiple nearby accesses
- `load_seq_sum.c`: load-only sequential sum
- `load_stride_sum.c`: load-only fixed-stride sum
- `load_gather_sum.c`: load-only indirect gather sum
- `load_chase_sum.c`: load-only pointer chasing

Build examples:

```bash
cc -O2 -std=c11 -o seq_stride seq_stride.c
cc -O2 -std=c11 -o pointer_chase pointer_chase.c
cc -O2 -std=c11 -o gather gather.c
cc -O2 -std=c11 -o stencil2d stencil2d.c
cc -O2 -std=c11 -o load_seq_sum load_seq_sum.c
cc -O2 -std=c11 -o load_stride_sum load_stride_sum.c
cc -O2 -std=c11 -o load_gather_sum load_gather_sum.c
cc -O2 -std=c11 -o load_chase_sum load_chase_sum.c
```

Run under PIN with sidecar output:

```bash
$PIN_ROOT/pin -t ../../tracer/pin/obj-intel64/champsim_tracer.so \
  -o /tmp/seq_stride.champsim \
  -op /tmp/seq_stride.optrace \
  -- ./seq_stride
```
