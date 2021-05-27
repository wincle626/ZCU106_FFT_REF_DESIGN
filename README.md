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

| FFT  |       16      |      64       |      256      |     1024      |      4096     |     16384     |
| ---- | ------------- | ------------- | ------------- | ------------- | ------------- | ------------- |
| LUT  |  8919 (3.87%) | 15306 (6.64%) | 16535 (7.18%) | 16535 (7.18%) | 16535 (7.18%) | 16535 (7.18%) |
| FF   | 12329 (2.68%) | 19382 (4.21%) | 16535 (7.18%) | 16535 (7.18%) | 16535 (7.18%) | 16535 (7.18%) |
| BRAM |     9 (2.88%) |    16 (5.13%) |   312 (100%)  |   312 (100%)  |   312 (100%)  |   312 (100%)  |
