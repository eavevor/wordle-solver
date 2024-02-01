#ifndef _WORDLE_H
#define WORD_LENGTH 5
#include <stdint.h>
#include <stddef.h>
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
struct guess_t make_guess(const char *restrict const word, const char *restrict const _answer);
void filter_words(struct string_vector_t *words, const struct guess_t *const filter);
void free_words(struct string_vector_t *words);
int game_length(struct string_vector_t *const words, int depth, String5 *winner);
#endif
