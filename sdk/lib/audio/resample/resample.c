#include "basic_include.h"
#include "lib/audio/resample/resample.h"

AURES_MALLOC 	 aures_malloc  = NULL;
AURES_ZALLOC 	 aures_zalloc  = NULL;
AURES_CALLOC 	 aures_calloc  = NULL;
AURES_REALLOC    aures_realloc = NULL;
AURES_FREE       aures_free    = NULL;

void reg_aures_alloc(AURES_MALLOC m, AURES_ZALLOC z, AURES_CALLOC c, AURES_REALLOC r, AURES_FREE f)
{
	aures_malloc  = m;
	aures_zalloc  = z;
	aures_calloc  = c;
	aures_realloc = r;
	aures_free    = f;
}