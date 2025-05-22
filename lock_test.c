#include <stdio.h>
#include <pthread.h>

pthread_mutex_t lock;

void* thread_func(void* arg) {
    pthread_mutex_lock(&lock);
    printf("Thread %d is in the critical section.\n", *(int*)arg);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main() {
    pthread_t threads[2];
    int thread_ids[2] = {1, 2};
    
    pthread_mutex_init(&lock, NULL);

    for(int i = 0; i < 2; i++) {
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    for(int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    return 0;
}

