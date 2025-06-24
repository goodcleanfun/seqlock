#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "greatest/greatest.h"
#include "threading/threading.h"

#include "seqlock.h"

static seqlock_t seqlock;
static int counter = 0;

#define READ_THREADS 16
#define WRITE_THREADS 4
#define MAX_COUNTER 100

int increment_counter_thread(void *arg) {
    uint64_t msec = (uint64_t)arg;
    bool done = false;
    while (1) {
        seqlock_write_lock(&seqlock);
        if (counter == MAX_COUNTER) {
            done = true;
        } else {
            counter++;
        }
        seqlock_write_unlock(&seqlock);
        if (done) break;
        thrd_sleep(&(struct timespec){.tv_nsec = msec * 1000000}, NULL);
    }
    return 0;
}

int read_counter_thread(void *arg) {
    int last_counter = -1;
    while (1) {
        uint64_t sequence = seqlock_read(&seqlock);
        if (last_counter != counter && seqlock_read_is_valid(&seqlock, sequence)) {
            last_counter = counter;
        }
        if (last_counter == MAX_COUNTER) break;
    }
    return 0;
}


TEST seqlock_test(void) {
    seqlock_init(&seqlock);
    thrd_t write_threads[WRITE_THREADS];
    thrd_t read_threads[READ_THREADS];
    for (int i = 0; i < WRITE_THREADS; i++) {
        ASSERT_EQ(thrd_success, thrd_create(&write_threads[i], increment_counter_thread, (void *)10));
    }
    for (int i = 0; i < READ_THREADS; i++) {
        ASSERT_EQ(thrd_success, thrd_create(&read_threads[i], read_counter_thread, NULL));
    }
    for (int i = 0; i < WRITE_THREADS; i++) {
        thrd_join(write_threads[i], NULL);
    }
    for (int i = 0; i < READ_THREADS; i++) {
        thrd_join(read_threads[i], NULL);
    }

    ASSERT_EQ(counter, MAX_COUNTER);

    PASS();
}

// Main test suite
SUITE(seqlock_tests) {
    RUN_TEST(seqlock_test);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(seqlock_tests);

    GREATEST_MAIN_END();
}