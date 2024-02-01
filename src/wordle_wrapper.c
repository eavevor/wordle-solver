#include "wordle.h"
#include <Python.h>

struct string_vector_t parse_list(PyObject *listObj)
{
    struct string_vector_t words = {0};

    int numLines = PyList_Size(listObj);
    if (numLines <= 0)
    {
        PyErr_SetString(PyExc_ValueError, "Something went wrong (list of size less than one)");
        return words;
    }
    words.count = numLines;
    words.data = malloc(words.count * WORD_LENGTH);
    for (int i = 0; i < numLines; i++)
    {
        PyObject *strObj = PyList_GetItem(listObj, i);
        const char *line = PyUnicode_AsUTF8(strObj);
        if (strlen(line) != WORD_LENGTH)
        {
            PyErr_Format(PyExc_ValueError, "All elements in the list must be %d characters long (got \"%.10s\")", WORD_LENGTH, line);
            free_words(&words);
            return words;
        }
        for (int j = 0; j < WORD_LENGTH; j++)
            words.data[i][j] = toupper(line[j]);
    }
    return words;
}
#define optimal_word_wrapper_docstr                                                                           \
    "Given a list of words, find the one that can solve the game in the fewest guesses in the worst case\n\n" \
    ":param list words: The list of words over which the optimal is searched\n"                               \
    ":return: The optimum amd the game length\n"                                                              \
    "rtype: tuple[int,str]\n"
static PyObject *optimal_word_wrapper(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *listObj;

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &listObj))
    {
        return NULL;
    }
    struct string_vector_t words = parse_list(listObj);
    if (!words.count)
    {
        return NULL;
    }
    union
    {
        String5 s5;
        char s6[6];
    } winner = {0};
    int len = game_length(&words, 0, &winner.s5);
    free_words(&words);
    PyObject *ret = PyTuple_New(2);
    PyTuple_SetItem(ret, 0, PyLong_FromLong(len));
    PyTuple_SetItem(ret, 1, Py_BuildValue("s", winner.s6));
    return ret;
}

#define filter_words_wrapper_docstr                                                                                                                         \
    "Given a list of words, a guess and its corresponding colours, find a smaller list of words that could have produced those colours from that guess\n\n" \
    ":param list words: The list of words to filter\n"                                                                                                      \
    ":param str guess: The word that was guessed\n"                                                                                                         \
    ":param str colours: The colours (e.g. yggkg) from that guess (g, y, k) are the available colours\n"                                                    \
    ":return: The whittled down liist\n"                                                                                                                    \
    "rtype: list\n"
static PyObject *filter_words_wrapper(PyObject *Py_UNUSED(self), PyObject *args)
{
    char *word, *colours;
    PyObject *listObj;

    if (!PyArg_ParseTuple(args, "O!ss", &PyList_Type, &listObj, &word, &colours))
        return NULL;
    for (unsigned i = 0; i < WORD_LENGTH; i++)
    {
        word[i] = toupper(word[i]);
        colours[i] = toupper(colours[i]);
    }
    struct string_vector_t words = parse_list(listObj);
    if (!words.count)
        return NULL;

    uint8_t c[WORD_LENGTH];

    for (unsigned i = 0; i < WORD_LENGTH; i++)
    {
        switch (colours[i])
        {
        case 'G':
            c[i] = GREEN;
            break;
        case 'K':
            c[i] = BLACK;
            break;
        case 'Y':
            c[i] = YELLOW;
            break;

        default:
            PyErr_Format(PyExc_RuntimeError, "Unknown colour %c", colours[i]);
            return NULL;
        }
    }

    struct guess_t g;
    memcpy(g.word, word, WORD_LENGTH);
    memcpy(g.colours, c, WORD_LENGTH);

    filter_words(&words, &g);
    PyObject *ret = PyList_New(words.count);

    for (unsigned i = 0; i < words.count; i++)
    {
        char w[WORD_LENGTH + 1];
        memcpy(w, words.data[i], WORD_LENGTH);
        w[WORD_LENGTH] = 0;
        PyList_SetItem(ret, i, Py_BuildValue("s", w));
    }
    free_words(&words);

    return ret;
}

static PyMethodDef game_length_wrapper_methods[] = {
    {"get_optimal_word", optimal_word_wrapper, METH_VARARGS, optimal_word_wrapper_docstr},
    {"filter_words", filter_words_wrapper, METH_VARARGS, filter_words_wrapper_docstr},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef wordle_solver_wrapper_module = {
    PyModuleDef_HEAD_INIT,
    "wordle_wolver",
    "Python interface wordle solver",
    -1,
    game_length_wrapper_methods,
    NULL,
    NULL,
    NULL,
    NULL};

PyMODINIT_FUNC PyInit_wordle_solver(void)
{
    return PyModule_Create(&wordle_solver_wrapper_module);
}
