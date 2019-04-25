#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

/* Overview:
 * 	`msyscall` push all the arguments into the stack, running
 * `syscall` instruction.
 * 
 * Pre-Condition:
 * 	The first, second, third and fourth arguments are passed
 * by registers(a0~a3). The remains are stored on the stack.
 *
 * Post-Condition:
 *	All arguments should be stored on the stack. Syscall number
 * should be passed by register v0.
 *
 * Hint:
 *	Interestingly, MIPS 32 ABI(application binary interface) defined that
 * allocating space, which shoud be large enough to contain all the arguments, 
 * on the stack is always required.
 *	So, we needn't allocate space on the stack again. In another word,
 * we shouldn't change the value of $sp. All we need to do is store 
 * registers(a0~a3) on the stack. 
 *  Remember passing syscall number by register v0 :)
 */

/* 把所有的参数都放进栈里。
* 前四个是被a0~a3传递的，剩下的被放在栈里。
* 所有的参数都应该被存进栈，syscall值被v0传递。
* mips32位机的二进制接口定义了，开辟空间应该足够大能够放得下所有的参数。
* 所以我们不应该改变sp的值，只需要将a0~a3放进栈。记得用v0传syscall的值。
*/


LEAF(msyscall)

    // TODO: execute a `syscall` instruction and return from msyscall

//    sw a0, 0(sp)	
//    sw a1, 4(sp)
//    sw a2, 8(sp)
//    sw a3, 12(sp)
    move v0, a0 			// ????? why

    syscall 

    jr ra

END(msyscall)