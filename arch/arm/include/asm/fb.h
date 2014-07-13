#ifndef _ASM_FB_H_
#define _ASM_FB_H_

#include <linux/fb.h>
#include <linux/fs.h>
#include <asm/page.h>

static inline void fb_pgprotect(struct file *file, struct vm_area_struct *vma,
				unsigned long off)
{
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
}

#ifdef CONFIG_FB_S3C
extern int fb_is_primary_device(struct fb_info *info);
#else
static inline int fb_is_primary_device(struct fb_info *info)
{
	return 0;
}
#endif
#endif /* _ASM_FB_H_ */
