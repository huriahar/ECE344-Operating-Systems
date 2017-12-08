/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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
 * Number of cars created.
 */

#define NCARS 20


/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

enum {N, E, S, W};

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

struct lock *NW, *NE, *SW, *SE, *num_cars_lock;
struct cv *num_cars_cv;
int num_cars;

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long carstart,
           unsigned long carnumber,
           unsigned long cardestination)
{
        /*
         * Avoid unused variable warnings.
         */
        
        //(void) cardirection;
        //(void) carnumber;

        if(carstart == N && cardestination == S){
            lock_acquire(NW);
            message(REGION1, carnumber, carstart, cardestination);
            
            lock_acquire(SW);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(NW);

            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(SW);            
        }
        else if(carstart == S && cardestination == N){
            lock_acquire(SE);
            message(REGION1, carnumber, carstart, cardestination);

            lock_acquire(NE);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(SE);

            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(NE);            
        }
        else if(carstart == E && cardestination == W){
            lock_acquire(NE);
            message(REGION1, carnumber, carstart, cardestination);

            lock_acquire(NW);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(NE);

            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(NW);            
        }
        else if(carstart == W && cardestination == E){
            lock_acquire(SW);
            message(REGION1, carnumber, carstart, cardestination);

            lock_acquire(SE);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(SW);

            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(SE);
        }

}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long carstart,
           unsigned long carnumber,
           unsigned long cardestination)
{
        /*
         * Avoid unused variable warnings.
         */

        //(void) cardirection;
        //(void) carnumber;

        if(carstart == N && cardestination == E){
            lock_acquire(NW);
            message(REGION1, carnumber, carstart, cardestination);

            lock_acquire(SW);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(NW);

            lock_acquire(SE);
            message(REGION3, carnumber, carstart, cardestination);

            lock_release(SW);

            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(SE);  
        }
        else if(carstart == W && cardestination == N){
            lock_acquire(SW);
            message(REGION1, carnumber, carstart, cardestination);

            lock_acquire(SE);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(SW);

            lock_acquire(NE);
            message(REGION3, carnumber, carstart, cardestination);

            lock_release(SE);
            
            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(NE);  
        }
        else if(carstart == S && cardestination == W){
            lock_acquire(SE);
            message(REGION1, carnumber, carstart, cardestination);
            
            lock_acquire(NE);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(SE);
            
            lock_acquire(NW);
            message(REGION3, carnumber, carstart, cardestination);

            lock_release(NE);
            
            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(NW);  
        }
        else if(carstart == E && cardestination == S){
            lock_acquire(NE);
            message(REGION1, carnumber, carstart, cardestination);
            
            lock_acquire(NW);
            message(REGION2, carnumber, carstart, cardestination);

            lock_release(NE);
            
            lock_acquire(SW);
            message(REGION3, carnumber, carstart, cardestination);

            lock_release(NW);
            
            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(SW);  
        }
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long carstart,
           unsigned long carnumber,
           unsigned long cardestination)
{
        /*
         * Avoid unused variable warnings.
         */

        //(void) cardirection;
        //(void) carnumber;

        if(carstart == N && cardestination == W){
            lock_acquire(NW);
            message(REGION1, carnumber, carstart, cardestination);
            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(NW);

        }
        else if(carstart == W && cardestination == S){
            lock_acquire(SW);
            message(REGION1, carnumber, carstart, cardestination);
            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(SW);
        }
        else if(carstart == S && cardestination == E){
            lock_acquire(SE);
            message(REGION1, carnumber, carstart, cardestination);
            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(SE);
        }
        else if(carstart == E && cardestination == N){
            lock_acquire(NE);
            message(REGION1, carnumber, carstart, cardestination);
            message(LEAVING, carnumber, carstart, cardestination);
            lock_release(NE);
        }
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int carstart, cardestination;

        /*
         * Avoid unused variable and function warnings.
         */
         /*
        (void) unusedpointer;
        (void) carnumber;
    	(void) gostraight;
    	(void) turnleft;
    	(void) turnright;
        */
        /*
         * carstart is set randomly. so is cardestination
         */

        carstart = random() % 4;
        cardestination = random() % 4;

        while(cardestination==carstart) {
            cardestination = random() % 4;
        }

        message(APPROACHING, carnumber, carstart, cardestination);

        //to avoid deadlocks, cannot have more than 3 cars in the intersection at once

        lock_acquire(num_cars_lock);
        
        while(num_cars >= 3){
            cv_wait(num_cars_cv, num_cars_lock);
        }
        num_cars++;

        lock_release(num_cars_lock);

        // determine car direction

        int absdirection = (carstart>cardestination) ? (carstart-cardestination) : (cardestination-carstart);
        
        if(absdirection == 2)
            gostraight(carstart, carnumber, cardestination);
        else if((carstart+1)%4 == cardestination)
            turnleft(carstart, carnumber, cardestination);
        else if((cardestination+1)%4 == carstart)
            turnright(carstart, carnumber, cardestination);

        lock_acquire(num_cars_lock);
        
        num_cars--;
        if(num_cars_cv->num_wait_threads)
            cv_signal(num_cars_cv, num_cars_lock);

        lock_release(num_cars_lock);

}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        NW = lock_create("NW");
        NE = lock_create("NE");
        SW = lock_create("SW");
        SE = lock_create("SE");
        
        num_cars_lock = lock_create("num_cars_lock");
        num_cars_cv = cv_create("num_cars_cv");

        num_cars = 0;

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }

        return 0;
}
