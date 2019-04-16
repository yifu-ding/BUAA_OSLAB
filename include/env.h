/* See COPYRIGHT for copyright information. */

#ifndef _ENV_H_
#define _ENV_H_

#include "types.h"
#include "queue.h"
#include "trap.h"
#include "mmu.h" 

#define LOG2NENV	10
#define NENV		(1<<LOG2NENV)
#define ENVX(envid)	((envid) & (NENV - 1)) 
/*envid & 11_1111_1111 即 envid的后10位，因为首位有个1*/
#define GET_ENV_ASID(envid) (((envid)>> 11)<<6)

// Values of env_status in struct Env
#define ENV_FREE	0
#define ENV_RUNNABLE		1
#define ENV_NOT_RUNNABLE	2

struct Env {
	struct Trapframe env_tf;        // Saved registers
	LIST_ENTRY(Env) env_link;       // Free list 
	/* env_link 的机制类似于实验二中的pp_link, 使用它和env_free_list来构造空闲进程链表。*/
	u_int env_id;                   // Unique environment identifier
	/* 每个进程的env_id 都不一样，env_id 是进程独一无二的标识符。*/
	u_int env_parent_id;            // env_id of this env's parent
	/* 该变量存储了创建本进程的进程id。这样进程之间通过父子进程之间的关联可以形成一颗进程树。*/
	u_int env_status;               // Status of the environment
	/* 该变量只能在以下三个值中进行取值：
  	– ENV_FREE : 表明该进程是不活动的，即该进程控制块处于进程空闲链表中。
  	– ENV_NOT_RUNNABLE : 表明该进程处于阻塞状态，处于该状态的进程往往在
  					     等待一定的条件才可以变为就绪状态从而被CPU 调度。
  	– ENV_RUNNABLE : 表明该进程处于就绪状态，正在等待被调度，但处于RUNNABLE
  				   	 状态的进程可以是正在运行的，也可能不在运行中。*/
	Pde  *env_pgdir;                // Kernel virtual address of page dir
	/* 进程页目录的虚拟地址。*/
	u_int env_cr3;
	/* 进程页目录的物理地址。*/
	LIST_ENTRY(Env) env_sched_link;
	/* 来构造就绪状态进程链表。*/
    u_int env_pri;
    /* 进程的优先级。*/
	// Lab 4 IPC
	u_int env_ipc_value;            // data value sent to us 
	u_int env_ipc_from;             // envid of the sender  
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;		// va at which to map received page
	u_int env_ipc_perm;		// perm of page mapping received

	// Lab 4 fault handling
	u_int env_pgfault_handler;      // page fault state
	u_int env_xstacktop;            // top of exception stack

	// Lab 6 scheduler counts
	u_int env_runs;			// number of times been env_run'ed
	u_int env_nop;                  // align to avoid mul instruction
};

LIST_HEAD(Env_list, Env);
extern struct Env *envs;		// All environments
extern struct Env *curenv;	        // the current env
extern struct Env_list env_sched_list[2]; // runnable env list

void env_init(void);
innenv_alloc(struct Env **e, u_int parent_id);
void env_free(struct Env *);
void env_create_priority(u_char *binary, int size, int priority);
void env_create(u_char *binary, int size);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Env *e);


// for the grading script
#define ENV_CREATE2(x, y) \
{ \
	extern u_char x[], y[]; \
	env_create(x, (int)y); \
}
#define ENV_CREATE_PRIORITY(x, y) \
{\
        extern u_char binary_##x##_start[]; \
        extern u_int binary_##x##_size;\
        env_create_priority(binary_##x##_start, \
                (u_int)binary_##x##_size, y);\
}
#define ENV_CREATE(x) \
{ \
	extern u_char binary_##x##_start[];\
	extern u_int binary_##x##_size; \
	env_create(binary_##x##_start, \
		(u_int)binary_##x##_size); \
}

#endif // !_ENV_H_