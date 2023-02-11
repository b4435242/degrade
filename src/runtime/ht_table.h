#ifndef _HT_TABLE_
#define _HT_TABLE_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

typedef int bool;

#define true 1
#define false 0


struct entry_s {
	char *key;
	int value;
	struct entry_s *next;
};
 
typedef struct entry_s entry_t;
 
struct hashtable_s {
	int size;
	struct entry_s **table;	
};
 
typedef struct hashtable_s hashtable_t;
 
 
/* Create a new hashtable. */
hashtable_t *ht_create( int size );

 
/* Hash a string for a particular hash table. */
int ht_hash( hashtable_t *hashtable, char *key );

/* Create a key-value pair. */
entry_t *ht_newpair( char *key, int value );

/* Insert a key-value pair into a hash table. */
void ht_set( hashtable_t *hashtable, char *key, int value);

/* Retrieve a key-value pair from a hash table. */
int ht_get( hashtable_t *hashtable, char *key, int* table_miss);  

/*
  End of hashtable implementation
 */

#endif
