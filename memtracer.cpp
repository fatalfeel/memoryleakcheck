#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_TRACER_C 1
#include "memtracer.h"

#ifdef __cplusplus
extern "C" {
#endif
// Allocation descriptors HashMap related functions
static size_t genHash(size_t max, void* ptr)
{
	size_t key= (size_t)ptr;
	return (key % max);
}

static int nextNotPow2(int num)
{
	int nextnotpow2=num;
	while ( (nextnotpow2!=0) && ((nextnotpow2 & (nextnotpow2-1)) == 0) ) 
	{
		nextnotpow2++;
	}
	return nextnotpow2;
}

/**
   Returns an empty hash of the chosen size
 */
allochashmap* hashmap_Init(int size)
{
	int asize=nextNotPow2(size);
	allochashmap* map=(allochashmap*) malloc(sizeof(allochashmap));
	assert((NULL!=map) &&  MSG_ERR_HM_MALLOC);
	map->size=asize;
	map->table=(allocnode**)calloc(asize,sizeof(allocnode));
	if (NULL==map->table) 
	{
		free(map);
		map=NULL;
	}
	assert((NULL!=map) &&  MSG_ERR_HM_MALLOC);
	return(map);
}

/**
   Free the hash with ALL its content, leaked pointers will be freed.
 */
void hashmap_Destroy(allochashmap* map)
{
	size_t i=0;
	for(i=0; i<map->size; ++i) 
	{
		while(map->table[i] != NULL)
		{
			allocnode* next=map->table[i]->next;
			free(map->table[i]->info->file);
			//free(map->table[i]->info->ptr); //external pointer as _CrtDumpMemoryLeaks();
			free(map->table[i]->info);
			free(map->table[i]);
			map->table[i]=next;
		}
	}
	free(map->table);
	map->table=NULL;
	free(map);
	map=NULL;
}

/**
   Remove the allocation descriptor of the given pointer, returns true: found, false:no found
 */
bool hashmap_Delete(allochashmap* map, void* ptr)
{
	size_t		idx	= genHash(map->size, ptr);
	allocnode*	cur	= map->table[idx];
	allocnode*	prev= map->table[idx];

	while(cur != NULL)
	{
		if (cur->info->ptr == ptr) 
		{
			//allocnode* found=cur;
			if (prev==cur) 
			{
				map->table[idx]=cur->next;
			}
			else
			{
				prev->next=cur->next;
			}
			
			free(cur->info->file);	//free(found->info->file);
			free(cur->info);		//free(found->info);
			free(cur);				//free(found);
			
			return true;
		}
		else
		{
			prev=cur;
			cur=cur->next;
		}
	}
	
	return false;
}

/**
   Print out the content of the hashmap using the given function as allocated block address - block size and stats on the load
 */
void hashmap_Print(allochashmap* map, int outp(const char *format, ...))
{
	//int	load	= 0;
	int		isfirst	= 1;
	
	for(size_t i=0; i<map->size; ++i)
	{
		allocnode* nextn = map->table[i];
		
		//if (nextn != NULL)
		//	load++;
		while(nextn != NULL)
		{
			if (isfirst)
			{
				isfirst = 0;
				outp("--- Memory Leak status ---\n");
			}
			
			outp("Addr:0x%016llX - Size: %llu allocated in %s:%d\n", (unsigned long long)nextn->info->ptr, (unsigned long long)nextn->info->size, nextn->info->file, nextn->info->line);
			nextn = nextn->next;
		}
	}
}

/**
   Insert a new allocation descriptor, returns hash key (pointer descriptor idx)
 */
size_t hashmap_Insert(allochashmap* map, allocdescr* data)
{
	size_t idx=genHash(map->size, data->ptr);
	allocnode* newnode= (allocnode*) calloc(1,sizeof(allocnode));
	assert((NULL!=newnode) &&  MSG_ERR_HM_MALLOC);
	newnode->info=data;
	newnode->next=map->table[idx];
	map->table[idx]=newnode;
	return idx;
}

/**
   Build an allocation descriptor
 */
allocdescr* buildAllocdescr(void* ptr, size_t size, const char* filename, int line)
{
	allocdescr* newn=(allocdescr*)malloc(sizeof(allocdescr));
	newn->ptr=ptr;
	newn->size=size;
	newn->file=(char*) malloc(50);
	strcpy(newn->file,filename);
	newn->line=line;
	return newn;
}

//--------------------------------------------------------------------------------------------
static allochashmap* allocMap=NULL;

void tracingInit()
{
	if (NULL == allocMap) 
	{
		allocMap=hashmap_Init(ALLOC_TABLE_SIZE);
	}
}

void* tracingMalloc(size_t size, const char* name, int line)
{
	tracingInit();
	void* ptr=malloc(size);
	hashmap_Insert(allocMap,buildAllocdescr(ptr,size,name,line));
	return ptr;
}

void* tracingRealloc(void* ptr, int size, const char* name, int line)
{
	tracingInit();
	hashmap_Delete(allocMap,ptr);
	void* ptrnew=realloc(ptr,size);
	hashmap_Insert(allocMap,buildAllocdescr(ptrnew,size,name,line));
	return ptr;
}

void* tracingCalloc(int num, int size, const char* name, int line)
{
	tracingInit();
	void* ptr=calloc(num,size);
	hashmap_Insert(allocMap,buildAllocdescr(ptr,num*size,name,line));
	return ptr;
}

void tracingFree(void* ptr)
{
	tracingInit();
	hashmap_Delete(allocMap,ptr);
	free(ptr);
}

void dumpAlloc()
{
    tracingInit();
	hashmap_Print(allocMap,&printf);
	hashmap_Destroy(allocMap);
}
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void* operator new(size_t size) throw()
{
	return tracingMalloc(size,"-N/A-",0);
}

void* operator new(size_t size, const char* name, int line)
{
	return tracingMalloc(size,name,line);
}

//void* operator new[] (size_t size) throw(std::bad_alloc)
void* operator new[](size_t size) throw()
{
	return tracingMalloc(size,"-N/A-",0);
}

void * operator new[] (size_t size, const char* name, int line)
{
	return tracingMalloc(size,name,line);
}

void operator delete(void* ptr) throw()
{
	tracingFree(ptr);
}

void operator delete(void* ptr, const char* name, int line)
{
	tracingFree(ptr);
}

void operator delete[](void* ptr) throw()
{
	tracingFree(ptr);
}

void operator delete[](void* ptr, const char* name, int line)
{
	tracingFree(ptr);
}
#endif //ifdef __cplusplus

