#ifndef _memmem_h__
#define _memmem_h__

void *memmem(const void *haystack, size_t haystack_len,
		const void *needle, size_t needle_len);

#endif
