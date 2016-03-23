#ifndef ORA_AGGR_COVAR_H

#define ORA_AGGR_COVAR_H

#include <ksu_common.h>

typedef struct {
  double       sum1;
  double       sum2;
  double       sum_prod;
  unsigned int n;
} COVAR_CONTEXT_T;

extern void ora_covar_step(sqlite3_context *context,
                           int              argc,
                           sqlite3_value  **argv);

#endif
