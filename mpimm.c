#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

#define MATRIX_SIZE 3000
#define SEND 0
#define RECV 1

double CLOCK_RATE = 700000000.0;
int MY_RANK;
int MATRIZ_SIZE = MATRIX_SIZE;

int index_translate( int r, int c ) {
	return (r * MATRIZ_SIZE) + c;
}

double matrix_mult( double *A, double *B, int cRow, int cCol ) {
	int idx;
	double sum = 0;
	for (idx = 0; idx < MATRIZ_SIZE; idx++) {
		sum += A[index_translate(cRow, idx)] * B[index_translate(idx, cCol)];
	}
	return sum;
}

int main(int argc, char **argv)
{
	unsigned int matrix_size=MATRIX_SIZE;
	if(argc==2){
		matrix_size = atoi(argv[1]);
		MATRIZ_SIZE = matrix_size;
	}
	clock_t start_t, end_t, total_t;

  int i, j;
	int myRank, commSize;
	int partitionSize, startIdx;
	int col_offset;
	int partitionsRemaining;
	int Bsource;
	double my_irecv_time = 0.0;
	double my_isend_time = 0.0;
	double my_compute_time = 0.0;
	int irecv_bytes = 0;
	int isend_bytes = 0;
	unsigned long long startSend, startRecv, startComp, finishSend, finishRecv, finishComp;

	double *A_ROWS;
	double *B_COLS;
	double *C_ROWS;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &commSize);
	MY_RANK = myRank;
	Bsource = myRank;

	partitionSize = matrix_size / commSize;
	startIdx = partitionSize * myRank;
	col_offset = partitionSize * myRank;
	partitionsRemaining = commSize;

	MPI_Request *requests = malloc(2 * sizeof(MPI_Request));

	double *recvBuffer = malloc( matrix_size * partitionSize * sizeof(double) );

	MPI_Datatype MPI_Partition;
	MPI_Type_contiguous(partitionSize * matrix_size, MPI_DOUBLE, &MPI_Partition);
	MPI_Type_commit(&MPI_Partition);

	A_ROWS = (double *) malloc( matrix_size * partitionSize * sizeof(double) );
	B_COLS = (double *) malloc( partitionSize * matrix_size * sizeof(double) );
	C_ROWS = (double *) malloc( matrix_size * partitionSize * sizeof(double) );

	printf("Rank %d com as linhas %d - %d\n", 
			myRank, startIdx, startIdx+partitionSize-1);

	for( i = 0; i < partitionSize; i++ ) {
  	for( j = 0; j < matrix_size; j++ ) {
			A_ROWS[index_translate(i, j)] = 1;
	    B_COLS[index_translate(j, i)] = 1;
	  }
	}


	//printf("[%d] Matriz A:\n", myRank);
	for (i = 0; i < partitionSize; i++) {
		for (j = 0; j < matrix_size; j++) {
			//printf("%5.5lf ", A_ROWS[index_translate(i, j)]);
		}
		//printf("\n");
	}

	//printf("[%d] Matriz B (%d - %d):\n", myRank, col_offset, col_offset+partitionSize-1);
	for (i = 0; i < matrix_size; i++) {
		for (j = 0; j < partitionSize; j++) {
			//printf("%5.2lf ", B_COLS[index_translate(i, j)]);
		}
		//printf("\n");
	}


	while (partitionsRemaining > 0) {
		printf("[%2d] %d / %d particoes faltando.\n", myRank, partitionsRemaining, commSize);
		if (partitionsRemaining != commSize) { //This isn't the first go	
			//Get the next B offset and partition
			//startRecv = rdtsc();
			int sourceRank = myRank -1;
			if (sourceRank == -1) { sourceRank = commSize-1; }
			MPI_Irecv(recvBuffer, 1, MPI_Partition, sourceRank, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[RECV]);
			Bsource--;
			if (Bsource == -1) { Bsource = commSize-1; }
			col_offset = partitionSize * Bsource;
			

			
			int destRank = (myRank+1) % commSize;
			MPI_Isend(B_COLS, 1, MPI_Partition, destRank, 1, MPI_COMM_WORLD, &requests[SEND]);
			printf("[%d] Enviando dados...\n", myRank);
			int completedIdx;
			for (i = 0; i < 2; i++) {
				printf("[%d] Esperando...\n", myRank);
				MPI_Waitany(2, requests, &completedIdx, MPI_STATUS_IGNORE);
				if (completedIdx == RECV) {
					
					printf("[%d] Recebido\n", myRank);
				} else if (completedIdx == SEND) {
					
					printf("[%d] Enviado\n", myRank);
				} else { printf("[%d] ERRO!\n", myRank); }
				requests[completedIdx] = MPI_REQUEST_NULL;
			}
			printf("[%d] Terminado worker\n", myRank);
			
			irecv_bytes += (matrix_size * partitionSize * sizeof(double));
			isend_bytes += (matrix_size * partitionSize * sizeof(double));
			my_isend_time += (finishSend - startSend) / CLOCK_RATE;
			my_irecv_time += (finishRecv - startRecv) / CLOCK_RATE;

			printf("[%d] Movendo buffer...\n", myRank);

			memcpy(B_COLS, recvBuffer, matrix_size * partitionSize * sizeof(double));
			printf("[%d] Terminado de mover o buffer\n", myRank);
			
		} 

		start_t = clock();
		printf("[%d] Calculando...\n", myRank);
		for (i = 0; i < partitionSize; i++) {
			for (j = 0; j < partitionSize; j++) {
				C_ROWS[index_translate(i, j+col_offset)] = matrix_mult( A_ROWS, B_COLS, i, j );
				
			}
		}
		printf("[%d] Calculado\n", myRank);
		my_compute_time += (finishComp - startComp) / CLOCK_RATE;
		partitionsRemaining--;
		
	}

	printf("[%d] Analisando\n", myRank);

	//int PRINT_CORRECTNESS_CHECK = 1;

	MPI_Barrier(MPI_COMM_WORLD);
	if (myRank == 0) {
		printf("Rank 0's parte da Matriz C (primeira %d linhas):\n", partitionSize);
		for (i = 0; i < partitionSize; i++) {
			for (j = 0; j < matrix_size; j++) {
				//printf("%5.5lf ", C_ROWS[index_translate(i, j)]);
			}
			//printf("\n");
		}
	}
	end_t = clock();
	double my_total_time = my_isend_time + my_irecv_time + my_compute_time;

	double my_isend_bw = (double) isend_bytes / my_isend_time;
	double my_irecv_bw = (double) irecv_bytes / my_irecv_time;
	double total_exec_time = 0.0;
	double max_isend_bw, min_isend_bw, sum_isend_bw, avg_isend_bw;
	double max_irecv_bw, min_irecv_bw, sum_irecv_bw, avg_irecv_bw;

	printf("[%d] Variaveis iniciadas.\n", myRank);
	total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
	my_total_time = total_t;
	printf("[%d] Termpo parcial: %8.7lf sec\n", myRank, my_total_time);

	MPI_Allreduce(&my_total_time, &total_exec_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

	MPI_Allreduce(&my_isend_bw, &max_isend_bw, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
	MPI_Allreduce(&my_isend_bw, &min_isend_bw, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
	MPI_Allreduce(&my_isend_bw, &sum_isend_bw, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	MPI_Allreduce(&my_irecv_bw, &max_irecv_bw, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
	MPI_Allreduce(&my_irecv_bw, &min_irecv_bw, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
	MPI_Allreduce(&my_irecv_bw, &sum_irecv_bw, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	printf("[%d] Allreduces Terminado!\n", myRank);

	if (myRank == 0) {
		printf("Rank 0 analise!\n");
		avg_isend_bw = sum_isend_bw / (double) commSize;
		avg_irecv_bw = sum_irecv_bw / (double) commSize;

		printf("Total exec time: %8.7lf seconds\n", total_exec_time);
		
		
		FILE *dataFile = fopen("saida.csv", "a");
		fprintf(dataFile, "%d %d %.2f\n",
				commSize, matrix_size,(double)total_t);
		printf("[0] Salvando arquivo...\n");
		fclose(dataFile);
	}
	printf("[%d] barreira...!\n", myRank);

	MPI_Barrier(MPI_COMM_WORLD);

	printf("[%d] Passou barreira\n", myRank); fflush(stdout);


	printf("[%d] Liberando buffer matriz\n", myRank); fflush(stdout);
//	free(A_ROWS);
//	free(B_COLS);
//	free(C_ROWS);
	printf("[%d] Liberando buffer receiver\n", myRank); fflush(stdout);
//	free(recvBuffer);
//	free(requests);
	printf("[%d] Liberado.\n", myRank); fflush(stdout);

	int rc = MPI_Finalize();
	printf("[%d] Finalizado codigo: %d\n", myRank, rc); fflush(stdout);
	return 0;
}
