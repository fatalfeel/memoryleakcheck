#ifndef MEM_TRACER_H
#define MEM_TRACER_H

#include <stddef.h>

#ifdef __cplusplus
#include <new>
extern "C" {
#endif

#define MSG_ERR_HM_MALLOC "malloc() failed for allocation descriptors hashmap."

// Allocation descriptors hashmap size, increase to lower the load shown by dumpAlloc
//#define ALLOC_TABLE_SIZE 1733
#define ALLOC_TABLE_SIZE 2048

// Allocation descriptor hashmap structs
typedef struct allocdescr
{
	void*	ptr;
	char*	file;
	int		line;
	size_t	size;
} allocdescr;

typedef struct allocnode
{
	allocdescr*			info;
	struct allocnode*	next;
} allocnode;

typedef struct allochashmap
{
	size_t		size;
	allocnode** table;
} allochashmap;

void*	tracingMalloc(size_t size, const char* name, int line);
void*	tracingRealloc(void* ptr, int size, const char* name, int line);
void*	tracingCalloc(int num, int size, const char* name, int line);
void	tracingFree(void* ptr);
void	dumpAlloc();

#ifndef MEM_TRACER_C
	#define malloc(s) tracingMalloc(s,__FILE__,__LINE__)
	#define realloc(p,s) tracingRealloc(p,s,__FILE__,__LINE__)
	#define calloc(n,s) tracingCalloc(n,s,__FILE__,__LINE__)
	#define free(p) tracingFree(p)
#endif //ifndef MEM_TRACER_C

#ifdef __cplusplus
}
#endif


#ifndef MEM_TRACER_C
#ifdef __cplusplus
	//void*	operator new(size_t) throw(std::bad_alloc);
	void*	operator new(size_t) throw();
	void*	operator new(size_t,const char* name,int line);
	//void*	operator new[] (size_t) throw(std::bad_alloc);
	void*	operator new[](size_t) throw();
	void*	operator new[] (size_t,const char* name,int line);
	void	operator delete(void*) throw();
	void	operator delete[](void*) throw();
	#define tracingNew new (__FILE__,__LINE__)
	#define tracingDelete delete
	#define new tracingNew
	#define delete tracingDelete
#endif //ifdef _cplusplus
#endif //ifndef MEM_TRACER_C


#endif //ifndef MEM_TRACER_H 
