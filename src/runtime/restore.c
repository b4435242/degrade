#include "record.h"

char *buf;

char* parse_sep(char* buf);
char* parse_ptr(char* buf, void** addr, int *sizes, int dim);
char* parse_byte(char* buf, void* ref, int size);
char* parse_int(char* buf, int* num);
char* init_buf(int beginLine, int beginCol, int endLine, int endCol);


//
// API Designed to be thread-safe
//

void DG_RESTORE(int beginLine, int beginCol, int endLine, int endCol, char* name, char* type, void* ref, int size)
{
	if (!buf)
		buf = init_buf(beginLine, beginCol, endLine, endCol);
	
	int dim;
	buf = parse_int(buf, &dim);
		
    if (dim==-1)
    	return;    

	if (dim>0) {
		//int base_size;
		int *dim_sizes = malloc(sizeof(int)*dim);
		//pbuf = parse_int(pbuf, &base_size);
		for(int i=1; i<dim; i++){
			buf = parse_int(buf, &dim_sizes[i]);
		}
		    
            
		// Use addr of ptr to modify where ptr points to
		void **addr = (void**) ref;
		buf = parse_ptr(buf, addr, dim_sizes, dim);
	} else {
		int size;	
		buf = parse_int(buf, &size);
		buf = parse_byte(buf, ref, size);
	}

}
/*
void DG_RESTORE(int beginLine, int beginCol, int endLine, int endCol, char** names, char** types, void** refs, int* sizes, int var_size)
{
	char* buf = init_buf(beginLine, beginCol, endLine, endCol);
	char* pbuf = buf;
	for(int i=0; i<var_size; i++){
		void* ref = refs[i];		
		//printf("%s %s\n", names[i], types[i]);
		int dim;
		pbuf = parse_int(pbuf, &dim);
		
        if (dim==-1)
            continue;
        

		if (dim>0) {
			//int base_size;
			int *dim_sizes = malloc(sizeof(int)*dim);
			//pbuf = parse_int(pbuf, &base_size);
			for(int i=1; i<dim; i++){
				pbuf = parse_int(pbuf, &dim_sizes[i]);
			}
		    
            
			// Use addr of ptr to modify where ptr points to
			void **addr = (void**) ref;
			pbuf = parse_ptr(pbuf, addr, dim_sizes, dim);
		} else {
			int size;
			
			pbuf = parse_int(pbuf, &size);
			pbuf = parse_byte(pbuf, ref, size);
		}
	}	



}*/

char* parse_sep(char* buf){
	int sep_size = 1;
	return buf + sep_size;
}

char* parse_ptr(char* buf, void** addr, int *sizes, int dim){
	int nullflag;
    buf = parse_int(buf, &nullflag);
    if (nullflag==-1){
        return buf;
    }

    if (dim==1){
		int size;
		buf = parse_int(buf, &size);
		*addr = malloc(size*sizeof(char));
		memcpy(*addr, buf, size);
		buf += size;
		buf = parse_sep(buf);
		return buf;
	}	

	
	*addr = malloc(sizeof(void*)*sizes[dim-1]);


	for(int i=0; i<sizes[dim-1]; i++){
		buf = parse_ptr(buf, (void**)(*addr+i*8), sizes, dim-1);
	}

	return buf;
}

char* parse_byte(char* buf, void* ref, int size){
	memcpy(ref, buf, size);
	buf += size;
	buf = parse_sep(buf);
	return buf;
}

char* parse_int(char* buf, int* num){
	*num = atoi(buf);
	buf += 4;
	buf = parse_sep(buf);
	return buf;
}



char* init_buf(int beginLine, int beginCol, int endLine, int endCol){
	//
	// path NOT DETERMINED YET
	//
	char *path = getenv("DG_RESTORE_PATH");

	FILE* fp = fopen(path, "r");
	if (fp == NULL) {
    	perror("Error opening file");
    	return NULL;
	}

	// Determine the size of the file
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Allocate a character array to hold the contents of the file
	char *buffer = malloc(file_size * sizeof(char));
	if (buffer == NULL) {
    	perror("Error allocating memory");
   		fclose(fp);
    	return NULL;
	}

	// Read the contents of the file into the character array
	int bytes_read = fread(buffer, sizeof(char), file_size, fp);
	if (bytes_read != file_size) {
    	perror("Error reading file");
    	free(buffer);
    	fclose(fp);
    	return NULL;
	}

	// Close the file
	fclose(fp);
	return buffer;
}


