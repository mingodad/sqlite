/*
** 2006 Oct 10
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** Implementation of the "sublatin" sublatin full-text-search tokenizer.
*/

/*
** The code in this file is only compiled if:
**
**     * The FTS3 module is being built as an extension
**       (in which case SQLITE_CORE is not defined), or
**
**     * The FTS3 module is being built into the core of
**       SQLite (in which case SQLITE_ENABLE_FTS3 is defined).
*/
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)

#include "fts3_tokenizer.h"
#include <string.h>


const unsigned char toLowerDeaccentedSubSetLatinUtf8MapTable[] = { //0
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,//16
16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,//32
32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,//48
48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,//64
64,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,//80
112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,//96
96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,//112
112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,//128
97,97,97,97,97,97,134,99,101,101,101,101,105,105,105,105,//144
144,110,111,111,111,111,111,151,111,117,117,117,117,121,158,159,//160
97,97,97,97,97,97,166,99,101,101,101,101,105,105,105,105,//176
176,110,111,111,111,111,111,183,111,117,117,117,117,121,190,121,//192
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,//208
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,//224
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,//240
240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

const static char *SPANISH_STOP_WORDS[]  = {
"al",
"ante",
"asi",
"cl",
"como",
"con",
"cuyo",
"de",
"del",
"desde",
"don",
"dt",
"el",
"en",
"es",
"esta",
"este",
"fin",
"ha",
"la",
"las",
"le",
"lo",
"los",
"ma",
"misma",
"no",
"para",
"por",
"pz",
"que",
"se",
"ser",
"si",
"sido",
"siguientes",
"sin",
"sl",
"su",
"sus",
"todos",
"un",
"una",
"y"
};
    
typedef struct sublatin_tokenizer {
  sqlite3_tokenizer base;
  char delim[128];             /* flag ASCII delimiters */
} sublatin_tokenizer;

typedef struct sublatin_tokenizer_cursor {
  sqlite3_tokenizer_cursor base;
  const char *pInput;          /* input we are tokenizing */
  int nBytes;                  /* size of the input */
  int iOffset;                 /* current position in pInput */
  int iToken;                  /* index of next token to be returned */
  char *pToken;                /* storage for current token */
  int nTokenAllocated;         /* space allocated to zToken buffer */
} sublatin_tokenizer_cursor;


/* Forward declaration */
static const sqlite3_tokenizer_module sublatinTokenizerModule;

static int sublatinDelim(sublatin_tokenizer *t, unsigned char c){
  return c<0x80 && t->delim[c];
}

static int sublatinIsStopwrod( char *word, int wlen){
	int result, middle, low = 0, high = (sizeof(SPANISH_STOP_WORDS)/sizeof(char*))-1;
	char buf[32];
	if(wlen >= sizeof(buf)) return 0;
	strncpy(buf, word, wlen);
	buf[wlen] = 0;
    while (low <= high) {
        middle = low + (high - low)/2;
		result = strcmp(SPANISH_STOP_WORDS[middle], buf);
		if(result > 0) high = middle - 1;
		else if(result < 0) low = middle + 1;
		else return 1;
    }
    return 0;
}

/*
** Create a new tokenizer instance.
*/
static int sublatinCreate(
  int argc, const char * const *argv,
  sqlite3_tokenizer **ppTokenizer
){
  sublatin_tokenizer *t;

  t = (sublatin_tokenizer *) sqlite3_malloc(sizeof(*t));
  if( t==NULL ) return SQLITE_NOMEM;
  memset(t, 0, sizeof(*t));

  /* TODO(shess) Delimiters need to remain the same from run to run,
  ** else we need to reindex.  One solution would be a meta-table to
  ** track such information in the database, then we'd only want this
  ** information on the initial create.
  */
  if( argc>1 ){
    int i, n = strlen(argv[1]);
    for(i=0; i<n; i++){
      unsigned char ch = argv[1][i];
      /* We explicitly don't support UTF-8 delimiters for now. */
      if( ch>=0x80 ){
        sqlite3_free(t);
        return SQLITE_ERROR;
      }
      t->delim[ch] = 1;
    }
  } else {
    /* Mark non-alphanumeric ASCII characters as delimiters */
    int i;
    for(i=1; i<0x80; i++){
      t->delim[i] = !isalnum(i);
    }
//    for(char *pNotDelimin = "/"; *pNotDelimin; pNotDelimin++)
//        t->delimin[*pNotDelimin] = 0;
  }

  *ppTokenizer = &t->base;
  return SQLITE_OK;
}

/*
** Destroy a tokenizer
*/
static int sublatinDestroy(sqlite3_tokenizer *pTokenizer){
  sqlite3_free(pTokenizer);
  return SQLITE_OK;
}

/*
** Prepare to begin tokenizing a particular string.  The input
** string to be tokenized is pInput[0..nBytes-1].  A cursor
** used to incrementally tokenize this string is returned in 
** *ppCursor.
*/
static int sublatinOpen(
  sqlite3_tokenizer *pTokenizer,         /* The tokenizer */
  const char *pInput, int nBytes,        /* String to be tokenized */
  sqlite3_tokenizer_cursor **ppCursor    /* OUT: Tokenization cursor */
){
  sublatin_tokenizer_cursor *c;

  c = (sublatin_tokenizer_cursor *) sqlite3_malloc(sizeof(*c));
  if( c==NULL ) return SQLITE_NOMEM;

  c->pInput = pInput;
  if( pInput==0 ){
    c->nBytes = 0;
  }else if( nBytes<0 ){
    c->nBytes = (int)strlen(pInput);
  }else{
    c->nBytes = nBytes;
  }
  c->iOffset = 0;                 /* start tokenizing at the beginning */
  c->iToken = 0;
  c->pToken = NULL;               /* no space allocated, yet. */
  c->nTokenAllocated = 0;

  *ppCursor = &c->base;
  return SQLITE_OK;
}

/*
** Close a tokenization cursor previously opened by a call to
** sublatinOpen() above.
*/
static int sublatinClose(sqlite3_tokenizer_cursor *pCursor){
  sublatin_tokenizer_cursor *c = (sublatin_tokenizer_cursor *) pCursor;
  sqlite3_free(c->pToken);
  sqlite3_free(c);
  return SQLITE_OK;
}

/*
** Extract the next token from a tokenization cursor.  The cursor must
** have been opened by a prior call to sublatinOpen().
*/
static int sublatinNext(
  sqlite3_tokenizer_cursor *pCursor,  /* Cursor returned by sublatinOpen */
  const char **ppToken,               /* OUT: *ppToken is the token text */
  int *pnBytes,                       /* OUT: Number of bytes in token */
  int *piStartOffset,                 /* OUT: Starting offset of token */
  int *piEndOffset,                   /* OUT: Ending offset of token */
  int *piPosition                     /* OUT: Position integer of token */
){
  sublatin_tokenizer_cursor *c = (sublatin_tokenizer_cursor *) pCursor;
  sublatin_tokenizer *t = (sublatin_tokenizer *) pCursor->pTokenizer;
  unsigned char *p = (unsigned char *)c->pInput;

  while( c->iOffset<c->nBytes ){
    int iStartOffset;

    /* Scan past delimiter characters */
    while( c->iOffset<c->nBytes && sublatinDelim(t, p[c->iOffset]) ){
      c->iOffset++;
    }

    /* Count non-delimiter characters. */
    iStartOffset = c->iOffset;
    //while( c->iOffset<c->nBytes && !sublatinDelim(t, p[c->iOffset]) ){
    //  c->iOffset++;
    //}
    while( c->iOffset<c->nBytes ) {
        int specialCase = 0;
        switch(p[c->iOffset]){
            case '/':
            case '.':
            case ',': 
            	{ // for cases like 234/987, 2.456 2.456,98
                	if( (c->iOffset > 0) && 
                		isdigit(p[c->iOffset-1]) && 
                		(c->iOffset < c->nBytes-1) && 
                		isdigit(p[c->iOffset+1]) 
                		) specialCase = 1;
            	}
            	break;
            default:
            	if(sublatinDelim(t, p[c->iOffset])) break;
        }
        if( !specialCase && sublatinDelim(t, p[c->iOffset]) )
        	break;
      	c->iOffset++;
    }

    if( c->iOffset>iStartOffset ){
      int i, i2, n = c->iOffset-iStartOffset;
      if( n>c->nTokenAllocated ){
        c->nTokenAllocated = n+20;
        c->pToken = sqlite3_realloc(c->pToken, c->nTokenAllocated);
        if( c->pToken==NULL ) return SQLITE_NOMEM;
      }
      for(i=i2=0; i<n; i++, i2++){
        /* TODO(shess) This needs expansion to handle UTF-8
        ** case-insensitivity.
        */
        unsigned char ch = p[iStartOffset+i];
		if (ch > 0x80 && ch == 195) {
			unsigned char ch2 = p[iStartOffset+i+1];
			if( ch2 > 128) {
				c->pToken[i2] = toLowerDeaccentedSubSetLatinUtf8MapTable[ch2];
				i++;
			} else {
				c->pToken[i2] = ch;
			}
		} else c->pToken[i2] = tolower(ch);
      }
      if((i2 == 1) || sublatinIsStopwrod(c->pToken, i2)) continue;
      *ppToken = c->pToken;
      *pnBytes = i2;
      *piStartOffset = iStartOffset;
      *piEndOffset = c->iOffset;
      *piPosition = c->iToken++;

      return SQLITE_OK;
    }
  }
  return SQLITE_DONE;
}

/*
** The set of routines that implement the sublatin tokenizer
*/
static const sqlite3_tokenizer_module sublatinTokenizerModule = {
  0,
  sublatinCreate,
  sublatinDestroy,
  sublatinOpen,
  sublatinClose,
  sublatinNext,
};

/*
** Allocate a new sublatin tokenizer.  Return a pointer to the new
** tokenizer in *ppModule
*/
void sqlite3Fts3SubLatinTokenizerModule(
  sqlite3_tokenizer_module const**ppModule
){
  *ppModule = &sublatinTokenizerModule;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3) */
