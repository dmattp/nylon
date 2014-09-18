#ifndef LINUX_WEAVE_SYSPORT_ATOMIC_H__
#define LINUX_WEAVE_SYSPORT_ATOMIC_H__

namespace Atomic {

////// #if defined(__x86_64__)
////// #define CFG_SMP_X86_64 1
////// #endif 
////// 
////// #if CFG_SMP_X86_64
////// 
////// # define rmb()	asm volatile("lfence":::"memory")
////// # define wmb()	asm volatile("sfence" ::: "memory")
////// # define MEMBARRIER() do { wmb(); rmb(); } while( 0 )
//////    
////// /* for 64bit/multicore platforms */
////// # define LOCK_PREFIX                             \
////// 		".section .smp_locks,\"a\"\n"             \
////// 		"  .align 8\n"                            \
////// 		"  .quad 661f\n" /* address */            \
////// 		".previous\n"                             \
////// 	       	"661:\n\tlock; "
////// #else
////// 
////// # define rmb()	asm volatile("lfence":::"memory")
////// # define wmb()	asm volatile("sfence" ::: "memory")
////// # define MEMBARRIER() do { wmb(); rmb(); } while( 0 )
//////    
////// /* for 32bit/multicore platforms */
////// #if 0
////// // dmp/12.1023: The use of .smp_locks section is causing
////// // complaints from the linker.  I'm not sure what the
////// // purpose of the use of a section is here.
////// # define LOCK_PREFIX                             \
////// 		".pushsection .smp_locks,\"a\"\n"             \
//////       "  .align 4\n" \
////// 		"  .long 661f\n" /* address */            \
////// 		".popsection\n"                             \
////// 	       	"661:\n\tlock; "
////// #else
////// # define LOCK_PREFIX "\n\tlock; "
////// #endif
//////    
////// #endif  // CFG_SMP_X86_64
////// 
//////    static inline unsigned
////// hpl_cmpxchg__cpp__
////// (
//////    volatile long *ptr,
//////    long old,
//////    long newval
////// )
////// {
////// 	long prev;
//////    __asm__ __volatile__( LOCK_PREFIX "cmpxchgl %1,%2"
//////       : "=a"(prev)
//////       : "r"(newval), "m"(*ptr), "0"(old)
//////       : "memory");
//////    return prev;
////// }
//////    
////// 
////// #  if CFG_SMP_X86_64
////// static inline  long long
////// hpl_cmpxchg__cpp__
////// (
//////    volatile  long long *ptr,
//////     long long old,
//////     long long newval
////// )
////// {
////// 	 long long prev;
//////  		__asm__ __volatile__( LOCK_PREFIX "cmpxchgq %1,%2"       \
//////       : "=a"(prev)                                       \
//////       : "r"(newval), "m"(*ptr), "0"(old)    \
//////       : "memory");
//////    return prev;
////// }
////// #  endif // if CFG_SMP_X86_64 (adds 8byte atomic cmpxchg)
////// 
   
    long nylon_sysport_cmpxchg( volatile long* var, long oldval, long newval )
    {
        return __sync_val_compare_and_swap( var, oldval, newval );
    }

    long long nylon_sysport_cmpxchg( volatile long long* var, long long oldval, long long newval )
    {
        return __sync_val_compare_and_swap( var, oldval, newval );
    }
}

      
#endif // not defined LINUX_WEAVE_SYSPORT_ATOMIC_H__
