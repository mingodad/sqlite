#include <stdio.h>
#include <ksu_common.h>
#include <ksu_my.h>

// Takes a char and int and returns number of chars from right
// using int parameter.
extern void my_strright(sqlite3_context * context,
                        int               argc,
                        sqlite3_value  ** argv) {
    int            number;
    unsigned char *word;
    unsigned char *w;
    int            len;
    int            skip;
    int            i;

    _ksu_null_if_null_param(argc, argv);
    word = (unsigned char *)sqlite3_value_text(argv[0]);
    number = my_value_int(argv[1], 0);
    len = ksu_charlen(word);
    if (number <= 0) {
      sqlite3_result_text(context, "", -1, SQLITE_STATIC);
    } else if (number >= len) {
      sqlite3_result_text(context, (char *)word, -1, NULL);
    } else {
      skip = len - number;
      w = word;
      for (i = 0; i < skip; i++) {
        SQLITE_SKIP_UTF8(w);
      }
      sqlite3_result_text(context, (char *)w, -1, NULL);
    }
}
