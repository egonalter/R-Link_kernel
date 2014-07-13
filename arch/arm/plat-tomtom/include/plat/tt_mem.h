#ifndef __TOMTOM_MEM_H__
#define __TOMTOM_MEM_H__

struct ttmem_info {
	void (* allow)(void);
	void (* disallow)(void);
};

#endif /* __TOMTOM_MEM_H__ */
