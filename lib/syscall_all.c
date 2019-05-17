#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>

extern char *KERNEL_SP;
extern struct Env *curenv;

/*
sysno 作为系统调用号被传入，现在起的更多是一个”占位“的作用，
能和之前用户层面的系统调用函数参数顺序相匹配。
*/
/* Overview:
 * 	This function is used to print a character on screen.
 * 
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area 
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this 
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current environment. This function will never return.
 */
// 实现用户进程对CPU的放弃
// 另外为了通过我们之前编写的进程切换机制保存现场，
// 这里需要在KERNEL_SP和TIMESTACK上做一点准备工作
void sys_yield(void)
{
	//类似于env_destroy，保存kernel_sp中的Trapframe，随后执行sched_yield;
	struct Trapframe *src = (struct Trapframe *)(KERNEL_SP - sizeof(struct Trapframe));
    struct Trapframe *dst = (struct Trapframe *)(TIMESTACK - sizeof(struct Trapframe));
    bcopy((void *)src, (void *)dst, sizeof(struct Trapframe));
	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a 
 * process, which is either a child of the caller of this function 
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 * 
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
    struct Env *env;
    int ret;
  
    if (envid2env(envid,&env,PTE_V) < 0) return -E_INVAL;
    // 使得发生COW类型缺页中断时可以在正确的栈里调用正确的处理函数。
    env->env_xstacktop = xstacktop;
    env->env_pgfault_handler = func;
 
    return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 * 分配一个页的内存，而且和va map起来。在envid的地址空间里。
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 * 如果一个页已经和va map了，这个页就unmap，作为一个副作用
 * 
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 * 前提条件
 * 
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 * 进程可能更改自己的地址空间或者他的子进程的地址空间
 */

/*  
	这个函数的主要功能是分配内存，简单的说，用户程序可以通过这个系统调用给该
	程序所允许的虚拟内存空间内存显式地分配实际的物理内存，需要用到一些我们之
	前在pmap.c 中所定义的函数
*/
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;
	ret = 0;

	// 首先满足基本条件
/* * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 * va must be < UTOP
 */
	/*if(!(perm & PTE_V) || (perm & PTE_COW) || va >= UTOP) return -E_INVAL;
	ret = envid2env(envid, &env, 1);
	if(ret != 0) return ret;
	ret = page_alloc(&ppage);
	if(ret != 0) return ret;
	ret = page_insert(env->env_pgdir, ppage, va, perm);
	if(ret != 0) return ret;
	return ret; // ret = 0;*/

	//首先满足基本条件，va不在范围
    if(va >= UTOP || va < 0) return -E_UNSPECIFIED;
    // PTE_V is required,PTE_COW is not allowed
    if ((perm & PTE_COW) || ((perm & PTE_V)==0) ) return -E_INVAL;
    if ((ret = envid2env(envid,&env,1))!=0) return ret;
    if ((ret = page_alloc(&ppage))!=0) return ret;
    //page_insert中已经隐含unmapping操作,无需我们再动手
    if ((ret = page_insert(env->env_pgdir,ppage,va,perm))!=0) return ret;
    ret = 0;
    return ret;

}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_mem_alloc.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 * 把源进程地址空间在srcva上的内存页map到在目标进程地址空间的dstva
 * 和 sys_mem_alloc 一样的限制条件。我们可能得加一个限制条件，这样你不能在只读的地方写。
 * 
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 * 不能access在UTOP上方的pages
 */

/*  
	将源进程地址空间中的相应内存映射到目标进程的
	相应地址空间的相应虚拟内存中去。换句话说，此
	时两者共享着一页物理内存。
*/

int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

/* * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 * va must be < UTOP
 */
	if (!(perm & PTE_V)) return -E_INVAL;
	// if ((perm & PTE_COW)) return -E_INVAL;

	// 地址非法
	if (srcva >= UTOP || dstva >= UTOP || srcva < 0 || dstva < 0) 
		return -E_UNSPECIFIED;

	// 两个进程必须都有效，而且必须是父子关系。
	// 如果send使用mem_map，这里checkperm不能为1
	ret = envid2env(srcid, &srcenv, 1);
	if(ret != 0) return -E_BAD_ENV;
	ret = envid2env(dstid, &dstenv, 1);
	if(ret != 0) return -E_BAD_ENV;

	// 获取srcva映射的page
	if ((ppage = page_lookup(srcenv -> env_pgdir, round_srcva, &ppte)) == 0) 
		return -E_UNSPECIFIED;
	// 企图从不可写映射到可写，返回错误
	if((ppte != NULL) && (((*ppte) & PTE_R) == 0) && ((perm & PTE_R) != 0)) 
		return -E_INVAL;
	ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm);
	if(ret != 0) return -E_NO_MEM;

	return ret;// ret = 0;

}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret = 0;
	struct Env *env;
	if(va >= UTOP || va < 0) return -E_INVAL;
	ret = envid2env(envid, &env, 0);
	if(ret != 0) return ret;
	// 直接调用page_remove的unmap功能
	page_remove(env->env_pgdir, va);
	return ret;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
int sys_env_alloc(void)
{
	// Your code here.
	int r;
	struct Env *e;

	if(env_alloc(&e, curenv->env_id) < 0) {
		return -E_NO_FREE_ENV;
	}

	e->env_status = ENV_NOT_RUNNABLE;
	// 把栈里的东西搬到新开的进程e的tf中
	// 从内核栈拷贝栈帧到进程控制块中
	bcopy((void*)KERNEL_SP - sizeof(struct Trapframe), &(e->env_tf), sizeof(struct Trapframe));
	e->env_tf.pc = e->env_tf.cp0_epc;
	e->env_tf.regs[2] = 0;
	e->env_pri = curenv->env_pri;

	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 * 
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// Your code here.
	struct Env *env;
	int ret;

	// Pre-Condition
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE && status != ENV_FREE) {
		return -E_INVAL;
	}
	if(envid2env(envid, &env, PTE_V) < 0) return -E_INVAL;

	env->env_status = status;
	// 启动子进程
	if(status==ENV_RUNNABLE) 
		LIST_INSERT_HEAD(&env_sched_list[0], env, env_sched_link);

	return 0;
	//	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{
	struct Env *env;
    int ret;
    if(ret=envid2env(envid,&env,0)) {
        return ret;
    }
    env->env_tf=*tf;

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`. 
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from 
 * other process. To be more specific, it will flag 
 * the current process so that other process could send 
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 * 
 * Post-Condition:
 * 	This syscall will set the current process's status to 
 * ENV_NOT_RUNNABLE, giving up cpu. 
 */
void sys_ipc_recv(int sysno, u_int dstva)
{
	if(dstva >= UTOP) return;

	// 首先要将env_ipc_recving 设置为1, 表明该进程准备接受其它进程的消息了
	curenv->env_ipc_recving = 1;
	// 之后阻塞当前进程，即将当前进程的状态置为不可运行
	curenv->env_status = ENV_NOT_RUNNABLE;
	// env_ipc_dstva 则说明了接收到的页需要被映射到哪个虚地址上
	curenv->env_ipc_dstva = dstva;
	LIST_REMOVE(curenv, env_sched_link);

	// 之后放弃CPU（调用相关函数重新进行调度）
	sys_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env.
 */
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)
{
	struct Env *e;
	struct Page *p;
	Pte *ppte;
	perm |= PTE_V;

	// 检查地址
	if (srcva >= UTOP || srcva < 0) return -E_INVAL;
	// 检查进程号是否正确
    if (envid2env(envid, &e, 0) < 0) return -E_INVAL;
    // 检查接收进程是否接收
    if (e->env_ipc_recving == 0) return -E_IPC_NOT_RECV;

	if (srcva != 0) {
	    p = page_lookup(curenv->env_pgdir, srcva, &ppte);
    	if((*ppte & PTE_R)==0 && (perm & PTE_R))
                printf("Permission Denied\n");
            if((r = page_insert(e->env_pgdir, p, e->env_ipc_dstva, perm) < 0)){
                printf("page insert failed in ipc_send");
                return r;
            }   
    }   
        
    e->env_ipc_perm = perm;
    e->env_ipc_recving = 0;
    e->env_status = ENV_RUNNABLE;
    e->env_ipc_value = value;
    e->env_ipc_from = curenv->env_id;
    LIST_INSERT_HEAD(&env_sched_list[0], e, env_sched_link);
    return 0;
}

/* Overview:
 * 	This function is used to write data to device, which is
 * 	represented by its mapped physical address.
 *	Remember to check the validity of device address (see Hint below);
 * 
 * Pre-Condition:
 *      'va' is the startting address of source data, 'len' is the
 *      length of data (in bytes), 'dev' is the physical address of
 *      the device
 * 	
 * Post-Condition:
 *      copy data from 'va' to 'dev' with length 'len'
 *      Return 0 on success.
 *	Return -E_INVAL on address error.
 *      
 * Hint: Use ummapped segment in kernel address space to perform MMIO.
 *	 Physical device address:
 *	* ---------------------------------*
 *	|   device   | start addr | length |
 *	* -----------+------------+--------*
 *	|  console   | 0x10000000 | 0x20   |
 *	|    IDE     | 0x13000000 | 0x4200 |
 *	|    rtc     | 0x15000000 | 0x200  |
 *	* ---------------------------------*
 */
int sys_write_dev(int sysno, u_int va, u_int dev, u_int len)
{
    // Your code here
        int flag = 0;
        if (dev >= 0x10000000 && dev + len <= 0x10000020) flag = 1;
        if (dev >= 0x13000000 && dev + len <= 0x13004200) flag = 1;
        if (dev >= 0x15000000 && dev + len <= 0x15000200) flag = 1;
        if (!flag) {
                return -E_INVAL;
        }
//        memcpy(dev + 0xa0000000, va, len);
         bcopy((void *)va, (void *)(0xa0000000 + dev), len);
        return 0;
}


/* Overview:
 * 	This function is used to read data from device, which is
 * 	represented by its mapped physical address.
 *	Remember to check the validity of device address (same as sys_read_dev)
 * 
 * Pre-Condition:
 *      'va' is the startting address of data buffer, 'len' is the
 *      length of data (in bytes), 'dev' is the physical address of
 *      the device
 * 
 * Post-Condition:
 *      copy data from 'dev' to 'va' with length 'len'
 *      Return 0 on success, < 0 on error
 *      
 * Hint: Use ummapped segment in kernel address space to perform MMIO.
 */

int sys_read_dev(int sysno, u_int va, u_int dev, u_int len)
{
    // Your code here
        int flag = 0;
        if (dev >= 0x10000000 && dev + len <= 0x10000020) flag = 1;
        if (dev >= 0x13000000 && dev + len <= 0x13004200) flag = 1;
        if (dev >= 0x15000000 && dev + len <= 0x15000200) flag = 1;
        if (!flag) {
                return -E_INVAL;
        }
//        memcpy(va, dev + 0xa0000000, len);
         bcopy((void *)(0xa0000000 + dev), (void *)va, len);
        return 0;
}





