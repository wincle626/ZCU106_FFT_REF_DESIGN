# Reference design for Xilinx FFT

## Hardware

In this design, the Xilinx FFT is used in pipelined stream mode. The FFT is configured to use 32 bits complex floating point data with various length from 16 to 16384. 

<img src="https://github.com/wincle626/ZCU106_FFT_REF_DESIGN/blob/main/figures/fft_blockdiagram.png" alt="fftblockdiagram"
	title="FFT block diagram" width="960" height="320" />

## Software

The complex floating point data is streamed through Xilinx DMA by mapping the On-Chip-Memory as the input and output buffer. It is also possible to map the DDR memory as the input and output buffer, however, so far there is a delay between input and output sequence that the output data is one frame behind the input data. 

The real sequence is used as the input data and the output data is checked by comparing with a native DFT results using the same input. The comparison is at bit level, so as to capture single bit variance. 

## Experiment

### 1. Resource utilization 

Here, we only pick the some typical cases. All the designs target 200 MHz. 

| FFT  |      64       |      256      |     1024      |      4096     |     16384     |
| ---- | ------------- | ------------- | ------------- | ------------- | ------------- |
| LUT  | 15306 (6.64%) | 15352 (6.66%) | 15453 (6.71%) | 15528 (6.74%) | 15764 (6.84%) |
| FF   | 19382 (4.21%) | 19470 (4.23%) | 19608 (4.26%) | 19775 (4.29%) | 19883 (4.31%) |
| BRAM |    16 (5.13%) |    16 (5.13%) |    16 (5.13%) |    20 (6.41%) |    48 (15.38%)|

### 2. Power consumption

The VCCINT is the power supply rail for programm logic system while the VCCBRAM is the power supply rail for the block ram. The nominal voltage of VCCINT and VCCBRAM are 0.85 volt and 0.9 volt. The official undervolt fault limited voltage of VCCINT and VCCBRAM are 0.7282 volt and 0.7253 volt, which means there might be hardware fault error under those voltages. 

1. 64 points FFT (200 MHz)

| VCCINT (volt) |       Power (mW)       | VCCBRAM (volt) |       Power (mW)       |
| ------------- | ---------------------- | -------------- | ---------------------- |
|      0.85     |         499.48         |      0.90      |          57.06         |
|      0.80     |         451.02         |      0.85      |          48.07         |
|      0.75     |         404.33         |      0.80      |          39.87         |
|      0.70     |         361.26         |      0.75      |          32.52         |
|      0.65     |         318.05         |      0.70      |          25.43         |
|      0.60     |         281.06         |      0.65      |          19.24         |
|      0.55     |         245.25         |      0.60      |          13.45         |
|      0.52     |         224.94         |      0.59      |          12.32         |
|      0.51     |          N/A           |      0.58      |           N/A          |


2. 16384 points FFT (200 MHz)

| VCCINT (volt) |       Power (mW)       | VCCBRAM (volt) |       Power (mW)       |
| ------------- | ---------------------- | -------------- | ---------------------- |
|      0.85     |         591.96         |      0.90      |          58.22         |
|      0.80     |         531.32         |      0.85      |          49.28         |
|      0.75     |         473.51         |      0.80      |          40.75         |
|      0.70     |         420.57         |      0.75      |          33.28         |
|      0.65     |         367.65         |      0.70      |          26.16         |
|      0.60     |         323.02         |      0.65      |          19.91         |
|      0.55     |         280.33         |      0.60      |          14.01         |
|      0.52     |         256.17         |      0.59      |          12.96         |
|      0.51     |          N/A           |      0.58      |           N/A          |
