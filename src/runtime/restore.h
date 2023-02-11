#ifndef __RESTORE_H__
#define __RESTORE_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



void DG_RESTORE(int beginLine, int beginCol, int endLine, int endCol, char** names, char** types, void** refs, int* sizes, int var_size)
;

char* parse_sep(char* buf);
char* parse_ptr(char* buf, void** addr, int *sizes, int dim);
char* parse_byte(char* buf, void* ref, int size);
char* parse_int(char* buf, int* num);
char* init_buf(int beginLine, int beginCol, int endLine, int endCol);


#endif
