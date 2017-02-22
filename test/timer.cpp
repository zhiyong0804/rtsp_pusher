/* 
 * This file provides an interface of timers with signal and list
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/queue.h>

#include "timer.h"

#define MAX_TIMER_NUM		1000	/**< max timer number	*/
#define TIMER_START 	       1	/**< timer start(second)*/
#define TIMER_TICK 		       1	/**< timer tick(second)	*/
#define INVALID_TIMER_ID 	(-1)	/**< invalid timer ID	*/
#define TIMER_TICK_MS       1000    /**< timer tick(microsecond) */


/**
 * The type of the timer
 */
struct timer {
	LIST_ENTRY(timer) entries;	    /**< list entry		*/	
	
	timer_id           id;			/**< timer id		*/

	int          interval;			/**< timer interval(microsecond)*/
	int            elapse; 			/**< 0 -> interval 	*/

	timer_expiry*      cb;		    /**< call if expiry 	*/
	void*       user_data;		    /**< callback arg	*/
	int               len;		    /**< user_data length	*/
};

/**
 * The timer list
 */
struct timer_list {
	LIST_HEAD(listheader, timer) header;	/**< list header 	*/
	int num;				/**< timer entry number */
	int max_num;				/**< max entry number	*/

	void (*old_sigfunc)(int);		/**< save previous signal handler */
	void (*new_sigfunc)(int);		/**< our signal handler	*/

	struct itimerval ovalue;		/**< old timer value */
	struct itimerval value;			/**< our internal timer value */
};

static struct timer_list timer_list;

static void sig_func(int signo);

int init_timer(int count)
{
	int ret = 0;
	
	if(count <=0 || count > MAX_TIMER_NUM) {
		return -1;
	}
	
	memset(&timer_list, 0, sizeof(struct timer_list));
	LIST_INIT(&timer_list.header);
	timer_list.max_num = count;	

	/* Register our internal signal handler and store old signal handler */
	if ((timer_list.old_sigfunc = signal(SIGALRM, sig_func)) == SIG_ERR) {
		return -1;
	}
	timer_list.new_sigfunc = sig_func;

	/* Setting our interval timer for driver our mutil-timer and store old timer value */
	timer_list.value.it_value.tv_sec = TIMER_START; //TIMER_START;
	timer_list.value.it_value.tv_usec = 0;
	timer_list.value.it_interval.tv_sec = 0; //TIMER_TICK;
	timer_list.value.it_interval.tv_usec = TIMER_TICK_MS;
	ret = setitimer(ITIMER_REAL, &timer_list.value, &timer_list.ovalue);

	return ret;
}

int destroy_timer(void)
{
	struct timer *node = NULL;
	
	if ((signal(SIGALRM, timer_list.old_sigfunc)) == SIG_ERR) {
		return -1;
	}

	if((setitimer(ITIMER_REAL, &timer_list.ovalue, &timer_list.value)) < 0) {
		return -1;
	}
	
	while (!LIST_EMPTY(&timer_list.header)) {/* Delete. */
		node = LIST_FIRST(&timer_list.header);
	    LIST_REMOVE(node, entries);
		free(node->user_data);
		free(node);
	}
	
	memset(&timer_list, 0, sizeof(struct timer_list));

	return 0;
}

timer_id add_timer(int interval, timer_expiry *cb, void *user_data, int len)
{
	struct timer *node = NULL;	

	if (cb == NULL || interval <= 0) {
		return INVALID_TIMER_ID;
	}

	if(timer_list.num < timer_list.max_num) {
		timer_list.num++;
	} else {
		return INVALID_TIMER_ID;
	} 
	
	if((node = static_cast<timer*>(malloc(sizeof(struct timer)))) == NULL) {
		return INVALID_TIMER_ID;
	}
	if(user_data != NULL || len != 0) {
		node->user_data = user_data;//malloc(len);
		//memcpy(node->user_data, user_data, len);
		node->len = len;
	}

	node->cb = cb;
	node->interval = interval;
	node->elapse = 0;
	node->id = timer_list.num;
	
	LIST_INSERT_HEAD(&timer_list.header, node, entries);
	
	return node->id;
}

int del_timer(timer_id id)
{
	if (id <0 || id > timer_list.max_num) {
		return -1;
	}
			
	struct timer *node = timer_list.header.lh_first;
	for ( ; node != NULL; node = node->entries.le_next) {
		if (id == node->id) {
			LIST_REMOVE(node, entries);
			timer_list.num--;
			//free(node->user_data);
			free(node);
			return 0;
		}
	}
	
	/* Can't find the timer */
	return -1;
}

/* Tick Bookkeeping */
static void sig_func(int signo)
{
	struct timer *node = timer_list.header.lh_first;
	for ( ; node != NULL; node = node->entries.le_next) {
		node->elapse++;
		if(node->elapse >= node->interval) {
			node->elapse = 0;
			node->cb(node->id, node->user_data, node->len);
		}
	}
}

static char *fmt_time(char *tstr)
{
	time_t t;

	t = time(NULL);
	strcpy(tstr, ctime(&t));
	tstr[strlen(tstr)-1] = '\0';

	return tstr;
}
