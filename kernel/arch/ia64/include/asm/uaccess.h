
#ifndef _ASM_IA64_UACCESS_H
#define _ASM_IA64_UACCESS_H


#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/page-flags.h>
#include <linux/mm.h>

#include <asm/intrinsics.h>
#include <asm/pgtable.h>
#include <asm/io.h>

#define KERNEL_DS	((mm_segment_t) { ~0UL })		/* cf. access_ok() */
#define USER_DS		((mm_segment_t) { TASK_SIZE-1 })	/* cf. access_ok() */

#define VERIFY_READ	0
#define VERIFY_WRITE	1

#define get_ds()  (KERNEL_DS)
#define get_fs()  (current_thread_info()->addr_limit)
#define set_fs(x) (current_thread_info()->addr_limit = (x))

#define segment_eq(a, b)	((a).seg == (b).seg)

#define __access_ok(addr, size, segment)						\
({											\
	__chk_user_ptr(addr);								\
	(likely((unsigned long) (addr) <= (segment).seg)				\
	 && ((segment).seg == KERNEL_DS.seg						\
	     || likely(REGION_OFFSET((unsigned long) (addr)) < RGN_MAP_LIMIT)));	\
})
#define access_ok(type, addr, size)	__access_ok((addr), (size), get_fs())

#define put_user(x, ptr)	__put_user_check((__typeof__(*(ptr))) (x), (ptr), sizeof(*(ptr)), get_fs())
#define get_user(x, ptr)	__get_user_check((x), (ptr), sizeof(*(ptr)), get_fs())

#define __put_user(x, ptr)	__put_user_nocheck((__typeof__(*(ptr))) (x), (ptr), sizeof(*(ptr)))
#define __get_user(x, ptr)	__get_user_nocheck((x), (ptr), sizeof(*(ptr)))

extern long __put_user_unaligned_unknown (void);

#define __put_user_unaligned(x, ptr)								\
({												\
	long __ret;										\
	switch (sizeof(*(ptr))) {								\
		case 1: __ret = __put_user((x), (ptr)); break;					\
		case 2: __ret = (__put_user((x), (u8 __user *)(ptr)))				\
			| (__put_user((x) >> 8, ((u8 __user *)(ptr) + 1))); break;		\
		case 4: __ret = (__put_user((x), (u16 __user *)(ptr)))				\
			| (__put_user((x) >> 16, ((u16 __user *)(ptr) + 1))); break;		\
		case 8: __ret = (__put_user((x), (u32 __user *)(ptr)))				\
			| (__put_user((x) >> 32, ((u32 __user *)(ptr) + 1))); break;		\
		default: __ret = __put_user_unaligned_unknown();				\
	}											\
	__ret;											\
})

extern long __get_user_unaligned_unknown (void);

#define __get_user_unaligned(x, ptr)								\
({												\
	long __ret;										\
	switch (sizeof(*(ptr))) {								\
		case 1: __ret = __get_user((x), (ptr)); break;					\
		case 2: __ret = (__get_user((x), (u8 __user *)(ptr)))				\
			| (__get_user((x) >> 8, ((u8 __user *)(ptr) + 1))); break;		\
		case 4: __ret = (__get_user((x), (u16 __user *)(ptr)))				\
			| (__get_user((x) >> 16, ((u16 __user *)(ptr) + 1))); break;		\
		case 8: __ret = (__get_user((x), (u32 __user *)(ptr)))				\
			| (__get_user((x) >> 32, ((u32 __user *)(ptr) + 1))); break;		\
		default: __ret = __get_user_unaligned_unknown();				\
	}											\
	__ret;											\
})

#ifdef ASM_SUPPORTED
  struct __large_struct { unsigned long buf[100]; };
# define __m(x) (*(struct __large_struct __user *)(x))

/* We need to declare the __ex_table section before we can use it in .xdata.  */
asm (".section \"__ex_table\", \"a\"\n\t.previous");

# define __get_user_size(val, addr, n, err)							\
do {												\
	register long __gu_r8 asm ("r8") = 0;							\
	register long __gu_r9 asm ("r9");							\
	asm ("\n[1:]\tld"#n" %0=%2%P2\t// %0 and %1 get overwritten by exception handler\n"	\
	     "\t.xdata4 \"__ex_table\", 1b-., 1f-.+4\n"						\
	     "[1:]"										\
	     : "=r"(__gu_r9), "=r"(__gu_r8) : "m"(__m(addr)), "1"(__gu_r8));			\
	(err) = __gu_r8;									\
	(val) = __gu_r9;									\
} while (0)

# define __put_user_size(val, addr, n, err)							\
do {												\
	register long __pu_r8 asm ("r8") = 0;							\
	asm volatile ("\n[1:]\tst"#n" %1=%r2%P1\t// %0 gets overwritten by exception handler\n"	\
		      "\t.xdata4 \"__ex_table\", 1b-., 1f-.\n"					\
		      "[1:]"									\
		      : "=r"(__pu_r8) : "m"(__m(addr)), "rO"(val), "0"(__pu_r8));		\
	(err) = __pu_r8;									\
} while (0)

#else /* !ASM_SUPPORTED */
# define RELOC_TYPE	2	/* ip-rel */
# define __get_user_size(val, addr, n, err)				\
do {									\
	__ld_user("__ex_table", (unsigned long) addr, n, RELOC_TYPE);	\
	(err) = ia64_getreg(_IA64_REG_R8);				\
	(val) = ia64_getreg(_IA64_REG_R9);				\
} while (0)
# define __put_user_size(val, addr, n, err)							\
do {												\
	__st_user("__ex_table", (unsigned long) addr, n, RELOC_TYPE, (unsigned long) (val));	\
	(err) = ia64_getreg(_IA64_REG_R8);							\
} while (0)
#endif /* !ASM_SUPPORTED */

extern void __get_user_unknown (void);

#define __do_get_user(check, x, ptr, size, segment)					\
({											\
	const __typeof__(*(ptr)) __user *__gu_ptr = (ptr);				\
	__typeof__ (size) __gu_size = (size);						\
	long __gu_err = -EFAULT;							\
	unsigned long __gu_val = 0;							\
	if (!check || __access_ok(__gu_ptr, size, segment))				\
		switch (__gu_size) {							\
		      case 1: __get_user_size(__gu_val, __gu_ptr, 1, __gu_err); break;	\
		      case 2: __get_user_size(__gu_val, __gu_ptr, 2, __gu_err); break;	\
		      case 4: __get_user_size(__gu_val, __gu_ptr, 4, __gu_err); break;	\
		      case 8: __get_user_size(__gu_val, __gu_ptr, 8, __gu_err); break;	\
		      default: __get_user_unknown(); break;				\
		}									\
	(x) = (__typeof__(*(__gu_ptr))) __gu_val;					\
	__gu_err;									\
})

#define __get_user_nocheck(x, ptr, size)	__do_get_user(0, x, ptr, size, KERNEL_DS)
#define __get_user_check(x, ptr, size, segment)	__do_get_user(1, x, ptr, size, segment)

extern void __put_user_unknown (void);

#define __do_put_user(check, x, ptr, size, segment)					\
({											\
	__typeof__ (x) __pu_x = (x);							\
	__typeof__ (*(ptr)) __user *__pu_ptr = (ptr);					\
	__typeof__ (size) __pu_size = (size);						\
	long __pu_err = -EFAULT;							\
											\
	if (!check || __access_ok(__pu_ptr, __pu_size, segment))			\
		switch (__pu_size) {							\
		      case 1: __put_user_size(__pu_x, __pu_ptr, 1, __pu_err); break;	\
		      case 2: __put_user_size(__pu_x, __pu_ptr, 2, __pu_err); break;	\
		      case 4: __put_user_size(__pu_x, __pu_ptr, 4, __pu_err); break;	\
		      case 8: __put_user_size(__pu_x, __pu_ptr, 8, __pu_err); break;	\
		      default: __put_user_unknown(); break;				\
		}									\
	__pu_err;									\
})

#define __put_user_nocheck(x, ptr, size)	__do_put_user(0, x, ptr, size, KERNEL_DS)
#define __put_user_check(x, ptr, size, segment)	__do_put_user(1, x, ptr, size, segment)

extern unsigned long __must_check __copy_user (void __user *to, const void __user *from,
					       unsigned long count);

static inline unsigned long
__copy_to_user (void __user *to, const void *from, unsigned long count)
{
	return __copy_user(to, (__force void __user *) from, count);
}

static inline unsigned long
__copy_from_user (void *to, const void __user *from, unsigned long count)
{
	return __copy_user((__force void __user *) to, from, count);
}

#define __copy_to_user_inatomic		__copy_to_user
#define __copy_from_user_inatomic	__copy_from_user
#define copy_to_user(to, from, n)							\
({											\
	void __user *__cu_to = (to);							\
	const void *__cu_from = (from);							\
	long __cu_len = (n);								\
											\
	if (__access_ok(__cu_to, __cu_len, get_fs()))					\
		__cu_len = __copy_user(__cu_to, (__force void __user *) __cu_from, __cu_len);	\
	__cu_len;									\
})

#define copy_from_user(to, from, n)							\
({											\
	void *__cu_to = (to);								\
	const void __user *__cu_from = (from);						\
	long __cu_len = (n);								\
											\
	__chk_user_ptr(__cu_from);							\
	if (__access_ok(__cu_from, __cu_len, get_fs()))					\
		__cu_len = __copy_user((__force void __user *) __cu_to, __cu_from, __cu_len);	\
	__cu_len;									\
})

#define __copy_in_user(to, from, size)	__copy_user((to), (from), (size))

static inline unsigned long
copy_in_user (void __user *to, const void __user *from, unsigned long n)
{
	if (likely(access_ok(VERIFY_READ, from, n) && access_ok(VERIFY_WRITE, to, n)))
		n = __copy_user(to, from, n);
	return n;
}

extern unsigned long __do_clear_user (void __user *, unsigned long);

#define __clear_user(to, n)		__do_clear_user(to, n)

#define clear_user(to, n)					\
({								\
	unsigned long __cu_len = (n);				\
	if (__access_ok(to, __cu_len, get_fs()))		\
		__cu_len = __do_clear_user(to, __cu_len);	\
	__cu_len;						\
})


extern long __must_check __strncpy_from_user (char *to, const char __user *from, long to_len);

#define strncpy_from_user(to, from, n)					\
({									\
	const char __user * __sfu_from = (from);			\
	long __sfu_ret = -EFAULT;					\
	if (__access_ok(__sfu_from, 0, get_fs()))			\
		__sfu_ret = __strncpy_from_user((to), __sfu_from, (n));	\
	__sfu_ret;							\
})

/* Returns: 0 if bad, string length+1 (memory size) of string if ok */
extern unsigned long __strlen_user (const char __user *);

#define strlen_user(str)				\
({							\
	const char __user *__su_str = (str);		\
	unsigned long __su_ret = 0;			\
	if (__access_ok(__su_str, 0, get_fs()))		\
		__su_ret = __strlen_user(__su_str);	\
	__su_ret;					\
})

extern unsigned long __strnlen_user (const char __user *, long);

#define strnlen_user(str, len)					\
({								\
	const char __user *__su_str = (str);			\
	unsigned long __su_ret = 0;				\
	if (__access_ok(__su_str, 0, get_fs()))			\
		__su_ret = __strnlen_user(__su_str, len);	\
	__su_ret;						\
})

/* Generic code can't deal with the location-relative format that we use for compactness.  */
#define ARCH_HAS_SORT_EXTABLE
#define ARCH_HAS_SEARCH_EXTABLE

struct exception_table_entry {
	int addr;	/* location-relative address of insn this fixup is for */
	int cont;	/* location-relative continuation addr.; if bit 2 is set, r9 is set to 0 */
};

extern void ia64_handle_exception (struct pt_regs *regs, const struct exception_table_entry *e);
extern const struct exception_table_entry *search_exception_tables (unsigned long addr);

static inline int
ia64_done_with_exception (struct pt_regs *regs)
{
	const struct exception_table_entry *e;
	e = search_exception_tables(regs->cr_iip + ia64_psr(regs)->ri);
	if (e) {
		ia64_handle_exception(regs, e);
		return 1;
	}
	return 0;
}

#define ARCH_HAS_TRANSLATE_MEM_PTR	1
static __inline__ char *
xlate_dev_mem_ptr (unsigned long p)
{
	struct page *page;
	char * ptr;

	page = pfn_to_page(p >> PAGE_SHIFT);
	if (PageUncached(page))
		ptr = (char *)p + __IA64_UNCACHED_OFFSET;
	else
		ptr = __va(p);

	return ptr;
}

static __inline__ char *
xlate_dev_kmem_ptr (char * p)
{
	struct page *page;
	char * ptr;

	page = virt_to_page((unsigned long)p);
	if (PageUncached(page))
		ptr = (char *)__pa(p) + __IA64_UNCACHED_OFFSET;
	else
		ptr = p;

	return ptr;
}

#endif /* _ASM_IA64_UACCESS_H */
