#include "record.h" 

#define MAX_BUF 100000
#define MAX_PTR 100
#define MAX_NAME 30
#define MAX_DIM 5

//
// Share for all threads
//
bool len_init_flag = false;
// ptr -> len
char ptr_len_arr[MAX_PTR][MAX_DIM][MAX_NAME];
char ptr_arr[MAX_PTR][MAX_NAME];
char len_arr[MAX_PTR*MAX_DIM][MAX_NAME];
int ptr_cnt = 0, len_cnt = 0;


// Utility Function Declaration
int get_ptr_dim(char* t);
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
void join_path(char *dest, char* prefix, char* suffix);


// 
// API Designed to be thread-safe
//
void DG_RECORD(int beginLine, int beginCol, int endLine, int endCol, char** names, char** types, void** refs, int* sizes, int var_size){
	// Init once
	// Read mappings of ptr -> len from file
	if (!len_init_flag){
		init_ptrlen_table();
		len_init_flag = true;
	}

	// Table is for mappings of len -> val
	hashtable_t* lenval_table = ht_create(MAX_PTR*MAX_DIM);

	// set names to be saved
	init_lenval_table(lenval_table);


	// set runtime value of size of ptr
	for(int i=0; i<var_size; i++){
		set_ptr_size(lenval_table, names[i], refs[i]);
	}




	char *buf = malloc(MAX_BUF);
	char* pbuf = buf;
	memset(buf, 0, sizeof(buf));
	
	for(int i=0; i<var_size; i++){
		char *type = types[i];
		char *name = names[i];
		int size = sizes[i];
		void* ref = refs[i];
		// format
		// dim>0 $sizes {size0 bytes}...	
			// sizes is the size of each dimentions from 1 to n
			// size0 is the size of 0 dim, and dump {} with inorder traversal
        // dim=0 size bytes
        // dim=-1: NULL
		//pbuf = dump_name(name, pbuf);

        if (ref==NULL){ // one descendant is null, a->b->c  a or b is NULL
            pbuf = dump_int(-1, pbuf);
            continue;
        }

		int dim = get_ptr_dim(type);
		pbuf = dump_int(dim, pbuf);
		

		if (dim>0){
			int base_size = size;
			//pbuf = dump_int(base_size, pbuf);
			int* dim_sizes = get_ptr_size(lenval_table, name, base_size, dim);
			for(int i=1; i<dim; i++)
				pbuf = dump_int(dim_sizes[i], pbuf);
			
			// deference back to pointer, origainally it is address of pointer	
			
            void *ptr = *((void**) ref);
			pbuf = dump_ptr(ptr, dim, pbuf, dim_sizes, base_size);
			
			free(dim_sizes);
		} 
		else {
			pbuf = dump_int(size, pbuf); 
			pbuf = dump_byte(ref, pbuf, size);
		}

	}

	int buf_size = pbuf-buf;
	write_buf(buf, buf_size);
	free(buf);

}

int get_ptr_dim(char* t){
	char* found = strchr(t, '*');
	int dim = 0;
	while(found!=NULL && *found=='*'){
		dim++;
		found++;
	}
	return dim;
}




char* dump_sep(char* buf){
	strcpy(buf, " ");
	buf++;
	return buf;
}

char* dump_name(char* name, char* buf){
	strcpy(buf, name);
	buf += strlen(name);
	buf = dump_sep(buf);
	return buf;
}

char* dump_int(int num, char* buf){
	char num_buf[4];
	sprintf(num_buf, "%d", num);
	strcpy(buf, num_buf);
	buf += 4;
	buf = dump_sep(buf);
	return buf;
}

char* dump_byte(void* ref, char* buf, int size){
	char *p = (char*) ref;
	memcpy(buf, p, size);
	buf += size;
	buf = dump_sep(buf);
	return buf;
}

char* dump_ptr(void* ref, int dim, char* buf, int* sizes, int base_size){
	if (ref==NULL){ // null safety check
        buf = dump_int(-1, buf); // -1 -> null
        return buf;
    }
    buf = dump_int(0, buf); // 0 -> not null 

    if (dim==1){ // continue memory
        int size;
		if (base_size==1) // char*
			size = strlen((char*) ref) + 1; // for NULL
		else
			size = base_size*sizes[0];

		buf = dump_int(size, buf);	
		return dump_byte(ref, buf, size);
	}

	void** refs = (void**)ref;
	
	for(int i=0; i<sizes[dim-1]; i++){
		buf = dump_ptr(refs[i], dim-1, buf, sizes, base_size);
	}

	return buf;
}


void init_ptrlen_table(){
	char* filepath = getenv("DG_PTR_LEN");
    if (filepath==NULL)
        return;

	FILE* fp = fopen(filepath, "r");
	// format ptr len...
	char buf[256];
	char *p;
	
	while(fgets(buf, sizeof(buf), fp)){
		int dim = 0;
		p = strtok(buf, " \n");
		strcpy(ptr_arr[ptr_cnt], p);
		while(p!=NULL){
			p = strtok(NULL, " \n");
			if (p!=NULL){
				strcpy(ptr_len_arr[ptr_cnt][dim++], p);
				strcpy(len_arr[len_cnt++], p);
			}
		}
		ptr_cnt++;
	}


	fclose(fp);
}

void init_lenval_table(hashtable_t *hashtable){
	for(int i=0; i<len_cnt; i++){
		ht_set(hashtable, len_arr[i], 0);
	}
}

// ptr len cannot be array
void set_ptr_size(hashtable_t* hashtable, char* name, void* ref){
	int table_miss;
	ht_get(hashtable, name, &table_miss);
	if (!table_miss){
		int size = *((int*) ref);
		ht_set(hashtable, name, size);
	}
} 

int* get_ptr_size(hashtable_t* hashtable, char* name, int base_size, int dim){
	int ptr_idx = -1;
	// get ptr index corressponding to index of len
	for(int i=0; i<ptr_cnt; i++){
		if (!strcmp(name, ptr_arr[i])){
			ptr_idx = i;
			break;
		}
	}
	

	// Use name of len to get runtime value of len
	int *sizes = malloc(dim*sizeof(int));
	memset(sizes, 0, dim*sizeof(int));
	
	if (ptr_idx==-1) // Not specify in file
	{
		for(int i=0; i<dim; i++) 
            sizes[i] = 1; // default 1 for all dim
		return sizes;
	}

	// char 0 dim is ignored
	int begin_dim = 0;
	if (base_size==1) // char
		begin_dim = 1;

	int j = 0; // Take name from index 0
	int table_miss;
	for(int i=begin_dim; i<dim; i++){
		char *len_name = ptr_len_arr[ptr_idx][j++];
		sizes[i] = ht_get(hashtable, len_name, &table_miss);
	}

	return sizes;
}

void write_buf(char* buf, int buf_size){
	// Current time as filename
	char* dir = getenv("DG_RECORD_DIR");

	char filename[16];
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%lld", current_timestamp());

	char path[100];
	memset(path, 0, sizeof(path));
	join_path(path, dir, filename);


	FILE* fp = fopen(path, "w");
	fwrite(buf, sizeof(char), buf_size, fp);
	fclose(fp);
}

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

void join_path(char* dest, char* prefix, char* suffix){
	if (prefix[strlen(prefix)-1]=='/')
		sprintf(dest, "%s%s", prefix, suffix);
	else
		sprintf(dest, "%s/%s", prefix, suffix); 
}
