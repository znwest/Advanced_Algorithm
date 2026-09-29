/*
 * 정렬 알고리즘 비교: 퀵 정렬 · 병합 정렬 · 힙 정렬 (C언어, 파일 하나)
 *
 * 한 번 실행하면 다음 세 가지를 차례로 보여준다.
 *   1) 정확성 검증  : 여러 입력에서 세 정렬의 결과가 맞는지 확인
 *   2) 안정성 검증  : 키가 같은 원소의 원래 순서가 유지되는지 확인
 *   3) 성능 비교표  : 입력 종류(무작위/정렬됨/역순)와 크기별로
 *                     실행 시간, 비교 횟수, 이동 횟수를 세 정렬 나란히 출력
 *                     (각 묶음에서 가장 작은 값에는 * 표시)
 *
 * 컴파일 : gcc -O2 -o sorting_comparison sorting_comparison.c
 * 실행   : ./sorting_comparison
 *
 * 통계 정의: 비교 횟수 = 원소 간 비교 횟수, 이동 횟수 = 원소 교환/대입 횟수
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ------------------------------------------------------------ 자료형 */

/* 안정성을 확인하기 위해 키(key)와 원래 위치 표식(tag)을 함께 둔다.
 * 정렬은 오직 key만 비교한다. */
typedef struct {
    int key;
    int tag;
} Item;

typedef struct {
    long long comparisons;
    long long moves;
} Stats;

typedef void (*SortFn)(Item *a, int n, Stats *st);

#define NUM_ALGO 3
static const char *ALGO_NAME[NUM_ALGO] = {"퀵", "병합", "힙"};

/* ------------------------------------------------------------ 퀵 정렬 */
/* Lomuto 분할, 마지막 원소를 피벗으로 사용.
 * 재귀 대신 명시적 스택을 사용해 최악의 경우에도 호출 스택이 넘치지 않게 했다.
 * (빈 구간은 넣지 않으므로 스택 깊이는 최대 n) */
static void quick_sort(Item *a, int n, Stats *st) {
    if (n <= 1) return;

    int (*stack)[2] = malloc(sizeof(int[2]) * (size_t)(n + 1));
    if (!stack) { fprintf(stderr, "메모리 부족\n"); exit(1); }
    int top = 0;
    stack[top][0] = 0;
    stack[top][1] = n - 1;
    top++;

    while (top > 0) {
        top--;
        int lo = stack[top][0];
        int hi = stack[top][1];
        if (lo >= hi) continue;

        int pivot = a[hi].key;
        int i = lo - 1;
        for (int j = lo; j < hi; j++) {
            st->comparisons++;
            if (a[j].key <= pivot) {
                i++;
                Item t = a[i]; a[i] = a[j]; a[j] = t;
                st->moves++;
            }
        }
        Item t = a[i + 1]; a[i + 1] = a[hi]; a[hi] = t;
        st->moves++;

        int p = i + 1;
        if (lo < p - 1) { stack[top][0] = lo;    stack[top][1] = p - 1; top++; }
        if (p + 1 < hi) { stack[top][0] = p + 1; stack[top][1] = hi;    top++; }
    }
    free(stack);
}

/* ------------------------------------------------------------ 병합 정렬 */
/* top-down, 임시 배열 사용, 안정 정렬. 구간은 [lo, hi) */
static void merge_rec(Item *a, Item *tmp, int lo, int hi, Stats *st) {
    if (hi - lo <= 1) return;
    int mid = lo + (hi - lo) / 2;
    merge_rec(a, tmp, lo, mid, st);
    merge_rec(a, tmp, mid, hi, st);

    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        st->comparisons++;
        if (a[i].key <= a[j].key) {      /* <= 이므로 안정 정렬 */
            tmp[k++] = a[i++];
        } else {
            tmp[k++] = a[j++];
        }
        st->moves++;
    }
    while (i < mid) { tmp[k++] = a[i++]; st->moves++; }
    while (j < hi)  { tmp[k++] = a[j++]; st->moves++; }

    for (k = lo; k < hi; k++) {          /* 임시 배열 -> 원본 복사 */
        a[k] = tmp[k];
        st->moves++;
    }
}

static void merge_sort(Item *a, int n, Stats *st) {
    if (n <= 1) return;
    Item *tmp = malloc(sizeof(Item) * (size_t)n);
    if (!tmp) { fprintf(stderr, "메모리 부족\n"); exit(1); }
    merge_rec(a, tmp, 0, n, st);
    free(tmp);
}

/* ------------------------------------------------------------ 힙 정렬 */
/* 최대 힙, 제자리 정렬 */
static void sift_down(Item *a, int i, int size, Stats *st) {
    for (;;) {
        int largest = i;
        int l = 2 * i + 1, r = 2 * i + 2;
        if (l < size) {
            st->comparisons++;
            if (a[l].key > a[largest].key) largest = l;
        }
        if (r < size) {
            st->comparisons++;
            if (a[r].key > a[largest].key) largest = r;
        }
        if (largest == i) return;
        Item t = a[i]; a[i] = a[largest]; a[largest] = t;
        st->moves++;
        i = largest;
    }
}

static void heap_sort(Item *a, int n, Stats *st) {
    for (int i = n / 2 - 1; i >= 0; i--)      /* 1단계: 최대 힙 만들기 O(n) */
        sift_down(a, i, n, st);
    for (int end = n - 1; end > 0; end--) {   /* 2단계: 루트(최댓값)를 뒤로 보내기 */
        Item t = a[0]; a[0] = a[end]; a[end] = t;
        st->moves++;
        sift_down(a, 0, end, st);
    }
}

static const SortFn ALGO_FN[NUM_ALGO] = {quick_sort, merge_sort, heap_sort};

/* ------------------------------------------------------------ 보조 함수 */

/* 현재 시각(초). timespec_get은 C11 표준이다. */
static double now_sec(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static int cmp_key(const void *x, const void *y) {
    int a = ((const Item *)x)->key, b = ((const Item *)y)->key;
    return (a > b) - (a < b);
}

/* 정렬 결과의 키 순서가 올바른지 검사 */
static int is_sorted_by_key(const Item *a, int n) {
    for (int i = 1; i < n; i++)
        if (a[i - 1].key > a[i].key) return 0;
    return 1;
}

/* 입력 만들기: kind 0=무작위, 1=이미 정렬됨, 2=역순 */
static const char *KIND_NAME[3] = {"무작위 입력", "이미 정렬된 입력", "역순 입력"};

static void make_data(Item *a, int n, int kind) {
    for (int i = 0; i < n; i++) {
        int key;
        if (kind == 0)      key = rand() % (n * 10 + 1);
        else if (kind == 1) key = i;
        else                key = n - i;
        a[i].key = key;
        a[i].tag = i;
    }
}

/* ------------------------------------------------------------ 1) 정확성 검증 */

static int run_correctness(void) {
    int fail = 0;
    int cases = 0;

    for (int a = 0; a < NUM_ALGO; a++) {
        cases = 0;
        /* 고정 케이스: 빈 배열, 원소 1개, 2개, 모두 같은 값, 정렬됨, 역순 */
        for (int n = 0; n <= 200; n++) {
            for (int kind = 0; kind < 4; kind++) {
                Item *src = malloc(sizeof(Item) * (size_t)(n + 1));
                Item *out = malloc(sizeof(Item) * (size_t)(n + 1));
                for (int i = 0; i < n; i++) {
                    src[i].tag = i;
                    if (kind == 0)      src[i].key = rand() % 201 - 100;
                    else if (kind == 1) src[i].key = 3;
                    else if (kind == 2) src[i].key = i;
                    else                src[i].key = n - i;
                }
                memcpy(out, src, sizeof(Item) * (size_t)n);

                Stats st = {0, 0};
                ALGO_FN[a](out, n, &st);

                /* 기준 결과: 표준 라이브러리 qsort의 키 순서와 비교 */
                Item *ref = malloc(sizeof(Item) * (size_t)(n + 1));
                memcpy(ref, src, sizeof(Item) * (size_t)n);
                qsort(ref, (size_t)n, sizeof(Item), cmp_key);
                for (int i = 0; i < n; i++) {
                    if (out[i].key != ref[i].key) { fail++; break; }
                }
                cases++;
                free(src); free(out); free(ref);
            }
        }
        printf("  %-4s %s (%d개 케이스)\n", ALGO_NAME[a],
               fail == 0 ? "통과" : "실패", cases);
    }
    return fail;
}

/* ------------------------------------------------------------ 2) 안정성 검증 */

static void run_stability(void) {
    enum { N = 300 };
    Item items[N], out[N];
    for (int i = 0; i < N; i++) {
        items[i].key = rand() % 6;   /* 키가 자주 겹치도록 0~5 */
        items[i].tag = i;            /* 원래 순서 */
    }
    for (int a = 0; a < NUM_ALGO; a++) {
        memcpy(out, items, sizeof(items));
        Stats st = {0, 0};
        ALGO_FN[a](out, N, &st);
        int stable = 1;
        for (int i = 1; i < N; i++)
            if (out[i - 1].key == out[i].key && out[i - 1].tag > out[i].tag) {
                stable = 0;
                break;
            }
        printf("  %-4s 안정 정렬 여부: %s\n", ALGO_NAME[a], stable ? "예" : "아니오");
    }
}

/* ------------------------------------------------------------ 3) 성능 비교 */

#define REPEAT 3   /* 시간은 REPEAT번 중 최솟값 사용 */

/* 가장 작은 값에 * 를 붙이기 위한 도우미 */
static int argmin_d(const double *v) {
    int m = 0;
    for (int i = 1; i < NUM_ALGO; i++) if (v[i] < v[m]) m = i;
    return m;
}
static int argmin_l(const long long *v) {
    int m = 0;
    for (int i = 1; i < NUM_ALGO; i++) if (v[i] < v[m]) m = i;
    return m;
}

static void run_benchmark(void) {
    const int sizes[] = {500, 1000, 2000, 4000, 8000};
    const int num_sizes = (int)(sizeof(sizes) / sizeof(sizes[0]));

    for (int kind = 0; kind < 3; kind++) {
        printf("\n[ %s ]\n", KIND_NAME[kind]);
        printf("+-------+----------------------------+--------------------------------------+--------------------------------------+\n");
        printf("|       |      실행 시간 (ms)        |             비교 횟수                |             이동 횟수                |\n");
        printf("|   n   |     퀵      병합      힙   |        퀵        병합         힙     |        퀵        병합         힙     |\n");
        printf("+-------+----------------------------+--------------------------------------+--------------------------------------+\n");

        for (int s = 0; s < num_sizes; s++) {
            int n = sizes[s];
            Item *data = malloc(sizeof(Item) * (size_t)n);
            Item *work = malloc(sizeof(Item) * (size_t)n);
            make_data(data, n, kind);

            double    tms[NUM_ALGO];
            long long cmp[NUM_ALGO], mov[NUM_ALGO];

            for (int a = 0; a < NUM_ALGO; a++) {
                double best = 1e30;
                Stats st = {0, 0};
                for (int r = 0; r < REPEAT; r++) {
                    memcpy(work, data, sizeof(Item) * (size_t)n);
                    st.comparisons = 0;
                    st.moves = 0;
                    double t0 = now_sec();
                    ALGO_FN[a](work, n, &st);
                    double dt = now_sec() - t0;
                    if (dt < best) best = dt;
                }
                if (!is_sorted_by_key(work, n)) {
                    fprintf(stderr, "오류: %s 정렬 결과가 틀렸습니다 (n=%d)\n", ALGO_NAME[a], n);
                    exit(1);
                }
                tms[a] = best * 1000.0;
                cmp[a] = st.comparisons;
                mov[a] = st.moves;
            }

            int bt = argmin_d(tms), bc = argmin_l(cmp), bm = argmin_l(mov);
            printf("| %5d |", n);
            for (int a = 0; a < NUM_ALGO; a++)
                printf(" %8.3f%c", tms[a], a == bt ? '*' : ' ');
            printf("|");
            for (int a = 0; a < NUM_ALGO; a++)
                printf(" %11lld%c", cmp[a], a == bc ? '*' : ' ');
            printf("|");
            for (int a = 0; a < NUM_ALGO; a++)
                printf(" %11lld%c", mov[a], a == bm ? '*' : ' ');
            printf("|\n");

            free(data);
            free(work);
        }
        printf("+-------+----------------------------+--------------------------------------+--------------------------------------+\n");
    }
    printf("\n* 표시는 각 항목에서 가장 작은(좋은) 값입니다.\n");
}

/* ------------------------------------------------------------ main */

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);   /* 윈도우 콘솔에서 한글이 깨지지 않게 UTF-8 설정 */
#endif
    srand(42);

    printf("==== 정렬 알고리즘 비교: 퀵 · 병합 · 힙 ====\n");

    printf("\n1) 정확성 검증\n");
    if (run_correctness() != 0) {
        printf("정확성 검증에 실패했습니다.\n");
        return 1;
    }

    printf("\n2) 안정성 검증\n");
    run_stability();

    printf("\n3) 성능 비교 (시간은 %d회 중 최솟값)\n", REPEAT);
    run_benchmark();

    return 0;
}
