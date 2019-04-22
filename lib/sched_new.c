#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *  Search through 'envs' for a runnable environment ,
 *  in circular fashion statrting after the previously running env,
 *  and switch to the first such environment found.
 *
 * Hints:
 *  The variable which is for counting should be defined as 'static'.
 */
void sched_yield_new(void)
{

	// 记录当前进程已经使用的时间片数目
    static int count = 0;
    
    static int t = 0;
    count++;
    /*
     * 切换进程的条件
     * 1. 当前进程时NULL，这种情况只发生在运行第一个进程的时候
     * 2. 当前进程的时间片已经用完了
    */
    if(curenv == NULL || count >= curenv->env_pri) {
		if(curenv != NULL) {
			curenv->env_pri--;
			if(curenv->env_pri == 0){
				env_destroy(curenv);
				return; // ???????
			}
		}
        // 如果不是第一次运行进程，则要将当前进程添加到另一个待调度队列中以便下一次调度
        if(curenv != NULL) {
            LIST_INSERT_HEAD(&env_sched_list[1 - t], curenv, env_sched_link);
        }
		int flag = 0;
        while(1) {
            struct Env *e = LIST_FIRST(&env_sched_list[t]);
            // 若当前进程列表没有可以调度的进程，就换一个链表
			if(e == NULL){
				if(flag == 0) flag = 1;
				else return;
				t = 1 - t;
                continue;
            }
            // 若找到一个可以执行的进程
            if(e->env_status == ENV_RUNNABLE){
                // 从待调度队列中移出
                LIST_REMOVE(e,env_sched_link);
                // 初始化已使用的时间片个数
                count = 0;
                // 运行找到的这个进程e，当成当前进程
                env_run(e);
                break;
            }
        }
    } else {
		env_run(curenv);
	}
}