/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2


int cats_meals[NCATS] = {};
int mice_meals[NMICE] = {};
int containers[NFOODBOWLS] = {};

int cats_cnt;
int mice_cnt;

struct semaphore *cats_or_mice;
struct semaphore *mutex_cats_cnt;
struct semaphore *mutex_mice_cnt;

struct semaphore *cats;
struct semaphore *mice;
struct semaphore *bowls;


/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
sem_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}

/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        while(cats_meals[catnumber] < 4){

            P(mutex_cats_cnt);

            cats_cnt++;
            if(cats_cnt == 1)
               P(cats_or_mice);

            V(mutex_cats_cnt);

            P(cats);
            P(bowls);

            int bowl_used = (containers[0]==0)?0:1;
            containers[bowl_used] = 1;
            sem_eat("cat",catnumber,bowl_used+1,cats_meals[catnumber]);
            cats_meals[catnumber]++;
            containers[bowl_used] = 0;

            V(bowls);
            V(cats);

            P(mutex_cats_cnt);

            cats_cnt--;
            if(cats_cnt == 0)
                V(cats_or_mice);

            V(mutex_cats_cnt);
        }


        //(void) unusedpointer;
        //(void) catnumber;
}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */
        while(mice_meals[mousenumber] < 4){

            P(mutex_mice_cnt);

            mice_cnt++;
            if(mice_cnt == 1)
               P(cats_or_mice);

            V(mutex_mice_cnt);

            P(mice);
            P(bowls);

            int bowl_used = (containers[0]==0)?0:1;
            containers[bowl_used] = 1;
            sem_eat("mouse",mousenumber,bowl_used+1,mice_meals[mousenumber]);
            mice_meals[mousenumber]++;
            containers[bowl_used] = 0;

            V(bowls);
            V(mice);

            P(mutex_mice_cnt);

            mice_cnt--;
            if(mice_cnt == 0)
                V(cats_or_mice);

            V(mutex_mice_cnt);
        }
        //(void) unusedpointer;
        //(void) mousenumber;
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        cats_or_mice = sem_create("cats_or_mice",1);
        mutex_cats_cnt = sem_create("mutex_cats_cnt",1);
        mutex_mice_cnt = sem_create("mutex_mice_cnt",1);

        cats = sem_create("cats",NCATS);
        mice = sem_create("mice = ",NMICE);
        bowls = sem_create("bowls",NFOODBOWLS);

        cats_cnt = 0;
        mice_cnt = 0;
        //(void) nargs;
        //(void) args;
        kprintf("Starting thread forking\n");
        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {
                //kprintf("in catmousesem: forking cat index = %d\n", index);
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
                //kprintf("in catmousesem: forking mouse index = %d\n", index);
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        //sem_destroy(cats_or_mice);
        //sem_destroy(mutex_cats_cnt);
        //sem_destroy(mutex_mice_cnt);

        //sem_destroy(cats);
        //sem_destroy(mice);
        //sem_destroy(bowls);

        return 0;
}


/*
 * End of catsem.c
 */
