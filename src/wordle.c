#define WORD_LENGTH 5U
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <Python.h>
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress(double percentage)
{
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}
typedef char String5[WORD_LENGTH];
enum Colour
{
    BLACK,
    YELLOW,
    GREEN
};
struct guess_t
{
    String5 word;
    uint8_t colours[WORD_LENGTH];
};
struct string_vector_t
{
    String5 *data;
    size_t count;
};

int alloc_count = 0;

struct guess_t make_guess(const char *restrict const word, const char *restrict const _answer)
{
    uint8_t colours[WORD_LENGTH];
    String5 answer;
    memcpy(answer, _answer, WORD_LENGTH); // sounds expensive but it's just moving 4 bytes then 1

    // tick off green letters then invalidate them so they don't get double counted as yellow
    for (unsigned i = 0; i < WORD_LENGTH; i++)
    {
        if (word[i] == answer[i])
        {
            colours[i] = GREEN;
            answer[i] = 0;
        }
    }

    for (unsigned i = 0; i < WORD_LENGTH; i++)
    {
        // was green so can't be yellow or black
        if (!answer[i])
            continue;
        char *c;
        if ((c = memchr(answer, word[i], WORD_LENGTH)))
        {
            colours[i] = YELLOW;
            *c = 1;
        }
        else
        {
            colours[i] = BLACK;
        }
    }
    struct guess_t g;
    memcpy(g.word, word, WORD_LENGTH);
    memcpy(g.colours, colours, WORD_LENGTH);
    return g;
}

uint8_t colours_to_number(const uint8_t colours[WORD_LENGTH])
{
    uint8_t sum = 0;
    for (unsigned i = 0; i < WORD_LENGTH; i++)
    {
        sum = sum * 3 + colours[i];
    }
    return sum;
}

bool passes_filter(const String5 _test, const struct guess_t *const filter)
{
    String5 test;
    memcpy(test, _test, WORD_LENGTH);

    for (unsigned i = 0; i < WORD_LENGTH; i++)
    {
        if (filter->colours[i] == GREEN)
        {
            if (test[i] != filter->word[i])
                return false;
            else
                test[i] = 0;
        }
    }
    for (unsigned i = 0; i < WORD_LENGTH; i++)
    {
        const char *const pos = memchr(test, filter->word[i], WORD_LENGTH);
        if (pos && (filter->colours[i] == BLACK))
            return false;
        else if (filter->colours[i] == YELLOW)
        {
            if (!pos)
                return false; // yellow color not found in test word
            else if (pos - test == i)
                return false; // would have been green if the word was correct
            else
                test[pos - test] = 0; // remove this character so that next iteration needs to look for different yellow character if there are two
        }
    }
    return true;
}

void filter_words(struct string_vector_t *words, const struct guess_t *const filter)
{
    size_t idx = 0;
    for (unsigned i = 0; i < words->count; i++)
    {
        if (passes_filter(words->data[i], filter))
            memcpy(words->data[idx++], words->data[i], WORD_LENGTH);
    }
    words->count = idx;
}
struct string_vector_t copy_words(const struct string_vector_t *words)
{
    struct string_vector_t ret;
    unsigned N = WORD_LENGTH * words->count;
    ret.count = words->count;
    ret.data = malloc(N);
    memcpy(ret.data, words->data, N); // for 2000 words, this is 40 sets of 256 bit chunks
    return ret;
}
void free_words(struct string_vector_t *words)
{
    words->count = 0;
    free(words->data);
}

typedef void (*sighandler_t)(int);
volatile bool stop = false;
void handle_sigint(int _)
{
    (void)_;
    stop = true;
}

int game_length(struct string_vector_t *const words, int depth, String5 *winner)
{
    sighandler_t old_handler = signal(SIGINT, handle_sigint);
    static int best;
    if (depth == 0)
    {
        stop = false;
        best = INT_MAX;
        printProgress(0);
    }
    int outer_minimum = best - depth;
    String5 best_word = {0};
    // If there are 1 or 2 words, the game length is trivial
    if (words->count <= 2)
    {
        // word size shouldn't be zero unless the word isn't in the list
        outer_minimum = words->count;
        memcpy(best_word, words->data[0], WORD_LENGTH);
        goto ret;
    }
    // Abort if the search depth is too high
    if (depth + 1 == best)
        return 1; // if it's solvable in 4 and I'm at a depth of 3, just pretend this is another solution of 4 and it will be ignored.
    if (depth == best)
        return 0; // if it's solvable in 4 and I'm at a depth of 4, pretend this is another solution of 4 and it will be ignored
    unsigned counter = 0;
#pragma omp parallel for if (depth == 0)
    for (unsigned i = 0; i < words->count; i++)
    {
        if (outer_minimum == 2 || stop)
            continue; // can't break due to omp
        int inner_maximum = 0;
        bool checked[243] = {0};
        for (unsigned j = 0; j < words->count; j++)
        {
            if (i == j)
                continue;
            struct guess_t g = make_guess(words->data[i], words->data[j]);
            if (checked[colours_to_number(g.colours)])
                continue;
            checked[colours_to_number(g.colours)] = true;
            struct string_vector_t new_words = copy_words(words);
            filter_words(&new_words, &g);
            int length = game_length(&new_words, depth + 1, NULL) + 1;
            free_words(&new_words);
            inner_maximum = length > inner_maximum ? length : inner_maximum;
            if (inner_maximum >= outer_minimum)
                break;
        }
        if (inner_maximum < outer_minimum)
        {
            outer_minimum = inner_maximum;
            if (depth == 0)
            {
                memcpy(best_word, words->data[i], WORD_LENGTH);
                best = outer_minimum;
            }
        }
        if (depth == 0)
            printProgress((double)++counter / (double)words->count);
    }

    signal(SIGINT, old_handler);
ret:
    if (!depth && winner)
    {
        printProgress(1.);
        puts("");
        memcpy(*winner, best_word, WORD_LENGTH);
    }
    return outer_minimum;
}
