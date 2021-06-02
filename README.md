# Mandelbrot optimization
This a practical task on optimization of heavy mathematical computations, based on the Mandelbrot set coloring.

The computation code was completely vectorized, where possible, using various Intel intrinsic SIMD and AVX instructions. 

The resulting performance increase was eight-fold: from 5 FPS to 40 FPS.
