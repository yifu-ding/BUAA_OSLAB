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

void sched_yield(void)
{
    int i;
    static int pos = 0;
    static int times = 0;
    static struct Env *e;
    while (--times<=0 || curenv == NULL || curenv->env_status!=ENV_RUNNABLE) {
        if(LIST_EMPTY(&env_sched_list[pos])) {
            pos = 1 - pos;
        }
        e = LIST_FIRST(&env_sched_list[pos]);
        if(e == NULL)
            continue;
        else {
            LIST_REMOVE(e, env_sched_link);
            LIST_INSERT_HEAD(&env_sched_list[1-pos], e, env_sched_link);
            times = e->env_pri;
            break;
        }
    }
    env_run(e);
}



/*void sched_yield(void)
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
  /*  if(curenv == NULL || count >= curenv->env_pri) {
        // 如果不是第一次运行进程，则要将当前进程添加到另一个待调度队列中以便下一次调度
        if(curenv != NULL) {
            LIST_INSERT_HEAD(&env_sched_list[1 - t], curenv, env_sched_link);
        }
        int flag = 0;
        while(1) {
            struct Env *e = LIST_FIRST(&env_sched_list[t]);
            // 若当前进程列表没有可以调度的进程，就换一个链表
            if(e == NULL) {
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
*/

/*我们在这里将优先级设置为时间片大小: 1 表示1 个时间片长度, 2 表示2 个时间片长度，以此类推。
不过寻找就绪状态进程不是简单遍历进程链表, 而是用两个链表存储所有就绪状态进程。
每当一个进程状态变为ENV_RUNNABLE , 我们要将其插入第一个就绪状态进程链表。
调用sched_yield 函数时, 先判断当前时间片是否用完。
如果用完,将其插入另一个就绪状态进程链表。
之后判断当前就绪状态进程链表是否为空。如果为空, 切换到另一个就绪状态进程链表。
*/