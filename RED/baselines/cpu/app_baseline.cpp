/*
* JGL@SAFARI
*/

/**
* CPU code with Thrust
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <math.h>
#include <sys/time.h>

#include <vector>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/scan.h>
#include <thrust/copy.h>
#include <thrust/system/omp/execution_policy.h>
#include <thrust/system/omp/vector.h>

#include <omp.h>

#include "../../support/common.h"
#include "../../support/timer.h"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Pointer declaration
static T* A;


/**
* @brief creates input arrays
* @param nr_elements how many elements in input arrays
*/
static void read_input(T* A, unsigned long long nr_elements) {
    srand(0);
    printf("nr_elements : %llu\n", nr_elements);
    
    for (unsigned long long i = 0; i < nr_elements; i++) {
        //printf("========%llu========",i);
        A[i] = (T)(1);
    }
   
}

/**
* @brief compute output in the host
*/
static T reduction_host(T* A, unsigned long long  nr_elements) {
    T tmp = 0;
    for(int i=0;i<10;++i){
        for (unsigned long long i = 0; i < nr_elements; i++) {
            tmp += A[i];
        }
    }
    return tmp;
}

static T reduction_host_parallel(T* A, unsigned long long  nr_elements, int t) {
    T tmp = 0;
    omp_set_num_threads(t);
    
    
    #pragma omp parallel for reduction(+:tmp)
    for (unsigned long long i = 0; i < nr_elements; i++) {
        tmp += A[i];
    }
    return tmp;
}
// Params ---------------------------------------------------------------------
typedef struct Params {
    unsigned long long   input_size;
    int   n_warmup;
    int   n_reps;
    int   exp;
    int   n_threads;
}Params;

void usage() {
    fprintf(stderr,
        "\nUsage:  ./program [options]"
        "\n"
        "\nGeneral options:"
        "\n    -h        help"
        "\n    -w <W>    # of untimed warmup iterations (default=1)"
        "\n    -e <E>    # of timed repetition iterations (default=3)"
        "\n    -x <X>    Weak (0) or strong (1) scaling (default=0)"
        "\n    -t <T>    # of threads (default=8)"
        "\n"
        "\nBenchmark-specific options:"
        "\n    -i <I>    input size (default=2M elements)"
        "\n");
}

struct Params input_params(int argc, char **argv) {
    struct Params p;
    p.input_size    = 2 << 20;
    p.n_warmup      = 1;
    p.n_reps        = 3;
    p.exp           = 0;
    p.n_threads     = 1;

    int opt;
    while((opt = getopt(argc, argv, "hi:w:e:x:t:")) >= 0) {
        switch(opt) {
        case 'h':
        usage();
        exit(0);
        break;
        case 'i': p.input_size    = atoi(optarg); break;
        case 'w': p.n_warmup      = atoi(optarg); break;
        case 'e': p.n_reps        = atoi(optarg); break;
        case 'x': p.exp           = atoi(optarg); break;
        case 't': p.n_threads     = atoi(optarg); break;
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

    unsigned int nr_of_dpus = 1;
    
    unsigned int i = 0;
    //const unsigned int input_size = p.exp == 0 ? p.input_size * nr_of_dpus : p.input_size;
    const unsigned long long input_size = 1280000000;
    assert(input_size % (p.n_threads) == 0 && "Input size!");

    // Input/output allocation
    A = (T*)malloc(input_size * sizeof(T));
    if( A == NULL ) {
    // 無法取得記憶體空間
    fprintf(stderr, "Error: unable to allocate required memory\n");
    return 1;
  }

    // Create an input file with arbitrary data.
    read_input(A, input_size);
    // printf("read input finish\n");
    // Timer declaration
    Timer timer;
    float time_gpu = 0;
    T count ;
    T count_host ;
    // Loop over main kernel
    for(int rep = 0; rep < p.n_warmup + p.n_reps; rep++) {
        count = 0;
        count_host = 0;

        // Compute output on CPU (performance comparison and verification purposes)
        if(rep >= p.n_warmup)
            start(&timer, 0, rep - p.n_warmup);
        count_host = reduction_host(A, input_size);
        if(rep >= p.n_warmup)
            stop(&timer, 0);

        if(rep >= p.n_warmup)
            start(&timer, 1, rep - p.n_warmup);
        for(int i=0;i<10;++i){
            count += reduction_host_parallel(A, input_size , p.n_threads);
        }
        if(rep >= p.n_warmup)
            stop(&timer, 1);
        // printf("count_final_result : %llu\n",count);
    }

    // Print timing results
    printf("CPU ");
    print(&timer, 0, p.n_reps);
    printf("Kernel ");
    print(&timer, 1, p.n_reps);
    printf("%llu -- %llu\n", count, count_host);
    // Check output
    bool status = true;
    if(count_host != count){ 
        status = false;
        printf("%llu -- %llu\n", count, count_host);
    }
	if (status) {
        printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] Outputs are equal\n");
    } else {
        printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] Outputs differ!\n");
    }

    // Deallocation
    free(A);
   
	
    return 0;
}