#include "tlist.h"
#include <stdlib.h>
#include <string.h>

tlist* tlist_new(int size){
    tlist* t=malloc(sizeof(tlist));
    memset(t,0,sizeof(tlist));
    return t;
}

void tlist_append(tlist* t,void* data,int size){
    tlist* next=tlist_new(0);
    tlist* pos=t;
    while (pos && pos->next) {
		pos=pos->next;
	}

	pos->data=malloc(size);
	memcpy(pos->data,data,size);
    pos->next=next;
}
int tlist_size(tlist* t){
    int ret=0;
    tlist* pos=t;
    while (pos && pos->next && pos->data) {
        pos=pos->next ;
        ret++;
    };
    return ret;
}
void* tlist_pop(tlist* t,int at){
    int ret=0;
    tlist* pos=t;
    /*if (pos && !pos->next ){
        return pos->data;
    }*/
    while (pos && pos->next) {
        if (ret==at){
            tlist* n=pos->next;
            pos->data=n->data;
            pos->next=n->next;
            return pos->data;
        }
        pos=pos->next ;
        ret++;
    };
    return NULL;
}
void tlist_fini(tlist* list){
	tlist* head=list;
	while (head){
		free(head->data);
		tlist *del=head;
		head=head->next;
		free(del);
	}
}
