#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define MATSIZE 1000
#define NRA MATSIZE
#define NCA MATSIZE            /* numero de colunas da matriz A */
#define NCB MATSIZE            /* numero de colunas da matriz B */
#define MASTER 0               /* taskid da tarefa1 */
#define FROM_MASTER 1          /* tipo de mensagem */
#define FROM_WORKER 2          /* tipo de mensagem */

int main (int argc, char *argv[])
{
int     numtasks,              /* numero de tarefas */
        taskid,                /* o id da tarefa */
        numworkers,            /* numero de workers */
        source,                /* id da tarefa do remetente  */
        dest,                  /* id da tarefa de destino */
        mtype,                 /* tipo de mensagem */
        rows,                  /* linhas da matriz a enviadas para cada worker */
        averow, extra, offset, /* usado para determinar as linhas enviadas para cada worker */
        i, j, k, rc;
double  a[NRA][NCA],           /* matriz A */
        b[NCA][NCB],           /* matriz B */
        c[NRA][NCB];           /* resultado matriz C */
MPI_Status status;

MPI_Init(&argc,&argv);
MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
if (numtasks < 2 ) {
  printf("Precisa de no minimo 2...\n");
  MPI_Abort(MPI_COMM_WORLD, rc);
  exit(1);
  }
numworkers = numtasks-1;


/**************************** master task ************************************/
if (taskid == MASTER)
   {
      printf("mpi_mm iniciou com %d tasks.\n",numtasks);
      // printf("Inicializando vetores...\n");
      for (i=0; i<NRA; i++)
         for (j=0; j<NCA; j++)
            a[i][j]= i+j;
      for (i=0; i<NCA; i++)
         for (j=0; j<NCB; j++)
            b[i][j]= i*j;

      /*Contando o tempo */
      double start = MPI_Wtime();

      /* Enviando dados da matriz */
      averow = NRA/numworkers;
      extra = NRA%numworkers;
      offset = 0;
      mtype = FROM_MASTER;
      for (dest=1; dest<=numworkers; dest++)
      {
         rows = (dest <= extra) ? averow+1 : averow;    
         // printf("Enviando %d linhas para task %d offset=%d\n",rows,dest,offset);
         MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&a[offset][0], rows*NCA, MPI_DOUBLE, dest, mtype,
                   MPI_COMM_WORLD);
         MPI_Send(&b, NCA*NCB, MPI_DOUBLE, dest, mtype, MPI_COMM_WORLD);
         offset = offset + rows;
      }

      /* Recebendo dados dos workers */
      mtype = FROM_WORKER;
for (i=1; i<=numworkers; i++)
      {
         source = i;
         MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&c[offset][0], rows*NCB, MPI_DOUBLE, source, mtype, 
                  MPI_COMM_WORLD, &status);
         // printf("Recebendo resultado das tasks %d\n",source);
      }

      /* Resultado */
      /*
      printf("******************************************************\n");
      printf("Resultado Matriz:\n");
      for (i=0; i<NRA; i++)
      {
         printf("\n"); 
         for (j=0; j<NCB; j++) 
            printf("%6.2f   ", c[i][j]);
      }
      printf("\n******************************************************\n");
      */

      /* Tempo de execucao */
      double finish = MPI_Wtime();
      printf("Terminado em %f segundos.\n", finish - start);
   }
/**************************** worker task ************************************/
   if (taskid > MASTER)
   {
      mtype = FROM_MASTER;
      MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&a, rows*NCA, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&b, NCA*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);

      for (k=0; k<NCB; k++)
         for (i=0; i<rows; i++)
         {
            c[i][k] = 0.0;
            for (j=0; j<NCA; j++)
               c[i][k] = c[i][k] + a[i][j] * b[j][k];
         }
      mtype = FROM_WORKER;
      MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&c, rows*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD);
   }
   MPI_Finalize();
}

