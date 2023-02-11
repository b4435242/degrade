#ifndef _RECORD_H_
#define _RECORD_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#include "ht_table.h"
//
// API
// typename -> var_id -> val
//
void DG_RECORD(int beginLine, int beginCol, int endLine, int endCol, char** names, char** types, void** refs, int* sizes, int var_size);
//void DG_RESTORE(int beginLine, int beginCol, int endLine, int endCol, char** names, char** types, void** refs, int* sizes, int var_size);
void DG_RESTORE(int beginLine, int beginCol, int endLine, int endCol, char* name, char* type, void* ref, int size);
// utils

/*int get_ptr_dim(char* t);
int get_ptr_basesize(char* t);
char* dump_sep(char* buf);
char* dump_name(char* name, char* buf);
char* dump_int(int num, char* buf);
char* dump_byte(void* ref, char* buf, int size);
char* dump_ptr(void* ref, int dim, char* buf, int* sizes, int base_size);
void init_ptrlen_table();
void init_lenval_table();
void set_ptr_size(hashtable_t* hashtable, char* name, void* ref);
int* get_ptr_size(hashtable_t* hashtable, char* name, int base_size, int dim);
void write_buf(char* buf, int size);
long long current_timestamp();
void join_path(char *dest, char* prefix, char* suffix);*/
#endif
