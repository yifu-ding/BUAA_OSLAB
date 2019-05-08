#include "lib.h"
#include <mmu.h>
#include <env.h>

/* ----------------- help functions ---------------- */

/* Overview:
 *      Copy `len` bytes from `src` to `dst`.
 *
 * Pre-Condition:
 *      `src` and `dst` can't be NULL. Also, the `src` area 
 *       shouldn't overlap the `dest`, otherwise the behavior of this 
 *       function is undefined.
 */
void user_bcopy(const void *src, void *dst, size_t len)
{
        void *max;

        //      writef("~~~~~~~~~~~~~~~~ src:%x dst:%x len:%x\n",(int)src,(int)dst,len);
        max = dst + len;

        // copy machine words while possible
        if (((int)src % 4 == 0) && ((int)dst % 4 == 0)) {
                while (dst + 3 < max) {
                        *(int *)dst = *(int *)src;
                        dst += 4;
                        src += 4;
                }
        }

        // finish remaining 0-3 bytes
        while (dst < max) {
                *(char *)dst = *(char *)src;
                dst += 1;
                src += 1;
        }

        //for(;;);
}

/* Overview:
 *      Sets the first n bytes of the block of memory 
 * pointed by `v` to zero.
 * 
 * Pre-Condition:
 *      `v` must be valid.
 *
 * Post-Condition:
 *      the content of the space(from `v` to `v`+ n) 
 * will be set to zero.
 */
void user_bzero(void *v, u_int n)
{
        char *p;
        int m;

        p = v;
        m = n;

        while (--m >= 0) {
                *p++ = 0;
        }
}
/*--------------------------------------------------------------*/


/* Overview:
 *      Custom page fault handler - if faulting page is copy-on-write,
 * map in our own private writable copy.
 * 
 * Pre-Condition:
 *      `va` is the address which leads to a TLBS exception.
 *
 * Post-Condition:
 *  Launch a user_panic if `va` is not a copy-on-write page.
 * Otherwise, this handler should map a private writable copy of 
 * the faulting page at correct address.
 */
static void
pgfault(u_int va) // 把va这一页拷贝到temp上，写时复制
{
    u_int *temp;
        //      writef("fork.c:pgfault():\t va:%x\n",va);
    va = ROUNDDOWN(va,BY2PG);
    
    temp = UXSTACKTOP-2*BY2PG;// 第一个BY2PG用于存TrapFrame了

    u_int perm = (*vpt)[VPN(va)]& 0xfff;
    
    //writef("fork.c:pgfault():\t va:%x\n",va);
    
    if(perm & PTE_COW){
        
        //map the new page at a temporary place
        if(syscall_mem_alloc(syscall_getenvid(),temp,perm & (~PTE_COW))<0)
        {
            user_panic("sys_mem_alloc error.\n");
        }
        
            //copy the content
        user_bcopy((void *)va,(void *)temp,BY2PG);
        
        //map the page on the appropriate place
        // temp和va两个虚拟地址映射到temp的物理页。改权限，不能写时复制。
        if(syscall_mem_map(syscall_getenvid(),temp,syscall_getenvid(),va,perm & (~PTE_COW))<0)
        {
            user_panic("sys_mem_map error.\n");
        }
        
        //unmap the temporary place，让temp抛弃这个页，temp本来就是中转页。
        if(syscall_mem_unmap(syscall_getenvid(),temp)<0)
        {
            user_panic("sys_mem_unmap error.\n");
        }
    }
    else
    {
        user_panic("Maximum Limit for ENV Exceeded\n");
    }
}

/* Overview:
 *      Map our virtual page `pn` (address pn*BY2PG) into the target `envid`
 * at the same virtual address. 
 *
 * Post-Condition:
 *  if the page is writable or copy-on-write, the new mapping must be 
 * created copy on write and then our mapping must be marked 
 * copy on write as well. In another word, both of the new mapping and
 * our mapping should be copy-on-write if the page is writable or 
 * copy-on-write.
 * 
 * Hint:
 *      PTE_LIBRARY indicates that the page is shared between processes.
 * A page with PTE_LIBRARY may have PTE_R at the same time. You
 * should process it correctly.
 */
static void
duppage(u_int envid, u_int pn)
{
        u_int addr;
        u_int perm;
    
    addr = pn * BY2PG;
    perm = (*vpt)[pn] & 0xfff; // 提取之前的有效位

    if(((perm & PTE_R) != 0) && (perm & PTE_V))
    {
        // 有效，而且不是只读
        if(perm & PTE_LIBRARY) {
            // 共享可写
            perm = PTE_V | PTE_R | PTE_LIBRARY | perm;
        }
        else {
            // 不是共享可写，子进程要写必须复制一份，所以 PTE_COW
            perm = PTE_V | PTE_R | PTE_COW | perm;
        }
        // 复制父进程的页给子进程，并修改双方的权限（按情况增加COW)
        if(syscall_mem_map(syscall_getenvid(), addr, envid, addr, perm) < 0) {
            writef("%x\n", addr);
            user_panic("sys_mem_map for son failed.\n");
        }
        if(syscall_mem_map(syscall_getenvid(), addr, syscall_getenvid(), addr, perm) < 0)
        { 
            user_panic("sys_mem_map for father failed.\n");
        }
    }
    else
    {
        // 只读或者无效，保留自己的perm
        if(syscall_mem_map(syscall_getenvid(), addr, envid, addr, perm) < 0){
            user_panic("sys_mem_map for son failed.1\n");
        
        }
    }
    
        //      user_panic("duppage not implemented");
} 

/* Overview:
 *      User-level fork. Create a child and then copy our address space
 * and page fault handler setup to the child.
 *
 * Hint: use vpd, vpt, and duppage.
 * Hint: remember to fix "env" in the child process!
 * Note: `set_pgfault_handler`(user/pgfault.c) is different from 
 *       `syscall_set_pgfault_handler`. 
 */ 
extern void __asm_pgfault_handler(void);
int
fork(void)
{
    // Your code here.
    u_int newenvid;
    extern struct Env *envs;
    extern struct Env *env; // 子进程
    u_int i;
    u_int parent_id = syscall_getenvid(); // 这是父进程
    

        //The parent installs pgfault using set_pgfault_handler
    set_pgfault_handler(pgfault);

        //alloc a new alloc，这是子进程
    if((newenvid = syscall_env_alloc()) == 0)
    {
        // 子进程进入到这个if，然后找到自己是谁。
        env = &envs[ENVX(syscall_getenvid())];
        env->env_parent_id = parent_id;
        return 0;   // 子进程 return 0
    }
  

    for(i = 0;i < USTACKTOP;i += BY2PG)
    {
        // VPN() = PPN(va)  // 页目录的入口  // 页表的入口
        if(((*vpd)[VPN(i)/1024]) != 0 && ((*vpt)[VPN(i)]) != 0)
        {
            duppage(newenvid, VPN(i));
        }
        if(((*vpd)[VPN(i)/1024]) == 0) {
            i += 1023 * BY2PG;
        }
    }

    // 为子进程申请一页（映射到UXSTACKTOP下面那个BY2PG）作为异常处理栈。
    if(syscall_mem_alloc(newenvid, UXSTACKTOP-BY2PG, PTE_V | PTE_R) < 0){
        user_panic("failed alloc UXSTACK.\n");
        return 0;
    }

    // 为子进程注册pgfault_handler
    if(syscall_set_pgfault_handler(newenvid, __asm_pgfault_handler, UXSTACKTOP) < 0){
        user_panic("page fault handler setup failed.\n");
        return 0;
    }
    
    // 唤醒孩子
    syscall_set_env_status(newenvid, ENV_RUNNABLE);


    //writef("ENV_ID: %d\n", newenvid);

        return newenvid;
} 

// Challenge!
int
sfork(void)
{
        user_panic("sfork not implemented");
        return -E_INVAL;
}
