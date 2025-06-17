#ifndef SEQLOCK_H
#define SEQLOCK_H

#include <stdatomic.h>
#include "cpu_relax/cpu_relax.h"
#include "spinlock/spinlock.h"


typedef struct seqlock {
    atomic_uint_fast64_t sequence;
    spinlock_t lock;
} seqlock_t;


static inline void seqlock_init(seqlock_t *seqlock) {
    atomic_init(&seqlock->sequence, 0);
    spinlock_t lock = SPINLOCK_INIT;
    seqlock->lock = lock;
}

static inline void seqlock_write_lock(seqlock_t *seqlock) {
    // acquire exclusive lock from other writers
    spinlock_lock(&seqlock->lock);
    // increment sequence once to indicate to readers that a write is in progress
    atomic_fetch_add(&seqlock->sequence, 1);
}

static inline void seqlock_write_unlock(seqlock_t *seqlock) {
    // increment the sequence again to signal to readers that the write is complete
    atomic_fetch_add(&seqlock->sequence, 1);
    // release the spinlock to allow other writers
    spinlock_unlock(&seqlock->lock);
}

static inline uint64_t seqlock_read_sequence(seqlock_t *seqlock) {
    while (1) {
        atomic_uint_fast64_t start = atomic_load(&seqlock->sequence);
        // if sequence is odd, a write is in progress
        if (start & 1) {
            cpu_relax();
            continue;
        }
        return start;
    }
}

static inline bool seqlock_read_is_valid(seqlock_t *seqlock, uint64_t sequence) {
    atomic_uint_fast64_t current = atomic_load(&seqlock->sequence);
    return sequence == (uint64_t)current;
}

#endif