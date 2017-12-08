/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char** args, int arg_num)
{
	int ret;
	int original_arg_num = arg_num;
	vaddr_t kargs[arg_num];

	// Step 1 - Open and copy the address space and activate it 
	struct vnode *v;
	/* Open the file. */
	ret = vfs_open(progname, O_RDONLY, &v);
	if (ret) {
		return ret;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	// Step 2- Load the executable file
	vaddr_t entrypoint;
	/* Load the executable. */
	ret = load_elf(v, &entrypoint);
	if (ret) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return ret;
	}

	/* Done with the file now. */
	vfs_close(v);
	vaddr_t stackptr;
	/* Define the user stack in the address space */
	ret = as_define_stack(curthread->t_vmspace, &stackptr);
	if (ret) {
		/* thread_exit destroys curthread->t_vmspace */
		return ret;
	}

	// Copy the args on the stack in reverse order (a3, a2...) with correct padding such 
	// that the stackpointer addresses are aligned by multiples of 4
	size_t official_length;
	if (args != NULL ) {
		arg_num = arg_num-1;
		while (arg_num >= 0) {
	        //char *arg;
	        int arg_len = strlen(args[arg_num]);
	        arg_len++;
	        int original_arg_len = arg_len;
	        int rem = arg_len%4; // number of '\0' padding to align addresses with multiples of 4
	        
	        if (rem > 0)
	        	arg_len += (4 - rem);

            stackptr -= arg_len;
	        ret = copyoutstr(args[arg_num], (userptr_t)stackptr, (size_t)original_arg_len, &official_length);

	        if (ret != 0) {
	        	return ret;
	        }

	        kargs[arg_num] = stackptr;
	        arg_num--;
	    }
	    kargs[original_arg_num] = NULL;

        stackptr -= 4*sizeof(char);	

	    int i;
	    for (i = (original_arg_num - 1); i >= 0; i--) {
	        stackptr = stackptr - 4;
	        ret = copyout(&kargs[i], (userptr_t)stackptr, 4);
	        if (ret) {
	            return ret;           
	        }
	    }

		/* Warp to user mode. */
		md_usermode(original_arg_num, (userptr_t)stackptr, stackptr, entrypoint);
	}
	else
		md_usermode(0, NULL, stackptr, entrypoint);
	
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}
