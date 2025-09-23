// cc test_buffer.c -pthread -std=c11 -O2 && ./a.out
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// -------- Include your code exactly as-is --------
#include "ring_buffer.h" // your header posted above (with the three tiny fixes applied)
// ------------------------------------------------

#define CHECK(cond, msg)                                               \
    do                                                                 \
    {                                                                  \
        if (!(cond))                                                   \
        {                                                              \
            fprintf(stderr, "❌ FAIL: %s (line %d)\n", msg, __LINE__); \
            exit(1);                                                   \
        }                                                              \
    } while (0)

static inline void tiny_sleep_us(int max_us)
{
    if (max_us <= 0)
        return;
    usleep(rand() % max_us);
}

// ----------------- Test 1: SPSC -----------------
typedef struct
{
    int start, count;
    atomic_long *sum;
} ProdArgs;
typedef struct
{
    int total;
    atomic_long *sum;
} ConsArgs;

void *producer_fn(void *argp)
{
    ProdArgs *a = (ProdArgs *)argp;
    for (int i = 0; i < a->count; i++)
    {
        int v = a->start + i;
        enqueue(&buffer, v);
        atomic_fetch_add(a->sum, v);
        tiny_sleep_us(200);
    }
    return NULL;
}

void *consumer_fn(void *argp)
{
    ConsArgs *a = (ConsArgs *)argp;
    long local = 0;
    for (int i = 0; i < a->total; i++)
    {
        // Your dequeue() discards the value; we’ll read then zero ourselves
        // So: we’ll grab the current head index under the assumption enqueue/dequeue are serialized by your sem/mutex.
        // Since your API doesn’t return an item, we’ll just assume items were integers pushed by producer and
        // count how many we consumed via a separate counter outside (Test 2). For SPSC we can peek before zeroing.
        // But your dequeue() already zeros the slot, so we can’t read a value here without changing your API.
        // For SPSC correctness we’ll just perform a dequeue and track count.
        dequeue(&buffer);
    }
    atomic_fetch_add(a->sum, local);
    return NULL;
}

static void test_spsc_basic()
{
    printf("TEST 1: SPSC basic...\n");
    init();               // semaphores
    init_buffer(&buffer); // ring indices & array

    atomic_long psum = 0, csum = 0;
    const int N = 10000;

    ProdArgs pa = {.start = 1, .count = N, .sum = &psum};
    ConsArgs ca = {.total = N, .sum = &csum};

    pthread_t pt, ct;
    pthread_create(&pt, NULL, producer_fn, &pa);
    pthread_create(&ct, NULL, consumer_fn, &ca);
    pthread_join(pt, NULL);
    pthread_join(ct, NULL);

    // We can’t compare sums because your dequeue() discards the value.
    // Instead, assert the buffer ends empty and indices advanced by N.
    CHECK(buffer.end - buffer.start == 0, "queue not empty after balanced SPSC");
    printf("  ✅ passed\n");
}

// ------------- Test 2: MPMC correctness -------------
typedef struct
{
    int id, count;
    atomic_int *produced;
} MPProdArgs;
typedef struct
{
    int id, count;
    atomic_int *consumed;
} MPConsArgs;

void *mp_producer(void *argp)
{
    MPProdArgs *a = (MPProdArgs *)argp;
    for (int i = 0; i < a->count; i++)
    {
        enqueue(&buffer, a->id * 1000000 + i); // stamp items with producer id
        atomic_fetch_add(a->produced, 1);
        tiny_sleep_us(300);
    }
    return NULL;
}
void *mp_consumer(void *argp)
{
    MPConsArgs *a = (MPConsArgs *)argp;
    for (int i = 0; i < a->count; i++)
    {
        dequeue(&buffer);
        atomic_fetch_add(a->consumed, 1);
        tiny_sleep_us(300);
    }
    return NULL;
}

static void test_mpmc_balanced()
{
    printf("TEST 2: MPMC 4P/4C balanced...\n");
    init();
    init_buffer(&buffer);

    const int P = 4, C = 4, PER = 5000;
    pthread_t pt[P], ct[C];
    MPProdArgs pa[P];
    MPConsArgs ca[C];
    atomic_int produced = 0, consumed = 0;

    for (int i = 0; i < P; i++)
    {
        pa[i] = (MPProdArgs){.id = i, .count = PER, .produced = &produced};
        pthread_create(&pt[i], NULL, mp_producer, &pa[i]);
    }
    for (int i = 0; i < C; i++)
    {
        ca[i] = (MPConsArgs){.id = i, .count = (P * PER) / C, .consumed = &consumed};
        pthread_create(&ct[i], NULL, mp_consumer, &ca[i]);
    }

    for (int i = 0; i < P; i++)
        pthread_join(pt[i], NULL);
    for (int i = 0; i < C; i++)
        pthread_join(ct[i], NULL);

    int prod = atomic_load(&produced);
    int cons = atomic_load(&consumed);

    CHECK(prod == P * PER, "produced count mismatch");
    CHECK(cons == P * PER, "consumed count mismatch");
    CHECK(buffer.end - buffer.start == 0, "queue not empty after balanced MPMC");
    printf("  ✅ passed\n");
}

// -------- Test 3: Wrap-around / capacity pressure --------
static void test_wrap_pressure()
{
    printf("TEST 3: wrap-around pressure...\n");
    init();
    init_buffer(&buffer);

    // Fill exactly BUFFER_SIZE, then drain, repeated
    for (int round = 0; round < 1000; ++round)
    {
        for (int i = 0; i < BUFFER_SIZE; i++)
            enqueue(&buffer, i);
        for (int i = 0; i < BUFFER_SIZE; i++)
            dequeue(&buffer);
    }

    CHECK(buffer.end - buffer.start == 0, "queue not empty after wrap test");
    printf("  ✅ passed\n");
}

int main(void)
{
    srand((unsigned)time(NULL));
    test_spsc_basic();
    test_mpmc_balanced();
    test_wrap_pressure();
    printf("\n🎉 All tests passed for your API.\n");
    return 0;
}
