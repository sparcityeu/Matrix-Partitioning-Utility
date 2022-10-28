#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "patoh.h"
#include "smsh.c"
#include "mmio.c"

void PrintInfo(int _k, int *partweights, int cut, int _nconst, char *base_file_name, char *output_file_name, int non_zero_count, int row_count, int col_count)
{
    double *avg, *maxi, maxall = -1.0;
    int i, j;

    FILE *info_fp;

    info_fp = fopen(output_file_name, "w");
    fprintf(info_fp, "File Name: %s\n", base_file_name);
    fprintf(info_fp, "Non-Zero Count: %d\n", non_zero_count);
    fprintf(info_fp, "Row and Column Count (M x N): %d and %d\n", row_count, col_count);
    fprintf(info_fp, "-------------------------------------------------------------------");
    fprintf(info_fp, "\n Partitioner: %s", (_nconst > 1) ? "Multi-Constraint" : "Single-Constraint");
    fprintf(info_fp, "\n %d-way cutsize = %d \n", _k, cut);

    avg = (double *)malloc(sizeof(double) * _nconst);
    maxi = (double *)malloc(sizeof(double) * _nconst);
    for (i = 0; i < _nconst; ++i)
        maxi[i] = avg[i] = 0.0;
    for (i = 0; i < _k; ++i)
        for (j = 0; j < _nconst; ++j)
            avg[j] += partweights[i * _nconst + j];
    for (i = 0; i < _nconst; ++i)
    {
        maxi[i] = 0.0;
        avg[i] /= (double)_k;
    }

    for (i = 0; i < _k; ++i)
    {
        // fprintf(info_fp, "\n %3d :", i);
        for (j = 0; j < _nconst; ++j)
        {
            double im = (double)((double)partweights[i * _nconst + j] - avg[j]) / avg[j];

            maxi[j] = (maxi[j] > im) ? maxi[j] : im;
            //     fprintf(info_fp, "%10d ", partweights[i * _nconst + j]);
        }
    }

    for (j = 0; j < _nconst; ++j)
        maxall = (maxi[j] > maxall) ? maxi[j] : maxall;
    fprintf(info_fp, "\n MaxImbals are (as %%): %.3lf", 100.0 * maxall);
    fprintf(info_fp, "\n      ");
    for (i = 0; i < _nconst; ++i)
        fprintf(info_fp, "%10.1lf \n", 100.0 * maxi[i]);

    fprintf(info_fp, "\n PartWeights are:\n");

    for (i = 0; i < _k; ++i)
    {
        fprintf(info_fp, "\n %3d :", i);
        for (j = 0; j < _nconst; ++j)
        {
            fprintf(info_fp, "%10d ", partweights[i * _nconst + j]);
        }
    }

    fprintf(info_fp, "\n");
    fclose(info_fp);
    free(maxi);
    free(avg);
}

int main(int argc, char *argv[])
{
    clock_t program_begins = clock();
    /* Declarations for reading the matrix */

    int ret_code;
    MM_typecode matcode;
    FILE *f;
    int M, N, nz;
    int i, *I_complete, *J_complete;

    /* Declarations for measuring time */
    double elapsedTime = 0.0;
    double elapsed_time_patoh_part_only = 0.0;

    /* Processing the input */

    if (argc != 6)
    {
        printf("Could not process arguments.\n");
        printf("Usage: [martix-market-pathname] [partition-type] [desired amount of partitions] [partition-quality] [seed]\n");
        exit(1);
    }
    else
    {

        if ((f = fopen(argv[1], "r")) == NULL)
        {
            printf("Could not locate the matrix file. Please make sure the pathname is valid.\n");
            exit(1);
        }

        if ((strcmp(argv[2], "conpart") != 0) && (strcmp(argv[2], "cutpart") != 0))
        {
            printf("Please either use \"conpart\" or \"cutpart\" as partitioning type.\n");
            exit(1);
        }
        if ((strcmp(argv[4], "speed") != 0) && (strcmp(argv[4], "default") != 0) && (strcmp(argv[4], "quality") != 0))
        {
            printf("Please either use \"speed\", \"default\" or \"quality\" as partitioning quality.\n");
            exit(1);
        }
    }

    if (mm_read_banner(f, &matcode) != 0)
    {
        printf("Could not process Matrix Market banner.\n");
        exit(1);
    }

    if ((ret_code = mm_read_mtx_crd_size(f, &M, &N, &nz)) != 0)
    {
        printf("Could not read matrix dimensions.\n");
        exit(1);
    }

    if ((strcmp(matcode, "MCRG") == 0) || (strcmp(matcode, "MCIG") == 0) || (strcmp(matcode, "MCPG") == 0) || (strcmp(matcode, "MCCG") == 0))
    {

        I_complete = (int *)calloc(nz, sizeof(int));
        J_complete = (int *)calloc(nz, sizeof(int));

        for (i = 0; i < nz; i++)
        {
            fscanf(f, "%d %d", &I_complete[i], &J_complete[i]);
            fscanf(f, "%*[^\n]\n");
            /* adjust from 1-based to 0-based */
            I_complete[i]--;
            J_complete[i]--;
        }
    }

    /* If the matrix is symmetric, we need to construct the other half */

    else if ((strcmp(matcode, "MCRS") == 0) || (strcmp(matcode, "MCIS") == 0) || (strcmp(matcode, "MCPS") == 0) || (strcmp(matcode, "MCCS") == 0) || (strcmp(matcode, "MCCH") == 0) || (strcmp(matcode, "MCRK") == 0))
    {

        I_complete = (int *)calloc(2 * nz, sizeof(int));
        J_complete = (int *)calloc(2 * nz, sizeof(int));

        int i_index = 0;

        for (i = 0; i < nz; i++)
        {
            fscanf(f, "%d %d", &I_complete[i], &J_complete[i]);
            fscanf(f, "%*[^\n]\n");

            if (I_complete[i] == J_complete[i])
            {
                /* adjust from 1-based to 0-based */
                I_complete[i]--;
                J_complete[i]--;
            }
            else
            {
                /* adjust from 1-based to 0-based */
                I_complete[i]--;
                J_complete[i]--;
                J_complete[nz + i_index] = I_complete[i];
                I_complete[nz + i_index] = J_complete[i];
                i_index++;
            }
        }
        nz += i_index;
    }
    else
    {
        printf("This matrix type is not supported: %s \n", matcode);
        exit(1);
    }

    /* We need the values to be sorted with respect to column index to convert COO to CSC */

    if (!isSorted(I_complete, J_complete, nz))
    {
        quicksort(I_complete, J_complete, nz);
    }

    /**** Create output file names ****/

    char partvecFileName[256];
    memset(partvecFileName, 0, 256 * sizeof(char));
    strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(partvecFileName, "PaToH_"), basename(argv[1])), "_"), argv[2]), "_k"), argv[3]), "_"), argv[4]), "_s"), argv[5]), "_partvec.txt");

    char partinfoFileName[256];
    memset(partinfoFileName, 0, 256 * sizeof(char));
    strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(partinfoFileName, "PaToH_"), basename(argv[1])), "_"), argv[2]), "_k"), argv[3]), "_"), argv[4]), "_s"), argv[5]), "_partinfo.txt");

    char timeinfoFileName[256];
    memset(timeinfoFileName, 0, 256 * sizeof(char));
    strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(timeinfoFileName, "PaToH_"), basename(argv[1])), "_"), argv[2]), "_k"), argv[3]), "_"), argv[4]), "_s"), argv[5]), "_timeinfo.txt");

    /* Part that uses PaToH */

    PaToH_Parameters args;
    int _c, _n, _nconst, *cwghts, *nwghts, *xpins, *pins, *partvec, cut, *partweights;

    _c = M;
    _n = N;
    _nconst = 1;

    cwghts = (int *)malloc(_c * sizeof(int));
    for (int i = 0; i < _c; i++)
    {
        cwghts[i] = 1;
    }

    nwghts = (int *)malloc(_n * sizeof(int));
    for (int i = 0; i < _n; i++)
    {
        nwghts[i] = 1;
    }

    // nwghts = NULL;

    /* Convert COO to CSC, and write the column pointer and row indices to xpins and pins respectivelly. */

    xpins = (int *)calloc(_n + 1, sizeof(int));
    pins = (int *)calloc(nz, sizeof(int));

    for (i = 0; i < nz; i++)
    {
        pins[i] = I_complete[i];
        xpins[J_complete[i] + 1]++;
    }
    for (i = 0; i < _n + 1; i++)
    {
        xpins[i + 1] += xpins[i];
    }

    free(I_complete);
    free(J_complete);

    /*
        FILE *xpins_fp;

        xpins_fp = fopen("xpins_file.txt", "w");

        for (i = 0; i < N + 1; i++)
        {
            fprintf(xpins_fp, "%d\n", xpins[i]);
        }
        fclose(xpins_fp);

        FILE *pins_fp;

        pins_fp = fopen("pins_file.txt", "w");

        for (i = 0; i < nz; i++)
        {
            fprintf(pins_fp, "%d\n", pins[i]);
        }
        fclose(pins_fp);
     */

    partvec = (int *)malloc(_c * sizeof(int));
    partweights = (int *)malloc(atoi(argv[3]) * _nconst * sizeof(int));

    if (!strcmp(argv[2], "conpart"))
    {

        if (!strcmp(argv[4], "quality"))
        {
            PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_QUALITY);
        }
        else if (!strcmp(argv[4], "speed"))
        {
            PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_SPEED);
        }
        else if (!strcmp(argv[4], "default"))
        {
            PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_DEFAULT);
        }
        else
        {
            printf("An error occured during PaToH initilization. Please check your input parameters. \n");
            exit(1);
        }
    }
    else if (!strcmp(argv[2], "cutpart"))
    {
        if (!strcmp(argv[4], "quality"))
        {
            PaToH_Initialize_Parameters(&args, PATOH_CUTPART, PATOH_SUGPARAM_QUALITY);
        }
        else if (!strcmp(argv[4], "speed"))
        {
            PaToH_Initialize_Parameters(&args, PATOH_CUTPART, PATOH_SUGPARAM_SPEED);
        }
        else if (!strcmp(argv[4], "default"))
        {
            PaToH_Initialize_Parameters(&args, PATOH_CUTPART, PATOH_SUGPARAM_DEFAULT);
        }
        else
        {
            printf("An error occured during PaToH initilization. Please check your input parameters. \n");
            exit(1);
        }
    }
    else
    {
        printf("Please use either 'conpart' or 'cutpart' as the partition type.\n");
        exit(1);
    }

    args._k = atoi(argv[3]);
    args.seed = atoi(argv[5]);

    PaToH_Alloc(&args, _c, _n, _nconst, cwghts, nwghts, xpins, pins);
    clock_t partitioning_begins = clock();
    PaToH_Part(&args, _c, _n, _nconst, 0, cwghts, nwghts, xpins, pins, NULL, partvec, partweights, &cut);
    clock_t partitioning_ends = clock();

    /***** Write the output files *****/

    FILE *partvec_fp;
    partvec_fp = fopen(partvecFileName, "w");
    for (int i = 0; i < _c; i++)
    {
        fprintf(partvec_fp, "%d\n", partvec[i]);
    }
    fclose(partvec_fp);

    PrintInfo(args._k, partweights, cut, _nconst, basename(argv[1]), partinfoFileName, nz, M, N);

    clock_t program_ends = clock();

    elapsedTime += (double)(program_ends - program_begins) / CLOCKS_PER_SEC;
    elapsed_time_patoh_part_only += (double)(partitioning_ends - partitioning_begins) / CLOCKS_PER_SEC;

    FILE *elapsedTime_fp;
    elapsedTime_fp = fopen(timeinfoFileName, "w");
    fprintf(elapsedTime_fp, "File Name: %s\n", basename(argv[1]));
    fprintf(elapsedTime_fp, "Non-Zero Count: %d\n", nz);
    fprintf(elapsedTime_fp, "Row and Column Count (M x N): %d and %d\n", M, N);
    fprintf(elapsedTime_fp, "Program took %.2f seconds to complete including IO operations.\n", elapsedTime);
    fprintf(elapsedTime_fp, "PaToH took %.2f seconds to partition the matrix.\n", elapsed_time_patoh_part_only);

    fclose(elapsedTime_fp);

    /***** Exit the program *****/
    free(cwghts);
    free(nwghts);
    free(partweights);
    free(partvec);
    PaToH_Free();
    exit(0);
}
