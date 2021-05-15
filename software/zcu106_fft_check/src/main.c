/*
 * xaxidma_register_wr.c
 *
 *  Created on: 2021Äê3ÔÂ7ÈÕ
 *      Author: vv_fa
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <math.h>
#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "xaxidma.h"
#include "xparameters.h"
#include "xdebug.h"

#define CHECK 0

#define TX_BUFFER_BASE		(0xFFFC0000)
#define RX_BUFFER_BASE		(0xFFFD0000)
//#define TX_BUFFER_BASE		(0x01100000)
//#define RX_BUFFER_BASE		(0x01300000)

//#define MAX_PKT_LEN		8192
#define BYTES sizeof(float)*2

typedef struct{
	int real;
	int imag;
}Complex;

int XAxiDma_Check(int MAX_PKT_LEN, XAxiDma * InstancePtr);
float *random_reals(int n);
void naive_dft(const float *inreal, const float *inimag,
		float *outreal, float *outimag, int n, bool inverse);
int datastream(int MAX_PKT_LEN, int Iter, int Check);
int complexstream(Complex values);

int main(int argc, char** argv){

	if(argc != 4){
		printf("Please input a) FFT size, b) Loop size, c) Check flag \n");
		exit(0);
	}
	int MAX_PKT_LEN = atoi(argv[1]);
	int Iter = atoi(argv[2]);
	int Check = atoi(argv[3]);

	datastream(MAX_PKT_LEN, Iter, Check);

	return XST_SUCCESS;
}

float *random_reals(int n) {
	float *result = malloc(n * sizeof(float));
	for (int i = 0; i < n; i++)
		result[i] = (rand() / (RAND_MAX + 1.0)) * 2 - 1;
	return result;
}

void naive_dft(const float *inreal, const float *inimag,
		float *outreal, float *outimag, int n, bool inverse) {

	float coef = (inverse ? 2 : -2) * M_PI;
	for (int k = 0; k < n; k++) {  // For each output element
		float sumreal = 0;
		float sumimag = 0;
		for (int t = 0; t < n; t++) {  // For each input element
			float angle = coef * ((long long)t * k % n) / n;
			sumreal += inreal[t] * cos(angle) - inimag[t] * sin(angle);
			sumimag += inreal[t] * sin(angle) + inimag[t] * cos(angle);
		}
		outreal[k] = sumreal;
		outimag[k] = sumimag;
	}
}

int datastream(int MAX_PKT_LEN, int Iter, int Check){

	int Status;
	int Index;
	int loop = Iter;

	float *TxBufferPtr;
	float *RxBufferPtr;
	void *TxBufferPtr_vaddr;
	void *RxBufferPtr_vaddr;
	void *TxBufferPtr_paddr;
	void *RxBufferPtr_paddr;

	clock_t start, end;
    double cpu_time_used;

	/* Initialize the addresses
	 */
	XAxiDma_Config *CfgPtr;
	XAxiDma AxiDma;
	int fd = open("/dev/mem", O_RDWR);
	TxBufferPtr_paddr = (float *) TX_BUFFER_BASE;
	RxBufferPtr_paddr = (float *) RX_BUFFER_BASE;
	TxBufferPtr_vaddr   = mmap(NULL, MAX_PKT_LEN*BYTES, PROT_READ|PROT_WRITE, MAP_SHARED, fd,
			TX_BUFFER_BASE);
	RxBufferPtr_vaddr = mmap(NULL, MAX_PKT_LEN*BYTES, PROT_READ|PROT_WRITE, MAP_SHARED, fd,
			RX_BUFFER_BASE);
	TxBufferPtr = (float *) TxBufferPtr_vaddr;
	RxBufferPtr = (float *) RxBufferPtr_vaddr;
//	printf("TxBufferPtr_paddr-0x%lX, RxBufferPtr_paddr-0x%lX\n "
//			"TxBufferPtr_vaddr-0x%lX, RxBufferPtr_vaddr-0x%lX\n"
//			"TxBufferPtr-0x%lX, RxBufferPtr-0x%lX\n",
//			(float)TxBufferPtr_paddr, (float)RxBufferPtr_paddr,
//			(float)TxBufferPtr_vaddr, (float)RxBufferPtr_vaddr,
//			(float)TxBufferPtr, (float)RxBufferPtr);

	/* Initialize the XAxiDma device.
	 */
	CfgPtr = XAxiDma_LookupConfig(0);
	if (!CfgPtr) {
		printf("No DMA config found for %d\n\n", 0);
		return XST_FAILURE;
	}else{
		printf("DMA config found for %d\n\n", 0);
	}
	printf("Physical DMA BaseAddr 0x%lX\n\n", CfgPtr->BaseAddr);
	CfgPtr->BaseAddr   = (UINTPTR)mmap(NULL, MAX_PKT_LEN*BYTES,
						PROT_READ|PROT_WRITE, MAP_SHARED, fd,
						CfgPtr->BaseAddr);
	printf("Virtual DMA BaseAddr 0x%lX\n\n", CfgPtr->BaseAddr);
	Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		printf("Initialization failed %d\n\n", Status);
		return XST_FAILURE;
	}else{
		printf("Initialization succeeded %d\n\n", Status);
	}
	printf("Tx Datawidth-%d, Rx Datawidth-%d\n\n",
			AxiDma.TxBdRing.DataWidth,
			AxiDma.RxBdRing[0].DataWidth);
	XAxiDma_WriteReg(AxiDma.TxBdRing.ChanBase,
			 XAXIDMA_SRCADDR_OFFSET,
			 LOWER_32_BITS((UINTPTR)TX_BUFFER_BASE));
//	printf("XAxiDma_WriteReg(AxiDma.TxBdRing.ChanBase,"
//			"XAXIDMA_SRCADDR_OFFSET, "
//			"LOWER_32_BITS((UINTPTR)TxBufferPtr_paddr)) 0x%X\n\n",
//			LOWER_32_BITS((UINTPTR)TX_BUFFER_BASE));
	if (AxiDma.AddrWidth > 32){
//		printf("XAxiDma_WriteReg(AxiDma.TxBdRing.ChanBase,"
//				"XAXIDMA_SRCADDR_MSB_OFFSET,"
//				"UPPER_32_BITS((UINTPTR)TxBufferPtr_paddr) 0x%X\n\n",
//				UPPER_32_BITS((UINTPTR)TxBufferPtr_paddr));
		XAxiDma_WriteReg(AxiDma.TxBdRing.ChanBase,
				 XAXIDMA_SRCADDR_MSB_OFFSET,
				 UPPER_32_BITS((UINTPTR)TX_BUFFER_BASE));
	}
	XAxiDma_WriteReg(AxiDma.TxBdRing.ChanBase,
				XAXIDMA_BUFFLEN_OFFSET, MAX_PKT_LEN*BYTES);
//	printf("XAxiDma_WriteReg(AxiDma.TxBdRing.ChanBase, "
//			"XAXIDMA_BUFFLEN_OFFSET, MAX_PKT_LEN) 0x%lX\n\n",
//			MAX_PKT_LEN*BYTES);
	XAxiDma_WriteReg(AxiDma.RxBdRing[0].ChanBase,
			 XAXIDMA_DESTADDR_OFFSET,
			 LOWER_32_BITS((UINTPTR)RX_BUFFER_BASE));
//	printf("XAxiDma_WriteReg(AxiDma.RxBdRing[0].ChanBase,"
//			"XAXIDMA_DESTADDR_OFFSET, "
//			"LOWER_32_BITS((UINTPTR)RxBufferPtr_paddr) 0x%X\n\n",
//			LOWER_32_BITS((UINTPTR)RX_BUFFER_BASE));
	if (AxiDma.AddrWidth > 32){
		XAxiDma_WriteReg(AxiDma.RxBdRing[0].ChanBase,
				 XAXIDMA_DESTADDR_MSB_OFFSET,
				 UPPER_32_BITS((UINTPTR)RX_BUFFER_BASE));
//		printf("XAxiDma_WriteReg(AxiDma.RxBdRing[0].ChanBase, "
//				"XAXIDMA_DESTADDR_MSB_OFFSET, "
//				"UPPER_32_BITS((UINTPTR)RxBufferPtr_paddr)) 0x%X\n\n",
//				UPPER_32_BITS((UINTPTR)RxBufferPtr_paddr));
	}
	XAxiDma_WriteReg(AxiDma.RxBdRing[0].ChanBase,
				XAXIDMA_BUFFLEN_OFFSET, MAX_PKT_LEN*BYTES);
//	printf("XAxiDma_WriteReg(AxiDma.RxBdRing[0].ChanBase, "
//			"XAXIDMA_BUFFLEN_OFFSET, MAX_PKT_LEN) 0x%lX\n\n",
//			MAX_PKT_LEN*BYTES);

	if(!(XAxiDma_Check(MAX_PKT_LEN, &AxiDma)==XST_SUCCESS)){
		printf("Check DMA found issue\n\n");
		return XST_FAILURE;
	}else{
		printf("Check DMA found no issue\n\n");
	}
	/* DFT
	 * */
//	float *inputreal = random_reals(MAX_PKT_LEN);
//	float *inputimag = random_reals(MAX_PKT_LEN);
	float *inputreal = malloc(MAX_PKT_LEN * sizeof(float));
	float *inputimag = malloc(MAX_PKT_LEN * sizeof(float));
	for(int i=0;i<MAX_PKT_LEN;i++){
		inputreal[i] = (float)i/10;
		inputimag[i] = 0.0;
	}
	float *expectreal = malloc(MAX_PKT_LEN * sizeof(float));
	float *expectimag = malloc(MAX_PKT_LEN * sizeof(float));
	char filename[256]="";
	char fftsize[256]="";
	strcat(filename, "fft");
	sprintf(fftsize, "%d", MAX_PKT_LEN);
	strcat(filename, fftsize);
	strcat(filename, ".txt");
	if(Check==1){
		FILE *fid;
		fid = fopen(filename,"r");
		char line[256];
		for(Index=0;Index<MAX_PKT_LEN;Index++){
			fgets(line, sizeof(line), fid);
			expectreal[Index] = (float) atof(line);
		}
		fclose(fid);
	}else{
		naive_dft(inputreal, inputimag, expectreal, expectimag, MAX_PKT_LEN, false);
	}
	/* Initialize the input data.
	 */
	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
		TxBufferPtr[2*Index] = inputreal[Index];
		TxBufferPtr[2*Index+1] = inputimag[Index];
	}
//	printf("Before data streaming: \n");
//	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
//		printf("TxBufferPtr[%d] = (float) %f\n", 2*Index, TxBufferPtr[2*Index]);
//		printf("TxBufferPtr[%d] = (float) %f\n", 2*Index+1, TxBufferPtr[2*Index+1]);
//	}
//	printf("\n");
//	printf("\n");



    start = clock();

	/* Do the DMA simple transfer.
	 */
    while(loop--){
		Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) RX_BUFFER_BASE,
					MAX_PKT_LEN*BYTES, XAXIDMA_DEVICE_TO_DMA);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
		Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) TX_BUFFER_BASE,
					MAX_PKT_LEN*BYTES, XAXIDMA_DMA_TO_DEVICE);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
		while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) ||
			(XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE))) {
				/* Wait */
		}
    }
	//usleep(1000*1000);

	end = clock();
    cpu_time_used = ((double) 1000 * (end - start)) / CLOCKS_PER_SEC / Iter;
    printf("%d point FFT consumes %lf ms \n", MAX_PKT_LEN, cpu_time_used);

//	printf("After data streaming: \n");
//	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
//		printf("RxBufferPtr[%d] = (float) %f\n", 2*Index, (float)RxBufferPtr[2*Index]);
//		printf("RxBufferPtr[%d] = (float) %f\n", 2*Index+1, (float)RxBufferPtr[2*Index+1]);
//	}
//	printf("\n");
//	printf("\n");

//	usleep(1000*BYTES000);

	/* Check the output data.
	 */
//	printf("After data streaming: \n");
//	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
//		printf("TxBufferPtr[%d] = (float) %f\n", Index, (float) TxBufferPtr[Index]);
//	}
//	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
//		printf("RxBufferPtr[%d] = (float) %f\n", Index, (float) RxBufferPtr[Index]);
//	}

	printf("Errors check...\n");
	int count = 0;
	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
		if((RxBufferPtr[2*Index] - expectreal[Index])
			*(RxBufferPtr[2*Index] - expectreal[Index])
			> 0.0001*MAX_PKT_LEN){
//			printf("expectreal[%d] = (float) %f\n", Index, expectreal[Index]);
//			printf("RxBufferPtr[%d] = (float) %f\n", 2*Index, (float) RxBufferPtr[2*Index]);
			count ++;
		}
//		if((RxBufferPtr[2*Index+1] - expectimag[Index])
//			*(RxBufferPtr[2*Index+1] - expectimag[Index])
//			> 0.0001*MAX_PKT_LEN){
////			printf("expectimag[%d] = (float) %f\n", Index, expectimag[Index]);
////			printf("RxBufferPtr[%d] = (float) %f\n", 2*Index+1, (float) RxBufferPtr[2*Index+1]);
//			count ++;
//		}
	}
//	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
//		printf("TxBufferPtr[%d] = (float) 0x%lx\n", Index, (float) TxBufferPtr[Index]);
//	}
//	for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
//		printf("RxBufferPtr[%d] = (float) 0x%lx\n", Index, (float) RxBufferPtr[Index]);
//	}
	if(count > 0 ){
		printf("%d errors found. \n", count);
	}else{
		printf("No errors found. \n");
	}
//	printf("TxBufferPtr[%d] = (float) 0x%lx\n", Index-1, (float) TxBufferPtr[Index-1]);
//	printf("RxBufferPtr[%d] = (float) 0x%lx\n", Index-1, (float) RxBufferPtr[Index-1]);
	printf("\n");

	if(Check!=1){
		FILE *fid;
		fid = fopen(filename,"a+");
		for(Index=0;Index<MAX_PKT_LEN;Index++){
			fprintf(fid,"%f\n",RxBufferPtr[2*Index]);
		}
		fclose(fid);
	}

	free(inputreal);
	free(inputimag);
	free(expectreal);
	free(expectimag);

	return Status;

}

//int complexstream(Complex values){
//
//	int Status;
//
//	return Status;
//
//}

int XAxiDma_Check(int MAX_PKT_LEN, XAxiDma * InstancePtr)
{

//	int Status = 0;

	if (!InstancePtr->Initialized) {

		printf("Pause: Driver not initialized"
					" %d\n\n",InstancePtr->Initialized);

		return XST_NOT_SGDMA;
	}

	if(XAxiDma_HasSg(InstancePtr)){
		printf("Device configured as SG mode \n\n");
		return XST_FAILURE;
	}else{
		printf("Device configured as no SG mode \n\n");
	}

	if (InstancePtr->HasMm2S) {
		XAxiDma_BdRing *TxRingPtr;
		TxRingPtr = XAxiDma_GetTxRing(InstancePtr);

//		printf("Now check TxRingPtr\n\n");
		XAxiDma_BdRingDumpRegs(TxRingPtr);

		if(!(XAxiDma_ReadReg(InstancePtr->TxBdRing.ChanBase,
				XAXIDMA_SRCADDR_OFFSET)==TX_BUFFER_BASE)){
			printf("XAxiDma_ReadReg(AxiDma.TxBdRing.ChanBase,"
					"XAXIDMA_SRCADDR_OFFSET)  0x%X\n\n",
					XAxiDma_ReadReg(InstancePtr->TxBdRing.ChanBase,
							XAXIDMA_SRCADDR_OFFSET));
//			printf("XAxiDma_ReadReg(AxiDma.TxBdRing.ChanBase,"
//					"XAXIDMA_SRCADDR_MSB_OFFSET) 0x%X\n\n",
//					XAxiDma_ReadReg(InstancePtr->TxBdRing.ChanBase,
//							XAXIDMA_SRCADDR_MSB_OFFSET));
			return XST_FAILURE;
		}

		if(!(XAxiDma_ReadReg(InstancePtr->TxBdRing.ChanBase,
				XAXIDMA_BUFFLEN_OFFSET)==MAX_PKT_LEN*BYTES)){
//			printf("XAxiDma_ReadReg(AxiDma.TxBdRing.ChanBase, "
//					"XAXIDMA_BUFFLEN_OFFSET) 0x%X\n\n",
//					XAxiDma_ReadReg(InstancePtr->TxBdRing.ChanBase,
//							XAXIDMA_BUFFLEN_OFFSET));
			return XST_FAILURE;
		}
	}

	if (InstancePtr->HasS2Mm) {
		int RingIndex = 0;
		for (RingIndex = 0; RingIndex < InstancePtr->RxNumChannels;
				RingIndex++) {
			XAxiDma_BdRing *RxRingPtr;
			RxRingPtr = XAxiDma_GetRxIndexRing(InstancePtr, RingIndex);

//			printf("Now check RxRingPtr\n\n");
			XAxiDma_BdRingDumpRegs(RxRingPtr);

			if(!(XAxiDma_ReadReg(InstancePtr->RxBdRing[RingIndex].ChanBase,
					 XAXIDMA_DESTADDR_OFFSET)==RX_BUFFER_BASE)){
				printf("XAxiDma_ReadReg(AxiDma.RxBdRing[RingIndex].ChanBase,"
						"XAXIDMA_DESTADDR_OFFSET)  0x%X\n\n",
						XAxiDma_ReadReg(InstancePtr->RxBdRing[RingIndex].ChanBase,
								XAXIDMA_DESTADDR_OFFSET));
//				printf("XAxiDma_ReadReg(AxiDma.RxBdRing[RingIndex].ChanBase,"
//						"XAXIDMA_DESTADDR_MSB_OFFSET) 0x%X\n\n",
//						XAxiDma_ReadReg(InstancePtr->RxBdRing[RingIndex].ChanBase,
//								XAXIDMA_DESTADDR_MSB_OFFSET));
//				printf("XAxiDma_ReadReg(AxiDma.RxBdRing[0].ChanBase, "
//						"XAXIDMA_BUFFLEN_OFFSET) 0x%X\n\n",
//						XAxiDma_ReadReg(InstancePtr->RxBdRing[0].ChanBase,
//								XAXIDMA_BUFFLEN_OFFSET));
				return XST_FAILURE;
			}

			if(!(XAxiDma_ReadReg(InstancePtr->RxBdRing[0].ChanBase,
								XAXIDMA_BUFFLEN_OFFSET)==MAX_PKT_LEN*BYTES)){
//				printf("XAxiDma_ReadReg(AxiDma.RxBdRing[0].ChanBase, "
//						"XAXIDMA_BUFFLEN_OFFSET) 0x%X\n\n",
//						XAxiDma_ReadReg(InstancePtr->RxBdRing[0].ChanBase,
//								XAXIDMA_BUFFLEN_OFFSET));
			}
		}
	}

	return XST_SUCCESS;

}
