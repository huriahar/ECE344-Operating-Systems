#ifndef _KERN_LIMITS_H_
#define _KERN_LIMITS_H_

/* Longest filename (without directory) not including null terminator */
#define NAME_MAX  255

/* Longest full path name */
#define PATH_MAX   1024

/* minimum PID available for processes. 0 --> child process */
#define PID_MIN 1

#define MAX_PROCESSES 128

#endif /* _KERN_LIMITS_H_ */
