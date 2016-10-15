/*
This library will provide common mathematical and string functions in
SQL queries using the operating system libraries or provided
definitions.  It includes the following functions:

Math: acos, asin, atan, atn2, atan2, acosh, asinh, atanh, difference,
degrees, radians, cos, sin, tan, cot, cosh, sinh, tanh, coth, exp,
log, log10, power, sign, sqrt, square, ceil, floor, pi.

String: replicate, charindex, leftstr, rightstr, ltrim, rtrim, trim,
replace, reverse, proper, padl, padr, padc, strfilter.

Aggregate: samplestdev, stdev, variance, mode, median, lower_quartile,
upper_quartile.

The string functions ltrim, rtrim, trim, replace are included in
recent versions of SQLite and so by default do not build.

Compilation instructions:
 Compile this C source file into a dynamic library as follows:
 * Linux:
   gcc -fPIC -lm -shared extension-functions.c -o libsqlitefunctions.so
 * Mac OS X:
   gcc -fno-common -dynamiclib extension-functions.c -o libsqlitefunctions.dylib
 (You may need to add flags
  -I /opt/local/include/ -L/opt/local/lib -lsqlite3
  if your sqlite3 is installed from Mac ports, or
  -I /sw/include/ -L/sw/lib -lsqlite3
  if installed with Fink.)
 * Windows:
  1. Install MinGW (http://www.mingw.org/) and you will get the gcc
  (gnu compiler collection)
  2. add the path to your path variable (isn't done during the
   installation!)
  3. compile:
   gcc -shared -I "path" -o libsqlitefunctions.so extension-functions.c
   (path = path of sqlite3ext.h; i.e. C:\programs\sqlite)

Usage instructions for applications calling the sqlite3 API functions:
  In your application, call sqlite3_enable_load_extension(db,1) to
  allow loading external libraries.  Then load the library libsqlitefunctions
  using sqlite3_load_extension; the third argument should be 0.
  See http://www.sqlite.org/cvstrac/wiki?p=LoadableExtensions.
  Select statements may now use these functions, as in
  SELECT cos(radians(inclination)) FROM satsum WHERE satnum = 25544;

Usage instructions for the sqlite3 program:
  If the program is built so that loading extensions is permitted,
  the following will work:
   sqlite> SELECT load_extension('./libsqlitefunctions.so');
   sqlite> select cos(radians(45));
   0.707106781186548
  Note: Loading extensions is by default prohibited as a
  security measure; see "Security Considerations" in
  http://www.sqlite.org/cvstrac/wiki?p=LoadableExtensions.
  If the sqlite3 program and library are built this
  way, you cannot use these functions from the program, you
  must write your own program using the sqlite3 API, and call
  sqlite3_enable_load_extension as described above, or else
  rebuilt the sqlite3 program to allow loadable extensions.

Alterations:
The instructions are for Linux, Mac OS X, and Windows; users of other
OSes may need to modify this procedure.  In particular, if your math
library lacks one or more of the needed trig or log functions, comment
out the appropriate HAVE_ #define at the top of file.  If you do not
wish to make a loadable module, comment out the define for
COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE.  If you are using a
version of SQLite without the trim functions and replace, comment out
the HAVE_TRIM #define.

Liam Healy

History:
2010-01-06 Correct check for argc in squareFunc, and add Windows
compilation instructions.
2009-06-24 Correct check for argc in properFunc.
2008-09-14 Add check that memory was actually allocated after
sqlite3_malloc or sqlite3StrDup, call sqlite3_result_error_nomem if
not.  Thanks to Robert Simpson.
2008-06-13 Change to instructions to indicate use of the math library
and that program might work.
2007-10-01 Minor clarification to instructions.
2007-09-29 Compilation as loadable module is optional with
COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE.
2007-09-28 Use sqlite3_extension_init and macros
SQLITE_EXTENSION_INIT1, SQLITE_EXTENSION_INIT2, so that it works with
sqlite3_load_extension.  Thanks to Eric Higashino and Joe Wilson.
New instructions for Mac compilation.
2007-09-17 With help from Joe Wilson and Nuno Luca, made use of
external interfaces so that compilation is no longer dependent on
SQLite source code.  Merged source, header, and README into a single
file.  Added casts so that Mac will compile without warnings (unsigned
and signed char).
2007-09-05 Included some definitions from sqlite 3.3.13 so that this
will continue to work in newer versions of sqlite.  Completed
description of functions available.
2007-03-27 Revised description.
2007-03-23 Small cleanup and a bug fix on the code.  This was mainly
letting errno flag errors encountered in the math library and checking
the result, rather than pre-checking.  This fixes a bug in power that
would cause an error if any non-positive number was raised to any
power.
2007-02-07 posted by Mikey C to sqlite mailing list.
Original code 2006 June 05 by relicoder.

*/
#ifdef SQLITE_ENABLE_EXTENSION_FUNCTIONS
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_EXTENSION_FUNCTIONS)

#define HAVE_ACOSH 1
#define HAVE_ASINH 1
#define HAVE_ATANH 1
#define HAVE_SINH 1
#define HAVE_COSH 1
#define HAVE_TANH 1
#define HAVE_LOG10 1
#define HAVE_ISBLANK 1
#define SQLITE_SOUNDEX 1
#define HAVE_TRIM 1		/* LMH 2007-03-25 if sqlite has trim functions */

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
  #include "sqlite3ext.h"
  SQLITE_EXTENSION_INIT1
#else
  #include "sqlite3.h"
  #include "sqliteInt.h"
#endif

#include <ctype.h>
/* relicoder */
#include <math.h>
#include <string.h>
#include <stdio.h>
#ifndef _WIN32_WCE
/* LMH 2007-03-25 */
#include <errno.h>
#endif

#include <stdlib.h>

#ifndef _WIN32_WCE
#include <assert.h>
#endif

#ifndef _MAP_H_
#define _MAP_H_

#if !defined(SQLITE_OS_WIN) || !defined(_MSC_VER)
#include <stdint.h>
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed int int16_t;
typedef unsigned int uint16_t;
typedef signed long int int32_t;
typedef unsigned long int uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

/*
** Simple binary tree implementation to use in median, mode and quartile calculations
** Tree is not necessarily balanced. That would require something like red&black trees of AVL
*/

typedef int(*cmp_func)(const void *, const void *);
typedef void(*map_iterator)(void*, sqlite_int64, void*);

typedef struct node{
  struct node *l;
  struct node *r;
  void* data;
  sqlite_int64 count;
} node;

typedef struct map{
  node *base;
  cmp_func cmp;
  short free;
} map;

/*
** creates a map given a comparison function
*/
map map_make(cmp_func cmp);

/*
** inserts the element e into map m
*/
void map_insert(map *m, void *e);

/*
** executes function iter over all elements in the map, in key increasing order
*/
void map_iterate(map *m, map_iterator iter, void* p);

/*
** frees all memory used by a map
*/
void map_destroy(map *m);

/*
** compares 2 integers
** to use with map_make
*/
int int_cmp(const void *a, const void *b);

/*
** compares 2 doubles
** to use with map_make
*/
int sqlite_double_cmp(const void *a, const void *b);

#endif /* _MAP_H_ */

#if !SQLITE_CORE
typedef uint8_t         u8;
typedef uint16_t        u16;
typedef sqlite_int64         i64;
#endif

static char *sqlite3StrDup( const char *z ) {
    char *res = sqlite3_malloc( strlen(z)+1 );
    return strcpy( res, z );
}

/*
** These are copied verbatim from fun.c so as to not have the names exported
*/

/* LMH from sqlite3 3.3.13 */
/*
** This table maps from the first byte of a UTF-8 character to the number
** of trailing bytes expected. A value '4' indicates that the table key
** is not a legal first byte for a UTF-8 character.
*/
static const u8 xtra_utf8_bytes[256]  = {
/* 0xxxxxxx */
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,

/* 10wwwwww */
4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,
4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,
4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,
4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,

/* 110yyyyy */
1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,

/* 1110zzzz */
2, 2, 2, 2, 2, 2, 2, 2,     2, 2, 2, 2, 2, 2, 2, 2,

/* 11110yyy */
3, 3, 3, 3, 3, 3, 3, 3,     4, 4, 4, 4, 4, 4, 4, 4,
};


/*
** This table maps from the number of trailing bytes in a UTF-8 character
** to an integer constant that is effectively calculated for each character
** read by a naive implementation of a UTF-8 character reader. The code
** in the READ_UTF8 macro explains things best.
*/
static const int xtra_utf8_bits[] =  {
  0,
  12416,          /* (0xC0 << 6) + (0x80) */
  925824,         /* (0xE0 << 12) + (0x80 << 6) + (0x80) */
  63447168        /* (0xF0 << 18) + (0x80 << 12) + (0x80 << 6) + 0x80 */
};

/*
** If a UTF-8 character contains N bytes extra bytes (N bytes follow
** the initial byte so that the total character length is N+1) then
** masking the character with utf8_mask[N] must produce a non-zero
** result.  Otherwise, we have an (illegal) overlong encoding.
*/
static const int utf_mask[] = {
  0x00000000,
  0xffffff80,
  0xfffff800,
  0xffff0000,
};

/* LMH salvaged from sqlite3 3.3.13 source code src/utf.c */
#define EXTFUNC_READ_UTF8(zIn, c) { \
  int xtra;                                            \
  c = *(zIn)++;                                        \
  xtra = xtra_utf8_bytes[c];                           \
  switch( xtra ){                                      \
    case 4: c = (int)0xFFFD; break;                    \
    case 3: c = (c<<6) + *(zIn)++;                     \
    case 2: c = (c<<6) + *(zIn)++;                     \
    case 1: c = (c<<6) + *(zIn)++;                     \
    c -= xtra_utf8_bits[xtra];                         \
    if( (utf_mask[xtra]&c)==0                          \
        || (c&0xFFFFF800)==0xD800                      \
        || (c&0xFFFFFFFE)==0xFFFE ){  c = 0xFFFD; }    \
  }                                                    \
}

static int sqlite3ReadUtf8(const unsigned char *z){
  int c;
  EXTFUNC_READ_UTF8(z, c);
  return c;
}

#define SKIP_UTF8(zIn) {                               \
  zIn += (xtra_utf8_bytes[*(u8 *)zIn] + 1);            \
}

#if 0
/*
** pZ is a UTF-8 encoded unicode string. If nByte is less than zero,
** return the number of unicode characters in pZ up to (but not including)
** the first 0x00 byte. If nByte is not less than zero, return the
** number of unicode characters in the first nByte of pZ (or up to
** the first 0x00, whichever comes first).
*/
static int sqlite3Utf8CharLen(const char *z, int nByte){
  int r = 0;
  const char *zTerm;
  if( nByte>=0 ){
    zTerm = &z[nByte];
  }else{
    zTerm = (const char *)(-1);
  }
  assert( z<=zTerm );
  while( *z!=0 && z<zTerm ){
    SKIP_UTF8(z);
    r++;
  }
  return r;
}
#endif

/*
** X is a pointer to the first byte of a UTF-8 character.  Increment
** X so that it points to the next character.  This only works right
** if X points to a well-formed UTF-8 string.
*/
#define sqliteNextChar(X)  while( (0xc0&*++(X))==0x80 ){}
#define sqliteCharVal(X)   sqlite3ReadUtf8(X)

/*
** This is a macro that facilitates writting wrappers for math.h functions
** it creates code for a function to use in SQlite that gets one numeric input
** and returns a floating point value.
**
** Could have been implemented using pointers to functions but this way it's inline
** and thus more efficient. Lower * ranking though...
**
** Parameters:
** name:      function name to de defined (eg: sinFunc)
** function:  function defined in math.h to wrap (eg: sin)
** domain:    boolean condition that CAN'T happen in terms of the input parameter rVal
**            (eg: rval<0 for sqrt)
*/

#ifdef _WIN32_WCE
#ifdef NO_CLEAR_FP
#define CLEAR_ERRNO(a)
#define HAS_ERRNO(a)  (0 == (a))
#else
#define CLEAR_ERRNO(a) ( _clearfp())
#define HAS_ERRNO(a)  (_statusfp() == (a))
#endif
#define strerror(x)  "Error ocurred"
#define errno (GetLastError ())

#ifndef isinf
static sqlite_double zero(void) { return LITDBL(0.0); }
static sqlite_double one (void) { return LITDBL(1.0); }
static sqlite_double inf (void) { return one() / zero(); }

int
isinf(sqlite_double n)
{
    static sqlite_double pinf = LITDBL(0.0);
    static sqlite_double ninf = LITDBL(0.0);

    if (pinf == LITDBL(0.0)) {
	pinf = inf();
	ninf = -pinf;
    }
    return memcmp(&n, &pinf, sizeof n) == 0
	|| memcmp(&n, &ninf, sizeof n) == 0;
}
#endif

/* DBL_MANT_DIG must be less than 4 times of bits of int */
#ifndef DBL_MANT_DIG
#define DBL_MANT_DIG 53		/* in this case, at least 12 digit precision */
#endif
#define BIG_CRITERIA_BIT (1<<DBL_MANT_DIG/2)
#if BIG_CRITERIA_BIT > 0
#define BIG_CRITERIA (1.0*BIG_CRITERIA_BIT)
#else
#define BIG_CRITERIA (1.0*(1<<DBL_MANT_DIG/4)*(1<<(DBL_MANT_DIG/2+1-DBL_MANT_DIG/4)))
#endif
#define SMALL_CRITERIA_BIT (1<<(DBL_MANT_DIG/3))
#if SMALL_CRITERIA_BIT > 0
#define SMALL_CRITERIA (1.0/SMALL_CRITERIA_BIT)
#else
#define SMALL_CRITERIA (1.0*(1<<DBL_MANT_DIG/4)*(1<<(DBL_MANT_DIG/3+1-DBL_MANT_DIG/4)))
#endif

sqlite_double
acosh(sqlite_double x)
{
    if (x < 1)
	x = -1;			/* NaN */
    else if (x == 1)
	return 0;
    else if (x > BIG_CRITERIA)
	x += x;
    else
	x += sqrt((x + 1) * (x - 1));
    return log(x);
}

sqlite_double
asinh(sqlite_double x)
{
    int neg = x < 0;
    sqlite_double z = fabs(x);

    if (z < SMALL_CRITERIA) return x;
    if (z < (LITDBL(1.0)/(1<<DBL_MANT_DIG/5))) {
	sqlite_double x2 = z * z;
	z *= 1 + x2 * (LITDBL(-1.0)/LITDBL(6.0) + x2 * LITDBL(3.0)/LITDBL(40.0));
    }
    else if (z > BIG_CRITERIA) {
	z = log(z + z);
    }
    else {
	z = log(z + sqrt(z * z + 1));
    }
    if (neg) z = -z;
    return z;
}

sqlite_double
atanh(sqlite_double x)
{
    int neg = x < 0;
    sqlite_double z = fabs(x);

    if (z < SMALL_CRITERIA) return x;
    z = log(z > 1 ? -1 : (1 + z) / (1 - z)) / 2;
    if (neg) z = -z;
    if (isinf(z))
#if defined(ERANGE)
	errno = ERANGE;
#elif defined(EDOM)
	errno = EDOM;
#else
	;
#endif
    return z;
}

#else
#define CLEAR_ERRNO(a) (errno = (a))
#define HAS_ERRNO(a) (errno == (a))
#endif

/* LMH 2007-03-25 Changed to use errno and remove domain; no pre-checking for errors. */
#define GEN_MATH_WRAP_DOUBLE_1(name, function) \
static void name(sqlite3_context *context, int argc, sqlite3_value **argv){\
  sqlite_double rVal = LITDBL(0.0), val;\
  assert( argc==1 );\
  switch( sqlite3_value_type(argv[0]) ){\
    case SQLITE_NULL: {\
      sqlite3_result_null(context);\
      break;\
    }\
    default: {\
      rVal = sqlite3_value_double(argv[0]);\
      CLEAR_ERRNO(0);\
      val = function(rVal);\
      if (HAS_ERRNO(0)) {\
        sqlite3_result_double(context, val);\
      } else {\
        sqlite3_result_error(context, strerror(errno), errno);\
      }\
      break;\
    }\
  }\
}\


/*
** Example of GEN_MATH_WRAP_DOUBLE_1 usage
** this creates function sqrtFunc to wrap the math.h standard function sqrt(x)=x^0.5
*/
GEN_MATH_WRAP_DOUBLE_1(sqrtFunc, sqrt)

/* trignometric functions */
GEN_MATH_WRAP_DOUBLE_1(acosFunc, acos)
GEN_MATH_WRAP_DOUBLE_1(asinFunc, asin)
GEN_MATH_WRAP_DOUBLE_1(atanFunc, atan)

/*
** Many of systems don't have inverse hyperbolic trig functions so this will emulate
** them on those systems in terms of log and sqrt (formulas are too trivial to demand
** written proof here)
*/

#ifndef HAVE_ACOSH
static sqlite_double acosh(sqlite_double x){
  return log(x + sqrt(x*x - LITDBL(1.0)));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(acoshFunc, acosh)

#ifndef HAVE_ASINH
static sqlite_double asinh(sqlite_double x){
  return log(x + sqrt(x*x + LITDBL(1.0)));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(asinhFunc, asinh)

#ifndef HAVE_ATANH
static sqlite_double atanh(sqlite_double x){
  return (LITDBL(1.0)/LITDBL(2.0))*log((1+x)/(1-x)) ;
}
#endif

GEN_MATH_WRAP_DOUBLE_1(atanhFunc, atanh)

/*
** math.h doesn't require cot (cotangent) so it's defined here
*/
static sqlite_double cot(sqlite_double x){
  //return LITDBL(1.0)/tan(x);
  return 1.0/tan(x);
}

GEN_MATH_WRAP_DOUBLE_1(sinFunc, sin)
GEN_MATH_WRAP_DOUBLE_1(cosFunc, cos)
GEN_MATH_WRAP_DOUBLE_1(tanFunc, tan)
GEN_MATH_WRAP_DOUBLE_1(cotFunc, cot)

static sqlite_double coth(sqlite_double x){
  return 1.0/tanh(x);
}

/*
** Many systems don't have hyperbolic trigonometric functions so this will emulate
** them on those systems directly from the definition in terms of exp
*/
#ifndef HAVE_SINH
static sqlite_double sinh(sqlite_double x){
  return (exp(x)-exp(-x))/LITDBL(2.0);
}
#endif

GEN_MATH_WRAP_DOUBLE_1(sinhFunc, sinh)

#ifndef HAVE_COSH
static sqlite_double cosh(sqlite_double x){
  return (exp(x)+exp(-x))/LITDBL(2.0);
}
#endif

GEN_MATH_WRAP_DOUBLE_1(coshFunc, cosh)

#ifndef HAVE_TANH
static sqlite_double tanh(sqlite_double x){
  return sinh(x)/cosh(x);
}
#endif

GEN_MATH_WRAP_DOUBLE_1(tanhFunc, tanh)

GEN_MATH_WRAP_DOUBLE_1(cothFunc, coth)

/*
** Some systems lack log in base 10. This will emulate it
*/

#ifndef HAVE_LOG10
static sqlite_double log10(sqlite_double x){
  static sqlite_double l10 = LITDBL(-1.0);
  if( l10<LITDBL(0.0) ){
    l10 = log(LITDBL(10.0));
  }
  return log(x)/l10;
}
#endif

GEN_MATH_WRAP_DOUBLE_1(logFunc, log)
GEN_MATH_WRAP_DOUBLE_1(log10Func, log10)
GEN_MATH_WRAP_DOUBLE_1(expFunc, exp)

/*
** Fallback for systems where math.h doesn't define M_PI
*/
#undef M_PI
#ifndef M_PI
/*
** static sqlite_double PI = acos(-1.0);
** #define M_PI (PI)
*/
#define M_PI LITDBL(3.14159265358979323846)
#endif

/* Convert Degrees into Radians */
static sqlite_double deg2rad(sqlite_double x){
  return x*M_PI/LITDBL(180.0);
}

/* Convert Radians into Degrees */
static sqlite_double rad2deg(sqlite_double x){
  return LITDBL(180.0)*x/M_PI;
}

GEN_MATH_WRAP_DOUBLE_1(rad2degFunc, rad2deg)
GEN_MATH_WRAP_DOUBLE_1(deg2radFunc, deg2rad)

/* constant function that returns the value of PI=3.1415... */
static void piFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite3_result_double(context, M_PI);
}

/*
** Implements the sqrt function, it has the peculiarity of returning an integer when the
** the argument is an integer.
** Since SQLite isn't strongly typed (almost untyped actually) this is a bit pedantic
*/
static void squareFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  i64 iVal = 0;
  sqlite_double rVal = LITDBL(0.0);
  assert( argc==1 );
  switch( sqlite3_value_type(argv[0]) ){
    case SQLITE_INTEGER: {
      iVal = sqlite3_value_int64(argv[0]);
      sqlite3_result_int64(context, iVal*iVal);
      break;
    }
    case SQLITE_NULL: {
      sqlite3_result_null(context);
      break;
    }
    default: {
      rVal = sqlite3_value_double(argv[0]);
      sqlite3_result_double(context, rVal*rVal);
      break;
    }
  }
}

/*
** Wraps the pow math.h function
** When both the base and the exponent are integers the result should be integer
** (see sqrt just before this). Here the result is always sqlite_double
*/
/* LMH 2007-03-25 Changed to use errno; no pre-checking for errors.  Also removes
  but that was present in the pre-checking that called sqlite3_result_error on
  a non-positive first argument, which is not always an error. */
static void powerFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite_double r1 = LITDBL(0.0);
  sqlite_double r2 = LITDBL(0.0);
  sqlite_double val;

  assert( argc==2 );

  if( sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL ){
    sqlite3_result_null(context);
  }else{
    r1 = sqlite3_value_double(argv[0]);
    r2 = sqlite3_value_double(argv[1]);
    CLEAR_ERRNO(0);
    val = pow(r1,r2);
    if (errno == 0) {
      sqlite3_result_double(context, val);
    } else {
      sqlite3_result_error(context, strerror(errno), errno);
    }
  }
}

/*
** atan2 wrapper
*/
static void atn2Func(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite_double r1 = LITDBL(0.0);
  sqlite_double r2 = LITDBL(0.0);

  assert( argc==2 );

  if( sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL ){
    sqlite3_result_null(context);
  }else{
    r1 = sqlite3_value_double(argv[0]);
    r2 = sqlite3_value_double(argv[1]);
    sqlite3_result_double(context, atan2(r1,r2));
  }
}

/*
** Implementation of the sign() function
** return one of 3 possibilities +1,0 or -1 when the argument is respectively
** positive, 0 or negative.
** When the argument is NULL the result is also NULL (completly conventional)
*/
static void signFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite_double rVal=LITDBL(0.0);
  i64 iVal=0;
  assert( argc==1 );
  switch( sqlite3_value_type(argv[0]) ){
    case SQLITE_INTEGER: {
      iVal = sqlite3_value_int64(argv[0]);
      iVal = ( iVal > 0) ? 1: ( iVal < 0 ) ? -1: 0;
      sqlite3_result_int64(context, iVal);
      break;
    }
    case SQLITE_NULL: {
      sqlite3_result_null(context);
      break;
    }
    default: {
 /* 2nd change below. Line for abs was: if( rVal<0 ) rVal = rVal * -1.0;  */

      rVal = sqlite3_value_double(argv[0]);
      rVal = ( rVal > 0) ? 1: ( rVal < 0 ) ? -1: 0;
      sqlite3_result_double(context, rVal);
      break;
    }
  }
}


/*
** smallest integer value not less than argument
*/
static void ceilFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite_double rVal=LITDBL(0.0);
  //i64 iVal=0;
  assert( argc==1 );
  switch( sqlite3_value_type(argv[0]) ){
    case SQLITE_INTEGER: {
      i64 iVal = sqlite3_value_int64(argv[0]);
      sqlite3_result_int64(context, iVal);
      break;
    }
    case SQLITE_NULL: {
      sqlite3_result_null(context);
      break;
    }
    default: {
      rVal = sqlite3_value_double(argv[0]);
      sqlite3_result_int64(context, (i64) ceil(rVal));
      break;
    }
  }
}

/*
** largest integer value not greater than argument
*/
static void floorFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite_double rVal=LITDBL(0.0);
  //i64 iVal=0;
  assert( argc==1 );
  switch( sqlite3_value_type(argv[0]) ){
    case SQLITE_INTEGER: {
      i64 iVal = sqlite3_value_int64(argv[0]);
      sqlite3_result_int64(context, iVal);
      break;
    }
    case SQLITE_NULL: {
      sqlite3_result_null(context);
      break;
    }
    default: {
      rVal = sqlite3_value_double(argv[0]);
      sqlite3_result_int64(context, (i64) floor(rVal));
      break;
    }
  }
}

/*
** Given a string (s) in the first argument and an integer (n) in the second returns the
** string that constains s contatenated n times
*/
static void replicateFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  unsigned char *z;        /* input string */
  unsigned char *zo;       /* result string */
  i64 iCount;              /* times to repeat */
  i64 nLen;                /* length of the input string (no multibyte considerations) */
  i64 nTLen;               /* length of the result string (no multibyte considerations) */
  i64 i=0;

  if( argc!=2 || SQLITE_NULL==sqlite3_value_type(argv[0]) )
    return;

  iCount = sqlite3_value_int64(argv[1]);

  if( iCount<0 ){
    sqlite3_result_error(context, "domain error", -1);
  }else{

    nLen  = sqlite3_value_bytes(argv[0]);
    nTLen = nLen*iCount;
    z=sqlite3_malloc(nTLen+1);
    zo=sqlite3_malloc(nLen+1);
    if (!z || !zo){
      sqlite3_result_error_nomem(context);
      if (z) sqlite3_free(z);
      if (zo) sqlite3_free(zo);
      return;
    }
    strcpy((char*)zo, (char*)sqlite3_value_text(argv[0]));

    for(i=0; i<iCount; ++i){
      strcpy((char*)(z+i*nLen), (char*)zo);
    }

    sqlite3_result_text(context, (char*)z, -1, SQLITE_TRANSIENT);
    sqlite3_free(z);
    sqlite3_free(zo);
  }
}

/*
** Some systems (win32 among others) don't have an isblank function, this will emulate it.
** This function is not UFT-8 safe since it only analyses a byte character.
*/
#if (!defined(HAVE_ISBLANK)) || (defined(_WIN32_WCE) && !defined(__GNUC__))
int isblank(char c){
  return( ' '==c || '\t'==c );
}
#endif

static void properFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const unsigned char *z;     /* input string */
  unsigned char *zo;          /* output string */
  unsigned char *zt;          /* iterator */
  char r;
  int c=1;

  assert( argc==1);
  if( SQLITE_NULL==sqlite3_value_type(argv[0]) ){
    sqlite3_result_null(context);
    return;
  }

  z = sqlite3_value_text(argv[0]);
  zo = (unsigned char *)sqlite3StrDup((char *) z);
  if (!zo) {
    sqlite3_result_error_nomem(context);
    return;
  }
  zt = zo;

  while( (r = *(z++))!=0 ){
    if( isblank(r) ){
      c=1;
    }else{
      if( c==1 ){
        r = toupper(r);
      }else{
        r = tolower(r);
      }
      c=0;
    }
    *(zt++) = r;
  }
  *zt = '\0';

  sqlite3_result_text(context, (char*)zo, -1, SQLITE_TRANSIENT);
  sqlite3_free(zo);
}

/*
** given an input string (s) and an integer (n) adds spaces at the begining of  s
** until it has a length of n characters.
** When s has a length >=n it's a NOP
** padl(NULL) = NULL
*/
static void padlFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  i64 ilen;          /* length to pad to */
  i64 zl;            /* length of the input string (UTF-8 chars) */
  int i = 0;
  const char *zi;    /* input string */
  char *zo;          /* output string */
  char *zt;

  assert( argc==2 );

  if( sqlite3_value_type(argv[0]) == SQLITE_NULL ){
    sqlite3_result_null(context);
  }else{
    zi = (char *)sqlite3_value_text(argv[0]);
    ilen = sqlite3_value_int64(argv[1]);
    /* check domain */
    if(ilen<0){
      sqlite3_result_error(context, "domain error", -1);
      return;
    }
    zl = sqlite3Utf8CharLen(zi, -1);
    if( zl>=ilen ){
      /* string is longer than the requested pad length, return the same string (dup it) */
      zo = sqlite3StrDup(zi);
      if (!zo){
        sqlite3_result_error_nomem(context);
        return;
      }
      sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    }else{
      zo = sqlite3_malloc(strlen(zi)+ilen-zl+1);
      if (!zo){
        sqlite3_result_error_nomem(context);
        return;
      }
      zt = zo;
      for(i=1; i+zl<=ilen; ++i){
        *(zt++)=' ';
      }
      /* no need to take UTF-8 into consideration here */
      strcpy(zt,zi);
    }
    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** given an input string (s) and an integer (n) appends spaces at the end of  s
** until it has a length of n characters.
** When s has a length >=n it's a NOP
** padl(NULL) = NULL
*/
static void padrFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  i64 ilen;          /* length to pad to */
  i64 zl;            /* length of the input string (UTF-8 chars) */
  i64 zll;           /* length of the input string (bytes) */
  int i = 0;
  const char *zi;    /* input string */
  char *zo;          /* output string */
  char *zt;

  assert( argc==2 );

  if( sqlite3_value_type(argv[0]) == SQLITE_NULL ){
    sqlite3_result_null(context);
  }else{
    zi = (char *)sqlite3_value_text(argv[0]);
    ilen = sqlite3_value_int64(argv[1]);
    /* check domain */
    if(ilen<0){
      sqlite3_result_error(context, "domain error", -1);
      return;
    }
    zl = sqlite3Utf8CharLen(zi, -1);
    if( zl>=ilen ){
      /* string is longer than the requested pad length, return the same string (dup it) */
      zo = (char *)sqlite3StrDup(zi);
      if (!zo){
        sqlite3_result_error_nomem(context);
        return;
      }
      sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    }else{
      zll = strlen(zi);
      zo = sqlite3_malloc(zll+ilen-zl+1);
      if (!zo){
        sqlite3_result_error_nomem(context);
        return;
      }
      zt = strcpy(zo,zi)+zll;
      for(i=1; i+zl<=ilen; ++i){
        *(zt++) = ' ';
      }
      *zt = '\0';
    }
    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** given an input string (s) and an integer (n) appends spaces at the end of  s
** and adds spaces at the begining of s until it has a length of n characters.
** Tries to add has many characters at the left as at the right.
** When s has a length >=n it's a NOP
** padl(NULL) = NULL
*/
static void padcFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  i64 ilen;           /* length to pad to */
  i64 zl;             /* length of the input string (UTF-8 chars) */
  i64 zll;            /* length of the input string (bytes) */
  int i = 0;
  const char *zi;     /* input string */
  char *zo;           /* output string */
  char *zt;

  assert( argc==2 );

  if( sqlite3_value_type(argv[0]) == SQLITE_NULL ){
    sqlite3_result_null(context);
  }else{
    zi = (char *)sqlite3_value_text(argv[0]);
    ilen = sqlite3_value_int64(argv[1]);
    /* check domain */
    if(ilen<0){
      sqlite3_result_error(context, "domain error", -1);
      return;
    }
    zl = sqlite3Utf8CharLen(zi, -1);
    if( zl>=ilen ){
      /* string is longer than the requested pad length, return the same string (dup it) */
      zo = (char *)sqlite3StrDup(zi);
      if (!zo){
        sqlite3_result_error_nomem(context);
        return;
      }
      sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    }else{
      zll = strlen(zi);
      zo = sqlite3_malloc(zll+ilen-zl+1);
      if (!zo){
        sqlite3_result_error_nomem(context);
        return;
      }
      zt = zo;
      for(i=1; 2*i+zl<=ilen; ++i){
        *(zt++) = ' ';
      }
      strcpy(zt, zi);
      zt+=zll;
      for(; i+zl<=ilen; ++i){
        *(zt++) = ' ';
      }
      *zt = '\0';
    }
    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** given 2 string (s1,s2) returns the string s1 with the characters NOT in s2 removed
** assumes strings are UTF-8 encoded
*/
static void strfilterFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *zi1;        /* first parameter string (searched string) */
  const char *zi2;        /* second parameter string (vcontains valid characters) */
  const char *z1;
  const char *z21;
  const char *z22;
  char *zo;               /* output string */
  char *zot;
  int c1 = 0;
  int c2 = 0;

  assert( argc==2 );

  if( sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL ){
    sqlite3_result_null(context);
  }else{
    zi1 = (char *)sqlite3_value_text(argv[0]);
    zi2 = (char *)sqlite3_value_text(argv[1]);
    /*
    ** maybe I could allocate less, but that would imply 2 passes, rather waste
    ** (possibly) some memory
    */
    zo = sqlite3_malloc(strlen(zi1)+1);
    if (!zo){
      sqlite3_result_error_nomem(context);
      return;
    }
    zot = zo;
    z1 = zi1;
    while( (c1=sqliteCharVal((unsigned char *)z1))!=0 ){
      z21=zi2;
      while( (c2=sqliteCharVal((unsigned char *)z21))!=0 && c2!=c1 ){
        sqliteNextChar(z21);
      }
      if( c2!=0){
        z22=z21;
        sqliteNextChar(z22);
        strncpy(zot, z21, z22-z21);
        zot+=z22-z21;
      }
      sqliteNextChar(z1);
    }
    *zot = '\0';

    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** Given a string z1, retutns the (0 based) index of it's first occurence
** in z2 after the first s characters.
** Returns -1 when there isn't a match.
** updates p to point to the character where the match occured.
** This is an auxiliary function.
*/
static int _substr(const char* z1, const char* z2, int s, const char** p){
  int c = 0;
  int rVal=-1;
  const char* zt1;
  const char* zt2;
  int c1,c2;

  if( '\0'==*z1 ){
    return -1;
  }

  while( (sqliteCharVal((unsigned char *)z2) != 0) && (c++)<s){
    sqliteNextChar(z2);
  }

  c = 0;
  while( (sqliteCharVal((unsigned char *)z2)) != 0 ){
    zt1 = z1;
    zt2 = z2;

    do{
      c1 = sqliteCharVal((unsigned char *)zt1);
      c2 = sqliteCharVal((unsigned char *)zt2);
      sqliteNextChar(zt1);
      sqliteNextChar(zt2);
    }while( c1 == c2 && c1 != 0 && c2 != 0 );

    if( c1 == 0 ){
      rVal = c;
      break;
    }

    sqliteNextChar(z2);
    ++c;
  }
  if(p){
    *p=z2;
  }
  return rVal >=0 ? rVal+s : rVal;
}

/*
** given 2 input strings (s1,s2) and an integer (n) searches from the nth character
** for the string s1. Returns the position where the match occured.
** Characters are counted from 1.
** 0 is returned when no match occurs.
*/

static void charindexFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const u8 *z1;          /* s1 string */
  u8 *z2;                /* s2 string */
  int s=0;
  int rVal=0;

  assert( argc==3 ||argc==2);

  if( SQLITE_NULL==sqlite3_value_type(argv[0]) || SQLITE_NULL==sqlite3_value_type(argv[1])){
    sqlite3_result_null(context);
    return;
  }

  z1 = sqlite3_value_text(argv[0]);
  if( z1==0 ) return;
  z2 = (u8*) sqlite3_value_text(argv[1]);
  if(argc==3){
    s = sqlite3_value_int(argv[2])-1;
    if(s<0){
      s=0;
    }
  }else{
    s = 0;
  }

  rVal = _substr((char *)z1,(char *)z2,s,NULL);
  sqlite3_result_int(context, rVal+1);
}

/*
** given a string (s) and an integer (n) returns the n leftmost (UTF-8) characters
** if the string has a length<=n or is NULL this function is NOP
*/
static void leftFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  int c=0;
  int cc=0;
  int l=0;
  const unsigned char *z;       /* input string */
  const unsigned char *zt;
  unsigned char *rz;            /* output string */

  assert( argc==2);

  if( SQLITE_NULL==sqlite3_value_type(argv[0]) || SQLITE_NULL==sqlite3_value_type(argv[1])){
    sqlite3_result_null(context);
    return;
  }

  z  = sqlite3_value_text(argv[0]);
  l  = sqlite3_value_int(argv[1]);
  zt = z;

  while( sqliteCharVal(zt) && c++<l)
    sqliteNextChar(zt);

  cc=zt-z;

  rz = sqlite3_malloc(zt-z+1);
  if (!rz){
    sqlite3_result_error_nomem(context);
    return;
  }
  strncpy((char*) rz, (char*) z, zt-z);
  *(rz+cc) = '\0';
  sqlite3_result_text(context, (char*)rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

/*
** given a string (s) and an integer (n) returns the n rightmost (UTF-8) characters
** if the string has a length<=n or is NULL this function is NOP
*/
static void rightFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  int l=0;
  int c=0;
  int cc=0;
  const char *z;
  const char *zt;
  const char *ze;
  char *rz;

  assert( argc==2);

  if( SQLITE_NULL == sqlite3_value_type(argv[0]) || SQLITE_NULL == sqlite3_value_type(argv[1])){
    sqlite3_result_null(context);
    return;
  }

  z  = (char *)sqlite3_value_text(argv[0]);
  l  = sqlite3_value_int(argv[1]);
  zt = z;

  while( sqliteCharVal((unsigned char *)zt)!=0){
    sqliteNextChar(zt);
    ++c;
  }

  ze = zt;
  zt = z;

  cc=c-l;
  if(cc<0)
    cc=0;

  while( cc-- > 0 ){
    sqliteNextChar(zt);
  }

  rz = sqlite3_malloc(ze-zt+1);
  if (!rz){
    sqlite3_result_error_nomem(context);
    return;
  }
  strcpy((char*) rz, (char*) (zt));
  sqlite3_result_text(context, (char*)rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

#ifndef HAVE_TRIM
/*
** removes the whitespaces at the begining of a string.
*/
const char* ltrim(const char* s){
  while( *s==' ' )
    ++s;
  return s;
}

/*
** removes the whitespaces at the end of a string.
** !mutates the input string!
*/
void rtrim(char* s){
  char* ss = s+strlen(s)-1;
  while( ss>=s && *ss==' ' )
    --ss;
  *(ss+1)='\0';
}

/*
**  Removes the whitespace at the begining of a string
*/
static void ltrimFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z;

  assert( argc==1);

  if( SQLITE_NULL==sqlite3_value_type(argv[0]) ){
    sqlite3_result_null(context);
    return;
  }
  z = sqlite3_value_text(argv[0]);
  sqlite3_result_text(context, ltrim(z), -1, SQLITE_TRANSIENT);
}

/*
**  Removes the whitespace at the end of a string
*/
static void rtrimFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z;
  char *rz;
  /* try not to change data in argv */

  assert( argc==1);

  if( SQLITE_NULL==sqlite3_value_type(argv[0]) ){
    sqlite3_result_null(context);
    return;
  }
  z = sqlite3_value_text(argv[0]);
  rz = sqlite3StrDup(z);
  rtrim(rz);
  sqlite3_result_text(context, rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

/*
**  Removes the whitespace at the begining and end of a string
*/
static void trimFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z;
  char *rz;
  /* try not to change data in argv */

  assert( argc==1);

  if( SQLITE_NULL==sqlite3_value_type(argv[0]) ){
    sqlite3_result_null(context);
    return;
  }
  z = sqlite3_value_text(argv[0]);
  rz = sqlite3StrDup(z);
  rtrim(rz);
  sqlite3_result_text(context, ltrim(rz), -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}
#endif

/*
** given a pointer to a string s1, the length of that string (l1), a new string (s2)
** and it's length (l2) appends s2 to s1.
** All lengths in bytes.
** This is just an auxiliary function
*/
// static void _append(char **s1, int l1, const char *s2, int l2){
//   *s1 = realloc(*s1, (l1+l2+1)*sizeof(char));
//   strncpy((*s1)+l1, s2, l2);
//   *(*(s1)+l1+l2) = '\0';
// }

#ifndef HAVE_TRIM

/*
** given strings s, s1 and s2 replaces occurrences of s1 in s by s2
*/
static void replaceFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z1;     /* string s (first parameter) */
  const char *z2;     /* string s1 (second parameter) string to look for */
  const char *z3;     /* string s2 (third parameter) string to replace occurrences of s1 with */
  int lz1;
  int lz2;
  int lz3;
  int lzo=0;
  char *zo=0;
  int ret=0;
  const char *zt1;
  const char *zt2;

  assert( 3==argc );

  if( SQLITE_NULL==sqlite3_value_type(argv[0]) ){
    sqlite3_result_null(context);
    return;
  }

  z1 = sqlite3_value_text(argv[0]);
  z2 = sqlite3_value_text(argv[1]);
  z3 = sqlite3_value_text(argv[2]);
  /* handle possible null values */
  if( 0==z2 ){
    z2="";
  }
  if( 0==z3 ){
    z3="";
  }

  lz1 = strlen(z1);
  lz2 = strlen(z2);
  lz3 = strlen(z3);

#if 0
  /* special case when z2 is empty (or null) nothing will be changed */
  if( 0==lz2 ){
    sqlite3_result_text(context, z1, -1, SQLITE_TRANSIENT);
    return;
  }
#endif

  zt1=z1;
  zt2=z1;

  while(1){
    ret=_substr(z2,zt1 , 0, &zt2);

    if( ret<0 )
      break;

    _append(&zo, lzo, zt1, zt2-zt1);
    lzo+=zt2-zt1;
    _append(&zo, lzo, z3, lz3);
    lzo+=lz3;

    zt1=zt2+lz2;
  }
  _append(&zo, lzo, zt1, lz1-(zt1-z1));
  sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
  sqlite3_free(zo);
}
#endif

/*
** given a string returns the same string but with the characters in reverse order
*/
static void reverseFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z;
  const char *zt;
  char *rz;
  char *rzt;
  int l = 0;
  int i = 0;

  assert( 1==argc );

  if( SQLITE_NULL==sqlite3_value_type(argv[0]) ){
    sqlite3_result_null(context);
    return;
  }
  z = (char *)sqlite3_value_text(argv[0]);
  l = strlen(z);
  rz = sqlite3_malloc(l+1);
  if (!rz){
    sqlite3_result_error_nomem(context);
    return;
  }
  rzt = rz+l;
  *(rzt--) = '\0';

  zt=z;
  while( sqliteCharVal((unsigned char *)zt)!=0 ){
    z=zt;
    sqliteNextChar(zt);
    for(i=1; zt-i>=z; ++i){
      *(rzt--)=*(zt-i);
    }
  }

  sqlite3_result_text(context, rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

/*
** An instance of the following structure holds the context of a
** stdev() or variance() aggregate computation.
** implementaion of http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Algorithm_II
** less prone to rounding errors
*/
typedef struct StdevCtx StdevCtx;
struct StdevCtx {
  sqlite_double rM;
  sqlite_double rS;
  i64 cnt;          /* number of elements */
};

/*
** An instance of the following structure holds the context of a
** mode() or median() aggregate computation.
** Depends on structures defined in map.c (see map & map)
** These aggregate functions only work for integers and floats although
** they could be made to work for strings. This is usually considered meaningless.
** Only usuall order (for median), no use of collation functions (would this even make sense?)
*/
typedef struct ModeCtx ModeCtx;
struct ModeCtx {
  i64 riM;            /* integer value found so far */
  sqlite_double rdM;         /* sqlite_double value found so far */
  i64 cnt;            /* number of elements so far */
  sqlite_double pcnt;        /* number of elements smaller than a percentile */
  i64 mcnt;           /* maximum number of occurrences (for mode) */
  i64 mn;             /* number of occurrences (for mode and percentiles) */
  i64 is_double;      /* whether the computation is being done for doubles (>0) or integers (=0) */
  map* m;             /* map structure used for the computation */
  int done;           /* whether the answer has been found */
};

/*
** called for each value received during a calculation of stdev or variance
*/
static void varianceStep(sqlite3_context *context, int argc, sqlite3_value **argv){
  StdevCtx *p;

  sqlite_double delta;
  sqlite_double x;

  assert( argc==1 );
  p = sqlite3_aggregate_context(context, sizeof(*p));
  /* only consider non-null values */
  if( SQLITE_NULL != sqlite3_value_numeric_type(argv[0]) ){
    p->cnt++;
    x = sqlite3_value_double(argv[0]);
    delta = (x-p->rM);
    p->rM += delta/p->cnt;
    p->rS += delta*(x-p->rM);
  }
}

/*
** called for each value received during a calculation of mode of median
*/
static void modeStep(sqlite3_context *context, int argc, sqlite3_value **argv){
  ModeCtx *p;
  i64 xi=0;
  sqlite_double xd=LITDBL(0.0);
  i64 *iptr;
  sqlite_double *dptr;
  int type;

  assert( argc==1 );
  type = sqlite3_value_numeric_type(argv[0]);

  if( type == SQLITE_NULL)
    return;

  p = sqlite3_aggregate_context(context, sizeof(*p));

  if( 0==(p->m) ){
    p->m = calloc(1, sizeof(map));
    if( type==SQLITE_INTEGER ){
      /* map will be used for integers */
      *(p->m) = map_make(int_cmp);
      p->is_double = 0;
    }else{
      p->is_double = 1;
      /* map will be used for doubles */
      *(p->m) = map_make(sqlite_double_cmp);
    }
  }

  ++(p->cnt);

  if( 0==p->is_double ){
    xi = sqlite3_value_int64(argv[0]);
    iptr = (i64*)calloc(1,sizeof(i64));
    *iptr = xi;
    map_insert(p->m, iptr);
  }else{
    xd = sqlite3_value_double(argv[0]);
    dptr = (sqlite_double*)calloc(1,sizeof(sqlite_double));
    *dptr = xd;
    map_insert(p->m, dptr);
  }
}

/*
**  Auxiliary function that iterates all elements in a map and finds the mode
**  (most frequent value)
*/
static void modeIterate(void* e, i64 c, void* pp){
  i64 ei;
  sqlite_double ed;
  ModeCtx *p = (ModeCtx*)pp;

  if( 0==p->is_double ){
    ei = *(int*)(e);

	if( p->mcnt==c ){
      ++p->mn;
    }else if( p->mcnt<c ){
      p->riM = ei;
      p->mcnt = c;
	  p->mn=1;
    }
  }else{
    ed = *(sqlite_double*)(e);

	if( p->mcnt==c ){
      ++p->mn;
    }else if(p->mcnt<c){
      p->rdM = ed;
      p->mcnt = c;
	  p->mn=1;
    }
  }
}

/*
**  Auxiliary function that iterates all elements in a map and finds the median
**  (the value such that the number of elements smaller is equal the the number of
**  elements larger)
*/
static void medianIterate(void* e, i64 c, void* pp){
  i64 ei;
  sqlite_double ed;
  sqlite_double iL;
  sqlite_double iR;
  int il;
  int ir;
  ModeCtx *p = (ModeCtx*)pp;

  if(p->done>0)
    return;

  iL = p->pcnt;
  iR = p->cnt - p->pcnt;
  il = p->mcnt + c;
  ir = p->cnt - p->mcnt;

  if( il >= iL ){
    if( ir >= iR ){
    ++p->mn;
      if( 0==p->is_double ){
        ei = *(int*)(e);
        p->riM += ei;
      }else{
        ed = *(sqlite_double*)(e);
        p->rdM += ed;
      }
    }else{
      p->done=1;
    }
  }
  p->mcnt+=c;
}

/*
** Returns the mode value
*/
static void modeFinalize(sqlite3_context *context){
  ModeCtx *p;
  p = sqlite3_aggregate_context(context, 0);
  if( p && p->m ){
    map_iterate(p->m, modeIterate, p);
    map_destroy(p->m);
    free(p->m);

    if( 1==p->mn ){
      if( 0==p->is_double )
        sqlite3_result_int64(context, p->riM);
      else
        sqlite3_result_double(context, p->rdM);
    }
  }
}

/*
** auxiliary function for percentiles
*/
static void _medianFinalize(sqlite3_context *context){
  ModeCtx *p;
  p = (ModeCtx*) sqlite3_aggregate_context(context, 0);
  if( p && p->m ){
    p->done=0;
    map_iterate(p->m, medianIterate, p);
    map_destroy(p->m);
    free(p->m);

    if( 0==p->is_double )
      if( 1==p->mn )
      	sqlite3_result_int64(context, p->riM);
      else
      	sqlite3_result_double(context, p->riM*1.0/p->mn);
    else
      sqlite3_result_double(context, p->rdM/p->mn);
  }
}

/*
** Returns the median value
*/
static void medianFinalize(sqlite3_context *context){
  ModeCtx *p;
  p = (ModeCtx*) sqlite3_aggregate_context(context, 0);
  if( p!=0 ){
    p->pcnt = (p->cnt)/2.0;
    _medianFinalize(context);
  }
}

/*
** Returns the lower_quartile value
*/
static void lower_quartileFinalize(sqlite3_context *context){
  ModeCtx *p;
  p = (ModeCtx*) sqlite3_aggregate_context(context, 0);
  if( p!=0 ){
    p->pcnt = (p->cnt)/4.0;
    _medianFinalize(context);
  }
}

/*
** Returns the upper_quartile value
*/
static void upper_quartileFinalize(sqlite3_context *context){
  ModeCtx *p;
  p = (ModeCtx*) sqlite3_aggregate_context(context, 0);
  if( p!=0 ){
    p->pcnt = (p->cnt)*3/4.0;
    _medianFinalize(context);
  }
}

/*
** Returns the sample stdev value
** See http://en.wikipedia.org/wiki/Stdev
*/
static void samplestdevFinalize(sqlite3_context *context){
  StdevCtx *p;
  p = sqlite3_aggregate_context(context, 0);
  if( p && p->cnt>1 ){
    sqlite3_result_double(context, sqrt(p->rS/(p->cnt-1)));
  }else{
    sqlite3_result_double(context, 0.0);
  }
}

/*
** Returns the stdev value
*/
static void stdevFinalize(sqlite3_context *context){
  StdevCtx *p;
  p = sqlite3_aggregate_context(context, 0);
  if( p && p->cnt>1 ){
    sqlite3_result_double(context, sqrt(p->rS/(p->cnt-1)));
  }else{
    sqlite3_result_double(context, 0.0);
  }
}

/*
** Returns the variance value
*/
static void varianceFinalize(sqlite3_context *context){
  StdevCtx *p;
  p = sqlite3_aggregate_context(context, 0);
  if( p && p->cnt>1 ){
    sqlite3_result_double(context, p->rS/(p->cnt-1));
  }else{
    sqlite3_result_double(context, 0.0);
  }
}

#ifdef SQLITE_SOUNDEX

/* relicoder factored code */
/*
** Calculates the soundex value of a string
*/

static void soundex(const u8 *zIn, char *zResult){
  int i, j;
  static const unsigned char iCode[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 2, 4, 5, 5, 0,
    1, 2, 6, 2, 3, 0, 1, 0, 2, 0, 2, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 2, 4, 5, 5, 0,
    1, 2, 6, 2, 3, 0, 1, 0, 2, 0, 2, 0, 0, 0, 0, 0,
  };

  for(i=0; zIn[i] && !isalpha(zIn[i]); i++){}
  if( zIn[i] ){
    zResult[0] = toupper(zIn[i]);
    for(j=1; j<4 && zIn[i]; i++){
      int code = iCode[zIn[i]&0x7f];
      if( code>0 ){
        zResult[j++] = code + '0';
      }
    }
    while( j<4 ){
      zResult[j++] = '0';
    }
    zResult[j] = 0;
  }else{
    strcpy(zResult, "?000");
  }
}

/*
** computes the number of different characters between the soundex value fo 2 strings
*/
static void differenceFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  char zResult1[8];
  char zResult2[8];
  char *zR1 = zResult1;
  char *zR2 = zResult2;
  int rVal = 0;
  int i = 0;
  const u8 *zIn1;
  const u8 *zIn2;

  assert( argc==2 );

  if( sqlite3_value_type(argv[0])==SQLITE_NULL || sqlite3_value_type(argv[1])==SQLITE_NULL ){
    sqlite3_result_null(context);
    return;
  }

  zIn1 = (u8*)sqlite3_value_text(argv[0]);
  zIn2 = (u8*)sqlite3_value_text(argv[1]);

  soundex(zIn1, zR1);
  soundex(zIn2, zR2);

  for(i=0; i<4; ++i){
    if( sqliteCharVal((unsigned char *)zR1)==sqliteCharVal((unsigned char *)zR2) )
      ++rVal;
    sqliteNextChar(zR1);
    sqliteNextChar(zR2);
  }
  sqlite3_result_int(context, rVal);
}
#endif

/*
** fmod wrapper
*/
static void fmodFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite_double r1 = LITDBL(0.0);
  sqlite_double r2 = LITDBL(0.0);

  assert( argc==2 );

  if( sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL ){
    sqlite3_result_null(context);
  }else{
    r1 = sqlite3_value_double(argv[0]);
    r2 = sqlite3_value_double(argv[1]);
    sqlite3_result_double(context, fmod(r1,r2));
  }
}

static void	sqlitex_distance(sqlite3_context * context, int argc, sqlite3_value **argv)
{
	// expect x1, y1, x2, y2 all as sqlite_doubles
	sqlite_double	x1;
	sqlite_double	y1;
	sqlite_double	x2;
	sqlite_double	y2;
	register sqlite_double	xd;
	register sqlite_double	yd;
	int		i;

	if (argc < 4)
	{
		sqlite3_result_error(context, "distance requires 4 parameters", -1);
		return;
	}
	for (i = 0; i < 4; i++)
	{
		if (argv[i] == NULL)
		{
			sqlite3_result_error(context, "distance requires 4 non-null numeric parameters", -1);
			return;
		}
	}

	x1 = sqlite3_value_double(argv[0]);
	y1 = sqlite3_value_double(argv[1]);
	x2 = sqlite3_value_double(argv[2]);
	y2 = sqlite3_value_double(argv[3]);
	xd = x1 - x2;
	yd = y1 - y2;
	sqlite3_result_double(context, sqrt(xd * xd + yd * yd));
}

static void	sqlitex_bearing(sqlite3_context * context, int argc, sqlite3_value **argv)
{
	// expect x1, y1, x2, y2 all as sqlite_doubles
	sqlite_double	x1;
	sqlite_double	y1;
	sqlite_double	x2;
	sqlite_double	y2;
	sqlite_double	xd;
	sqlite_double	yd;
	sqlite_double bearing;
	int		i;

	if (argc < 4)
	{
		sqlite3_result_error(context, "bearing requires 4 parameters", -1);
		return;
	}
	for (i = 0; i < 4; i++)
	{
		if (argv[i] == NULL)
		{
			sqlite3_result_error(context, "bearing requires 4 non-null numeric parameters", -1);
			return;
		}
	}

	x1 = sqlite3_value_double(argv[0]);
	y1 = sqlite3_value_double(argv[1]);
	x2 = sqlite3_value_double(argv[2]);
	y2 = sqlite3_value_double(argv[3]);
	if( x1 == x2 && y1 == y2 ) {
		sqlite3_result_null(context);
		return;
	}
	xd = x2 - x1;
	yd = y2 - y1;
	//bearing = fmod(atan2(xd, yd) * (LITDBL(180.0)/M_PI), LITDBL(360.0));
	bearing = fmod(TO_SQLITE_DOUBLE(atan2(xd, yd)) * (LITDBL(180.0)/M_PI), LITDBL(360.0));
	sqlite3_result_double(context, bearing < 0 ? bearing + LITDBL(360.0) : bearing);
}

static void	sqlitex_is_inside_circle(sqlite3_context * context, int argc, sqlite3_value **argv)
{
	// expect x1, y1, x2, y2 all as sqlite_doubles
	sqlite_double	x1;
	sqlite_double	y1;
	sqlite_double	r;
	sqlite_double	x2;
	sqlite_double	y2;
	register sqlite_double	xd;
	register sqlite_double	yd;
	int		i, result = 0;

	if (argc < 5)
	{
		sqlite3_result_error(context, "is_inside_circle requires 5 parameters", -1);
		return;
	}
	for (i = 0; i < 5; i++)
	{
		if (argv[i] == NULL)
		{
			sqlite3_result_error(context, "is_inside_circle requires 5 non-null numeric parameters", -1);
			return;
		}
	}

	x1 = sqlite3_value_double(argv[0]);
	y1 = sqlite3_value_double(argv[1]);
	r = sqlite3_value_double(argv[2]);
	x2 = sqlite3_value_double(argv[3]);
	y2 = sqlite3_value_double(argv[4]);
	xd = fabs(x1 - x2);
	yd = fabs(y1 - y2);

	if(xd + yd <= r) result = 1;
	else if( (xd > r)  || ( yd > r ) ) result = 0;
	else if( (xd*xd) + (yd*yd) <= (r*r) ) result = 1;

	sqlite3_result_int(context, result);
}

static void	geo_distance(sqlite3_context * context, int argc, sqlite3_value **argv)
{
  sqlite_double theta, dist, lat1, lon1, lat2, lon2;
  char *unit;
  int i;

  assert( argc==5 );
  for (i = 0; i < 4; i++) {
	if (argv[i] == NULL) {
       sqlite3_result_null(context);
	   return;
	}
  }

  lat1 = deg2rad(sqlite3_value_double(argv[0]));
  lon1 = sqlite3_value_double(argv[1]);
  lat2 = deg2rad(sqlite3_value_double(argv[2]));
  lon2 = sqlite3_value_double(argv[3]);
  unit = (char *)sqlite3_value_text(argv[4]);

  theta = deg2rad(lon1 - lon2);
  dist = sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(theta);
  dist = rad2deg(acos(dist)) * 60 * LITDBL(1.1515);
  switch(unit[0]) {
    //case 'M':
    //  break;
    case 'K':
      dist = dist * LITDBL(1.609344);
      break;
    case 'N':
      dist = dist * LITDBL(0.8684);
      break;
  }
  sqlite3_result_double(context, dist);
}

static const char *GEOHASHBASE32 = "0123456789bcdefghjkmnpqrstuvwxyz";
static const char GEOHASHBITS[] = {16,8,4,2,1};

enum geoDir {geoRight, geoLeft, geoTop, geoBottom};

void decode_geohash_bbox(char *geohash, sqlite_double *lat, sqlite_double *lon) {
	int i, j, hashlen;
	char c, cd, mask, is_even=1;

	lat[0] = LITDBL(-90.0);  lat[1] = LITDBL(90.0);
	lon[0] = LITDBL(-180.0); lon[1] = LITDBL(180.0);
	//*lat_err = LITDBL(90.0);  *lon_err = LITDBL(180.0);
	hashlen = strlen(geohash);

	for (i=0; i<hashlen; i++) {
		c = tolower(geohash[i]);
		cd = strchr(GEOHASHBASE32, c)-GEOHASHBASE32;
		for (j=0; j<5; j++) {
			mask = GEOHASHBITS[j];
			if (is_even) {
				//*lon_err /= 2;
				lon[!(cd&mask)] = (lon[0] + lon[1])/2;
			} else {
				//*lat_err /= 2;
				lat[!(cd&mask)] = (lat[0] + lat[1])/2;
			}
			is_even = !is_even;
		}
	}
}

void decode_geohash(char *geohash, sqlite_double *point) {
	sqlite_double lat[2], lon[2];

	decode_geohash_bbox(geohash, lat, lon);

	point[0] = (lat[0] + lat[1]) / 2;
	point[1] = (lon[0] + lon[1]) / 2;
}

static void	encode_geohash(sqlite3_context * context, int argc, sqlite3_value **argv)
{
	int is_even=1, i=0;
	sqlite_double lat[2], lon[2], mid, latitude, longitude;
	int bit=0, ch=0;
	int precision;
	char *geohash;

	assert( argc==3 );

    latitude = sqlite3_value_double(argv[0]);
    longitude = sqlite3_value_double(argv[1]);
    precision = sqlite3_value_int(argv[2]);
	geohash=sqlite3_malloc(precision+1);

	lat[0] = LITDBL(-90.0);  lat[1] = LITDBL(90.0);
	lon[0] = LITDBL(-180.0); lon[1] = LITDBL(180.0);

	while (i < precision) {
		if (is_even) {
			mid = (lon[0] + lon[1]) / 2;
			if (longitude > mid) {
				ch |= GEOHASHBITS[bit];
				lon[0] = mid;
			} else
				lon[1] = mid;
		} else {
			mid = (lat[0] + lat[1]) / 2;
			if (latitude > mid) {
				ch |= GEOHASHBITS[bit];
				lat[0] = mid;
			} else
				lat[1] = mid;
		}

		is_even = !is_even;
		if (bit < 4)
			bit++;
		else {
			geohash[i++] = GEOHASHBASE32[ch];
			bit = 0;
			ch = 0;
		}
	}
	geohash[i] = '\0';
	sqlite3_result_text(context, geohash, -1, SQLITE_TRANSIENT);
	sqlite3_free(geohash);
}

// Given a particular geohash string, a direction, and a final length
// Compute a neighbor using base32 lookups, recursively when necessary
void get_neighbor(char *str, int dir, int hashlen)
{
	/* Right, Left, Top, Bottom */

	static char *neighbors[] = {
           "bc01fg45238967deuvhjyznpkmstqrwx",
		   "238967debc01fg45kmstqrwxuvhjyznp",
		   "p0r21436x8zb9dcf5h7kjnmqesgutwvy",
		   "14365h7k9dcfesgujnmqp0r2twvyx8zb"
           };

	static char *borders[] = { "bcfguvyz", "0145hjnp", "prxz", "028b" };

	char last_chr, *border, *neighbor;
	int index = ( 2 * (hashlen % 2) + dir) % 4;
	neighbor = neighbors[index];
	border = borders[index];
	last_chr = str[hashlen-1];
	if (strchr(border,last_chr))
		get_neighbor(str, dir, hashlen-1);
	str[hashlen-1] = GEOHASHBASE32[strchr(neighbor, last_chr)-neighbor];
}

// Given a particular geohash string, a direction, and a final length
// Compute a neighbor using base32 lookups, recursively when necessary
static void	get_geohash_neighbor(sqlite3_context * context, int argc, sqlite3_value **argv)
{
    int dir, hashlen;
    char *geohash;

	assert( argc==3 );

    dir = sqlite3_value_int(argv[2]);
    if( (dir < 0) || (dir > 3)) {
        sqlite3_result_null(context);
        return;
    }
    geohash = (char *)sqlite3StrDup((char *)sqlite3_value_text(argv[0]));
    hashlen = sqlite3_value_int(argv[1]);
    if(hashlen > strlen(geohash)) hashlen = strlen(geohash);

	get_neighbor(geohash, dir, hashlen);

	sqlite3_result_text(context, geohash, -1, SQLITE_TRANSIENT);
	sqlite3_free(geohash);
}

static void lastRowsFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  sqlite3 *db = sqlite3_context_db_handle(context);
  sqlite3_result_int64(context, sqlite3_changes(db));
}

/*
** accumulate numbers returning tha accumulated value so far
* first parameter is the start value and the second is normally
* a database field to accumulate
*/
typedef struct
{
    union {
    sqlite_double fvalue;
    sqlite_int64 ivalue;
    } uvalue;
    int isDouble;
} accumulate_st;

static void accumulateFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  assert( argc==2 );
  int setAux = 0;           /* True to invoke sqlite3_set_auxdata() */
  //i64 iVal=0;
  accumulate_st *saved_value=sqlite3_get_auxdata(context, 0);
  if(saved_value==NULL)
  {
      saved_value = sqlite3_malloc(sizeof(*saved_value));
      setAux=1;
      switch( sqlite3_value_type(argv[0]) ){
        case SQLITE_INTEGER: {
          saved_value->uvalue.ivalue = sqlite3_value_int64(argv[0]);
	  saved_value->isDouble = 0;
          break;
        }
        case SQLITE_NULL: {
          saved_value->uvalue.fvalue = LITDBL(0.0);
	  saved_value->isDouble = 1;
          break;
        }
        default: {
          saved_value->uvalue.fvalue = sqlite3_value_double(argv[0]);
	  saved_value->isDouble = 1;
          break;
        }
      }
  }
  switch( sqlite3_value_type(argv[1]) ){
    case SQLITE_INTEGER: {
      if(saved_value->isDouble) saved_value->uvalue.fvalue += (sqlite_double)sqlite3_value_int64(argv[1]);
      else saved_value->uvalue.ivalue += sqlite3_value_int64(argv[1]);
      break;
    }
    case SQLITE_NULL: {
      break;
    }
    default: {
      if(saved_value->isDouble) saved_value->uvalue.fvalue += sqlite3_value_double(argv[1]);
      else saved_value->uvalue.ivalue += (sqlite_int64)sqlite3_value_double(argv[1]);
      break;
    }
  }
  if( setAux ){
    sqlite3_set_auxdata(context, 0, saved_value, (void(*)(void*))sqlite3_free);
  }
  if(saved_value->isDouble) sqlite3_result_double(context, saved_value->uvalue.fvalue);
  else sqlite3_result_int64(context, saved_value->uvalue.ivalue);
}

/*
** This function registered all of the above C functions as SQL
** functions.  This should be the only routine in this file with
** external linkage.
*/
int RegisterExtensionFunctions(sqlite3 *db){
  static const struct FuncDef {
     char *zName;
     signed char nArg;
     u8 argType;           /* 0: none.  1: db  2: (-1) */
     u8 eTextRep;          /* 1: UTF-16.  0: UTF-8 */
     u8 needCollSeq;
     void (*xFunc)(sqlite3_context*,int,sqlite3_value **);
  } aFuncs[] = {
    /* math.h */
    { "acos",               1, 0, SQLITE_UTF8,    0, acosFunc  },
    { "asin",               1, 0, SQLITE_UTF8,    0, asinFunc  },
    { "atan",               1, 0, SQLITE_UTF8,    0, atanFunc  },
    { "atn2",               2, 0, SQLITE_UTF8,    0, atn2Func  },
    /* XXX alias */
    { "atan2",              2, 0, SQLITE_UTF8,    0, atn2Func  },
    { "acosh",              1, 0, SQLITE_UTF8,    0, acoshFunc  },
    { "asinh",              1, 0, SQLITE_UTF8,    0, asinhFunc  },
    { "atanh",              1, 0, SQLITE_UTF8,    0, atanhFunc  },

    { "difference",         2, 0, SQLITE_UTF8,    0, differenceFunc},
    { "degrees",            1, 0, SQLITE_UTF8,    0, rad2degFunc  },
    { "radians",            1, 0, SQLITE_UTF8,    0, deg2radFunc  },

    { "cos",                1, 0, SQLITE_UTF8,    0, cosFunc  },
    { "sin",                1, 0, SQLITE_UTF8,    0, sinFunc },
    { "tan",                1, 0, SQLITE_UTF8,    0, tanFunc },
    { "cot",                1, 0, SQLITE_UTF8,    0, cotFunc },
    { "cosh",               1, 0, SQLITE_UTF8,    0, coshFunc  },
    { "sinh",               1, 0, SQLITE_UTF8,    0, sinhFunc },
    { "tanh",               1, 0, SQLITE_UTF8,    0, tanhFunc },
    { "coth",               1, 0, SQLITE_UTF8,    0, cothFunc },

    { "exp",                1, 0, SQLITE_UTF8,    0, expFunc  },
    { "log",                1, 0, SQLITE_UTF8,    0, logFunc  },
    { "ln",                 1, 0, SQLITE_UTF8,    0, logFunc  },
    { "log10",              1, 0, SQLITE_UTF8,    0, log10Func  },
    { "power",              2, 0, SQLITE_UTF8,    0, powerFunc  },
    { "sign",               1, 0, SQLITE_UTF8,    0, signFunc },
    { "sqrt",               1, 0, SQLITE_UTF8,    0, sqrtFunc },
    { "square",             1, 0, SQLITE_UTF8,    0, squareFunc },

    { "ceil",               1, 0, SQLITE_UTF8,    0, ceilFunc },
    { "ceiling",            1, 0, SQLITE_UTF8,    0, ceilFunc },
    { "floor",              1, 0, SQLITE_UTF8,    0, floorFunc },

    { "pi",                 0, 0, SQLITE_UTF8,    1, piFunc },

    { "last_rows_affected", 0, 0, SQLITE_UTF8,    0, lastRowsFunc },

    { "fmod",               2, 0, SQLITE_UTF8,    0, fmodFunc  },
    { "distance",           4, 0, SQLITE_UTF8,    0, sqlitex_distance  },
    { "bearing",           4, 0, SQLITE_UTF8,    0, sqlitex_bearing  },
    { "is_inside_circle",  5, 0, SQLITE_UTF8,    0, sqlitex_is_inside_circle  },
    { "geodistance",        5, 0, SQLITE_UTF8,    0, geo_distance  },
    { "geohash",            3, 0, SQLITE_UTF8,    0, encode_geohash  },
    { "geohashneighbor",    3, 0, SQLITE_UTF8,    0, get_geohash_neighbor  },

    /* string */
    { "replicate",          2, 0, SQLITE_UTF8,    0, replicateFunc },
    { "charindex",          2, 0, SQLITE_UTF8,    0, charindexFunc },
    { "charindex",          3, 0, SQLITE_UTF8,    0, charindexFunc },
    { "leftstr",            2, 0, SQLITE_UTF8,    0, leftFunc },
    { "rightstr",           2, 0, SQLITE_UTF8,    0, rightFunc },
#ifndef HAVE_TRIM
    { "ltrim",              1, 0, SQLITE_UTF8,    0, ltrimFunc },
    { "rtrim",              1, 0, SQLITE_UTF8,    0, rtrimFunc },
    { "trim",               1, 0, SQLITE_UTF8,    0, trimFunc },
    { "replace",            3, 0, SQLITE_UTF8,    0, replaceFunc },
#endif
    { "reverse",            1, 0, SQLITE_UTF8,    0, reverseFunc },
    { "proper",             1, 0, SQLITE_UTF8,    0, properFunc },
    { "padl",               2, 0, SQLITE_UTF8,    0, padlFunc },
    { "padr",               2, 0, SQLITE_UTF8,    0, padrFunc },
    { "padc",               2, 0, SQLITE_UTF8,    0, padcFunc },
    { "strfilter",          2, 0, SQLITE_UTF8,    0, strfilterFunc },
    { "accumulate",         2, 0, SQLITE_UTF8,    0, accumulateFunc },

  };
  /* Aggregate functions */
  static const struct FuncDefAgg {
    char *zName;
    signed char nArg;
    u8 argType;
    u8 needCollSeq;
    void (*xStep)(sqlite3_context*,int,sqlite3_value**);
    void (*xFinalize)(sqlite3_context*);
  } aAggs[] = {
    { "samplestdev",      1, 0, 0, varianceStep, samplestdevFinalize  },
    { "stdev",            1, 0, 0, varianceStep, stdevFinalize  },
    { "variance",         1, 0, 0, varianceStep, varianceFinalize  },
    { "mode",             1, 0, 0, modeStep,     modeFinalize  },
    { "median",           1, 0, 0, modeStep,     medianFinalize  },
    { "lower_quartile",   1, 0, 0, modeStep,     lower_quartileFinalize  },
    { "upper_quartile",   1, 0, 0, modeStep,     upper_quartileFinalize  },
  };
  int i;

  for(i=0; i<sizeof(aFuncs)/sizeof(aFuncs[0]); i++){
    void *pArg = 0;
    switch( aFuncs[i].argType ){
      case 1: pArg = db; break;
      case 2: pArg = (void *)(-1); break;
    }
    //sqlite3CreateFunc
    /* LMH no error checking */
    sqlite3_create_function(db, aFuncs[i].zName, aFuncs[i].nArg,
        aFuncs[i].eTextRep, pArg, aFuncs[i].xFunc, 0, 0);
#if 0
    if( aFuncs[i].needCollSeq ){
      struct FuncDef *pFunc = sqlite3FindFunction(db, aFuncs[i].zName,
          strlen(aFuncs[i].zName), aFuncs[i].nArg, aFuncs[i].eTextRep, 0);
      if( pFunc && aFuncs[i].needCollSeq ){
        pFunc->needCollSeq = 1;
      }
    }
#endif
  }

  for(i=0; i<sizeof(aAggs)/sizeof(aAggs[0]); i++){
    void *pArg = 0;
    switch( aAggs[i].argType ){
      case 1: pArg = db; break;
      case 2: pArg = (void *)(-1); break;
    }
    //sqlite3CreateFunc
    /* LMH no error checking */
    sqlite3_create_function(db, aAggs[i].zName, aAggs[i].nArg, SQLITE_UTF8,
        pArg, 0, aAggs[i].xStep, aAggs[i].xFinalize);
#if 0
    if( aAggs[i].needCollSeq ){
      struct FuncDefAgg *pFunc = sqlite3FindFunction( db, aAggs[i].zName,
          strlen(aAggs[i].zName), aAggs[i].nArg, SQLITE_UTF8, 0);
      if( pFunc && aAggs[i].needCollSeq ){
        pFunc->needCollSeq = 1;
      }
    }
#endif
  }
  return 0;
}

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
int sqlite3_extension_init(
    sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi){
  SQLITE_EXTENSION_INIT2(pApi);
  RegisterExtensionFunctions(db);
  return 0;
}
#endif /* COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE */

map map_make(cmp_func cmp){
  map r;
  r.cmp=cmp;
  r.base = 0;
  r.free = 0;

  return r;
}

void* myxcalloc(size_t nmemb, size_t size, char* s){
  void* ret = calloc(nmemb, size);
  return ret;
}

void myxfree(void* p){
  free(p);
}

void node_insert(node** n, cmp_func cmp, void *e){
  int c;
  node* nn;
  if(*n==0){
    nn = (node*)myxcalloc(1,sizeof(node), "for node");
    nn->data = e;
    nn->count = 1;
    *n=nn;
  }else{
    c=cmp((*n)->data,e);
    if(0==c){
      ++((*n)->count);
      myxfree(e);
    }else if(c>0){
      /* put it right here */
      node_insert(&((*n)->l), cmp, e);
    }else{
      node_insert(&((*n)->r), cmp, e);
    }
  }
}

void map_insert(map *m, void *e){
  node_insert(&(m->base), m->cmp, e);
}

void node_iterate(node *n, map_iterator iter, void* p){
  if(n){
    if(n->l)
      node_iterate(n->l, iter, p);
    iter(n->data, n->count, p);
    if(n->r)
      node_iterate(n->r, iter, p);
  }
}

void map_iterate(map *m, map_iterator iter, void* p){
  node_iterate(m->base, iter, p);
}

void node_destroy(node *n){
  if(0!=n){
    myxfree(n->data);
    if(n->l)
      node_destroy(n->l);
    if(n->r)
      node_destroy(n->r);

    myxfree(n);
  }
}

void map_destroy(map *m){
  node_destroy(m->base);
}

int int_cmp(const void *a, const void *b){
  sqlite_int64 aa = *(sqlite_int64 *)(a);
  sqlite_int64 bb = *(sqlite_int64 *)(b);
  /* printf("cmp %d <=> %d\n",aa,bb); */
  if(aa==bb)
    return 0;
  else if(aa<bb)
    return -1;
  else
    return 1;
}

int sqlite_double_cmp(const void *a, const void *b){
  sqlite_double aa = *(sqlite_double *)(a);
  sqlite_double bb = *(sqlite_double *)(b);
  /* printf("cmp %d <=> %d\n",aa,bb); */
  if(aa==bb)
    return 0;
  else if(aa<bb)
    return -1;
  else
    return 1;
}

void print_elem(void *e, sqlite_int64 c, void* p){
  int ee = *(int*)(e);
  printf("%d => %lld\n", ee,c);
}

#endif
#endif
