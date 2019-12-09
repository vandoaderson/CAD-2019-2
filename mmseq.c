#include<stdio.h>
#include<stdlib.h>
#include <time.h>

#define MATSIZE 5000

int main()
{
		clock_t time; 
        int *ans,*first,*second;
        int *A,*B,*C;
        int i,j,k=0;
        int rowA = MATSIZE,
        	colA = MATSIZE,
        	sizeA,
        	sizeB,
        	sizeC;
        int rowB = MATSIZE,
        	colB = MATSIZE;

       


        sizeC = rowA*colB;
        sizeA = rowA*colA;
        sizeB = rowB*colB;

        A  = (int *)malloc(sizeA*sizeof(int *));
        first = A;

        B = (int *)malloc(sizeB*sizeof(int *));
        second = B;

        C    = (int *)malloc(sizeC*sizeof(int *));
        ans = C;


        

        for(i=0;i<sizeA;i++,first++)
        	*first = 1;


        for(i=0;i<sizeB;i++,second++)
        	*second = 1;

        first=A;        
        second= B;      

        time = clock();
        if(rowA==1 && colB==1)
        {
            for(i=0;i<rowA;i++)
            {
                for(j=0;j<colB;j++)
                {
                *ans=0;
                for(k=0;k<rowB;k++)
                    *ans = *ans + (*(first + (k + i*colA))) * (*(second + (j+k*colB)));
                ans++;
                }//j
            }//i
        }//if

    else
    {
        for(i=0;i<rowA;i++)
        {
        for(j=0;j<colB;j++)
        {
            *ans=0;
            for(k=0;k<rowB;k++)
                *ans = *ans + (*(first + (k + i*colA))) * (*(second + (j+k*rowB)));
            ans++;
        }//j
        }//i

        }
        time = clock() - time;
        printf("Tempo total : %.2f segundos \n", ((double)time)/CLOCKS_PER_SEC);
        //printf("\nvalor da matriz 'C' = \n");

        ans = C;

        //for(i=0;i<rowA;i++)
        //{
        // printf("\n");
        // for(j=0;j<colB;j++,ans++)
        // printf("%d\t",*ans);
        // }

        free(A);
        free(B);
        free(C);
        
 }
