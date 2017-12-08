#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include <../../arch/mips/include/trapframe.h>
/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

 #define NUM_ARGS 10

int sys_reboot(int code);

int sys_write(int fd, int buf, size_t nbytes, int* retval);

int sys_getpid(int *retval);

void sys__exit(int exitcode);

int sys_fork(struct trapframe *tf, int *retval);

int sys_waitpid(int pid, int *status, int options, int *retval);

int sys_read(int fd, void *buf, size_t nbytes, int *retval);

void free_mem(char** kargs, int end);

int sys_execv(const char *program, char **args);

//int sys_execv(char* progname, char** args);

int sys_sbrk(intptr_t amount, int *retval);

#endif /* _SYSCALL_H_ */

