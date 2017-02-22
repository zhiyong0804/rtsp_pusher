#ifndef NS_PROXYER_TIMER_H_
#define NS_PROXYER_TIMER_H_



typedef int timer_id;


/**
 * The type of callback function to be called by timer scheduler when a timer
 * has expired.
 *
 * @param id		The timer id.
 * @param user_data     The user data.
 * $param len		The length of user data.
 */
typedef int timer_expiry(timer_id id, void *user_data, int len);

/**
 * Create a timer list.
 *
 * @param count    The maximum number of timer entries to be supported 
 *			initially.  
 *
 * @return         0 means ok, the other means fail.
 */
int init_timer(int count);

/**
 * Destroy the timer list.
 *
 * @return          0 means ok, the other means fail.
 */
int destroy_timer(void);

/**
 * Add a timer to timer list.
 *
 * @param interval  The timer interval(millisecond).  
 * @param cb  	    When cb!= NULL and timer expiry, call it.  
 * @param user_data Callback's param.  
 * @param len  	    The length of the user_data.  
 *
 * @return          The timer ID, if == INVALID_TIMER_ID, add timer fail.
 */
timer_id add_timer(int interval, timer_expiry *cb, void *user_data, int len);

/**
 * Delete a timer from timer list.
 *
 * @param id  	    The timer ID.  
 * @return          0 means ok, the other fail.
 */
int del_timer(timer_id id);

#endif