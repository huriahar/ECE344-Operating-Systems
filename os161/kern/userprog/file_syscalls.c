#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>

/* 
	Error codes from the MAN pages:

	EBADF	fd is not a valid file descriptor, or was not opened for writing.
	EFAULT	Part or all of the address space pointed to by buf is invalid.
	ENOSPC	There is no free space remaining on the filesystem containing the file.
	EIO	A hardware I/O error occurred writing the data.
*/

int sys_write(int fd, int buf, size_t nbytes, int* retval){

    if(fd < STDIN_FILENO || fd > STDERR_FILENO){
        return EBADF;
    }

    if(buf == NULL){
        return EFAULT;
    }

    char* buff = kmalloc(sizeof(char) * (nbytes+1));

    if(buff == NULL){
        return ENOMEM;
    }

    copyin(buf, buff, nbytes);
    
    if(buff == NULL){
        return EFAULT;
    }

    buff[nbytes] = '\0';

    kprintf("%s", buff);

    kfree(buff);

    *retval = nbytes;
    return 0;
}


int sys_read(int fd, void *buf, size_t nbytes, int *retval){

    if(fd < STDIN_FILENO || fd > STDERR_FILENO){
        return EBADF;
    }

    if (nbytes == NULL) {
    	return EINVAL;
    }

    if (buf == NULL) {
    	return EFAULT;
    }

	*(char *)buf = getch();

	*retval = nbytes;
	return 0;
}