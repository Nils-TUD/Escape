/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/cpu.h>
#include <esc/sllist.h>
#include <assert.h>

void thread_switchTo(tTid tid) {
	sThread *cur = thread_getRunning();
	/* finish kernel-time here since we're switching the process */
	if(tid != cur->tid) {
		uint64_t kcstart = cur->stats.kcycleStart;
		if(kcstart > 0) {
			uint64_t cycles = cpu_rdtsc();
			cur->stats.kcycleCount.val64 += cycles - kcstart;
		}

		if(!thread_save(&cur->save)) {
			sThread *old;
			sThread *t = thread_getById(tid);
			vassert(t != NULL,"Thread with tid %d not found",tid);

			/* mark old process ready, if it should not be blocked, killed or something */
			if(cur->state == ST_RUNNING)
				sched_setReady(cur);

			old = cur;
			thread_setRunning(t);
			cur = t;

			/* set used */
			cur->stats.schedCount++;
			vmm_setTimestamp(cur,timer_getTimestamp());
			sched_setRunning(cur);

			thread_resume(cur->proc->pagedir,&cur->save,
					sll_length(cur->proc->threads) > 1 ? cur->kstackFrame : 0);
		}

		/* now start kernel-time again */
		cur = thread_getRunning();
		cur->stats.kcycleStart = cpu_rdtsc();
	}

	thread_killDead();
}
