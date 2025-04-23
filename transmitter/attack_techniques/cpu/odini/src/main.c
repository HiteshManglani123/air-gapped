#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <sched.h>

// Global variables
pthread_mutex_t *mutex;
int *vector;
int vector_length;
int f;
int nCycles;

// Function prototypes
void *workerThread(void *arg);
void signal(int bit, int f, int nCycles);
void busywait(int ms);

int BindThreadToCore(int core_id) {
    cpu_set_t cpuset;
    pthread_t current_thread = pthread_self();

    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int result = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        return -1;
    }

    return 0;
}

// Algorithm 1: Transmit
void transmit(int nWorkers, int *vec, int freq, int cycles) {
    vector = vec;
    f = freq;
    nCycles = cycles;
    
    mutex = (pthread_mutex_t*)malloc(nWorkers * sizeof(pthread_mutex_t));
    pthread_t *threads = (pthread_t*)malloc(nWorkers * sizeof(pthread_t));

    for (int i = 0; i < nWorkers; i++) {
        pthread_mutex_init(&mutex[i], NULL);
        pthread_mutex_lock(&mutex[i]);
        pthread_create(&threads[i], NULL, workerThread, (void*)(intptr_t)i);
    }

    for (int j = 0; j < nWorkers; j++) {
        pthread_mutex_unlock(&mutex[j]);
    }

    // Wait for threads to finish
    for (int i = 0; i < nWorkers; i++) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    free(mutex);
    free(threads);
}

// Algorithm 2: Workerthread
void *workerThread(void *arg) {
    int thread_id = (intptr_t)arg;
    
    BindThreadToCore(thread_id);

    pthread_mutex_lock(&mutex[thread_id]);
    
    for (int i = 0; i < vector_length; i++) {
        signal(vector[i], f, nCycles);
    }
    
    pthread_mutex_unlock(&mutex[thread_id]);
    
    return NULL;
}

// Algorithm 3: Signal
void signal(int bit, int f, int nCycles) {
    if (bit == 1) {
        for (int j = 0; j < nCycles; j++) {
            busywait((1000 / f) * 0.5);
            usleep((1000000 / f) * 0.5);  // sleep for microseconds
        }
    } else {
        usleep(nCycles * (1000000 / f));  // sleep for microseconds
    }
}

// Algorithm 4: Busywait
void busywait(int ms) {
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int i = 0;
    do {
        clock_gettime(CLOCK_MONOTONIC, &current);
	i++;
    } while ((current.tv_sec - start.tv_sec) * 1000 + 
             (current.tv_nsec - start.tv_nsec) / 1000000 < ms);
}

// Main function for testing
int main() {
    int nWorkers = 8;
    int testVector[] = {1, 1};
    vector_length = sizeof(testVector) / sizeof(testVector[0]);
    int frequency = 1000;  // 1 kHz
    int cycles = 1000000;

    transmit(nWorkers, testVector, frequency, cycles);

    return 0;
}

