

#ifndef __TLIST__
#define __TLIST__

typedef struct _tlist {
	        void* data;
			            void* next;
} tlist;


tlist* tlist_new(int size);
void tlist_append(tlist* t,void* data,int size);
int tlist_size(tlist* t);
void* tlist_pop(tlist* t,int at);
void tlist_fini(tlist* );

#endif
