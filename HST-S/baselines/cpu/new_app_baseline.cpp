/*
* JGL@SAFARI
*/

/**
* @file app.c
* @brief Template for a Host Application Source File.
*
* The macros DPU_BINARY and NR_TASKLETS are directly
* used in the static functions, and are not passed as arguments of these functions.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <stdint.h>
#include <omp.h>

#include "../../support/common.h"
#include "../../support/timer.h"

// Pointer declaration
static T* A;
static unsigned long long* histo_host;

typedef struct Params {
    unsigned long long   input_size;
    unsigned long long   bins;
    int   n_warmup;
    int   n_reps;
    const char *file_name;
    int  exp;
    int  n_threads;
} Params;

/**
* @brief creates input arrays
* @param nr_elements how many elements in input arrays
*/
static void read_input(T* A, const Params p, unsigned long long input_size) {
    char dctFileName[100];
    FILE *File = NULL;

    // Open input file
    unsigned short temp;
    sprintf(dctFileName, p.file_name);
    if ((File = fopen(dctFileName, "rb")) != NULL) {
        for (unsigned long long y = 0; y < input_size; y++) {
            fread(&temp, sizeof(unsigned short), 1, File);
            A[y] = (unsigned long long)ByteSwap16(temp);
            if (A[y] >= 4096)
                A[y] = 4095;
        }
        fclose(File);
    } else {
        printf("%s does not exist\n", dctFileName);
        exit(1);
    }
}

/**
* @brief compute output in the host
*/
static void histogram_host(unsigned long long* histo, T* A, unsigned long long bins,
                             unsigned long long nr_elements, int exp,
                             unsigned long long nr_of_dpus, int t) {

    omp_set_num_threads(t);

    if (!exp) {
        #pragma omp parallel for
        for (unsigned long long i = 0; i < nr_of_dpus; i++) {
            for (unsigned long long j = 0; j < nr_elements; j++) {
                T d = A[j];
                histo[i * bins + ((d * bins) >> DEPTH)] += 1;
            }
        }
    } else {
        /* 
         * Instead of using "#pragma omp atomic update", we create a private histogram
         * for each thread, update that histogram without synchronization, and then reduce
         * (merge) the private histograms into the global one.
         */
        int nthreads = t;
        unsigned long long* local_histo = malloc(nthreads * bins * sizeof(unsigned long long));
        if (local_histo == NULL) {
            fprintf(stderr, "Memory allocation error\n");
            exit(1);
        }
        memset(local_histo, 0, nthreads * bins * sizeof(unsigned long long));
        printf("nr_elements : % llu\n",nr_elements);
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for
            for (unsigned long long j = 0; j < nr_elements; j++) {
                T d = A[j];
                local_histo[tid * bins + ((d * bins) >> DEPTH)]++;
            }
        }

        // Merge the private histograms into the global histogram
        for (int tid = 0; tid < nthreads; tid++) {
            for (unsigned long long bin = 0; bin < bins; bin++) {
                histo[bin] += local_histo[tid * bins + bin];
            }
        }
        free(local_histo);
    }
}

// Params ---------------------------------------------------------------------
void usage() {
    fprintf(stderr,
        "\nUsage:  ./program [options]"
        "\n"
        "\nGeneral options:"
        "\n    -h        help"
        "\n    -w <W>    # of untimed warmup iterations (default=1)"
        "\n    -e <E>    # of timed repetition iterations (default=3)"
        "\n    -t <T>    # of threads (default=8)"
        "\n    -x <X>    Weak (0) or strong (1) scaling (default=0)"
        "\n"
        "\nBenchmark-specific options:"
        "\n    -i <I>    input size (default=1536*1024 elements)"
        "\n    -b <B>    histogram size (default=256 bins)"
        "\n    -f <F>    input image file (default=../input/image_VanHateren.iml)"
        "\n");
}

struct Params input_params(int argc, char **argv) {
    struct Params p;
    p.input_size    = 1536 * 1024;
    p.bins          = 256;
    p.n_warmup      = 1;
    p.n_reps        = 3;
    p.n_threads     = 8;
    p.exp           = 1;
    p.file_name     = "../../input/image_VanHateren.iml";

    int opt;
    while ((opt = getopt(argc, argv, "hi:b:w:e:f:x:t:")) >= 0) {
        switch (opt) {
        case 'h':
            usage();
            exit(0);
            break;
        case 'i': p.input_size = atoi(optarg); break;
        case 'b': p.bins       = atoi(optarg); break;
        case 'w': p.n_warmup   = atoi(optarg); break;
        case 'e': p.n_reps     = atoi(optarg); break;
        case 'f': p.file_name  = optarg; break;
        case 'x': p.exp        = atoi(optarg); break;
        case 't': p.n_threads  = atoi(optarg); break;
        default:
            fprintf(stderr, "\nUnrecognized option!\n");
            usage();
            exit(0);
        }
    }
    assert(p.n_threads > 0 && "Invalid # of threads!");

    return p;
}

/**
* @brief Main of the Host Application.
*/
int main(int argc, char **argv) {

    struct Params p = input_params(argc, argv);

    /* In this example, we set nr_of_dpus to 1.
     * In a real multi-DPU environment, you would set it accordingly.
     */
    uint32_t nr_of_dpus = 1;

    const unsigned long long input_size = 25600000000; // Size of input image
    assert(input_size % p.n_threads == 0 && "Input size!");

    // Input/output allocation
    A = malloc(input_size * sizeof(T));
    if (A == NULL) {
        fprintf(stderr, "Memory allocation error for A\n");
        exit(1);
    }

    if (!p.exp)
        histo_host = malloc(nr_of_dpus * p.bins * sizeof(unsigned long long));
    else
        histo_host = malloc(p.bins * sizeof(unsigned long long));

    if (histo_host == NULL) {
        fprintf(stderr, "Memory allocation error for histo_host\n");
        exit(1);
    }

    // Create an input file with arbitrary data.
    read_input(A, p, input_size);

    Timer timer;
    start(&timer, 0, 0);

    if (!p.exp)
        memset(histo_host, 0, nr_of_dpus * p.bins * sizeof(unsigned long long));
    else
        memset(histo_host, 0, p.bins * sizeof(unsigned long long));

    histogram_host(histo_host, A, p.bins, input_size, p.exp, nr_of_dpus, p.n_threads);

    stop(&timer, 0);
    printf("Kernel ");
    print(&timer, 0, 1);
    printf("\n");
    
    // Free allocated memory
    free(A);
    free(histo_host);

    return 0;
}
