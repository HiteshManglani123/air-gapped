#ifndef MACROS_H
#define MACROS_H

// #define MAX_PRIME 50000
#define MAX_PRIME 100000000000

#define container_of(ptr, type, member)({			\
	const typeof(((type *)0)->member) *__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)) ; })

#endif // MACROS_H
