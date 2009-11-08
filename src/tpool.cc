#include <stdio.h>
#include <string.h>
#include "tpool.h"
#include "timer.h"

Task::Task()
{
	closure = 0;
	proc = done = 0;
}

Task::Task(void (*proc)(void*), void (*done)(void*), void *cls)
{
	closure = cls;
	this->proc = proc;
	this->done = done;
}

struct ThreadData {
	ThreadPool *tpool;
	int tid;
};


// ---- thread pool class ----
ThreadPool::ThreadPool()
{
	num_tasks = 0;

	pthread_cond_init(&work_pending_cond, 0);
	pthread_mutex_init(&work_pending_mutex, 0);

	work_left = 0;
	pthread_cond_init(&done_cond, 0);
	//pthread_mutex_init(&done_mutex, 0);

	// start the timer if we haven't done so already to avoid getting 0 msec later on
	get_msec();
}

ThreadPool::~ThreadPool()
{
	if(threads) {
		stop();
	}
}

bool ThreadPool::start(int num_threads)
{
	stopping = false;

	if(!num_threads) {
		num_threads = get_number_processors();
	}

	threads = new pthread_t[num_threads];
	stats = new ThreadStats[num_threads];
	this->num_threads = num_threads;

	memset(stats, 0, num_threads * sizeof *stats);

	for(int i=0; i<num_threads; i++) {
		ThreadData *data = new ThreadData;
		data->tpool = this;
		data->tid = i;

		int res = pthread_create(&threads[i], 0, worker_thread_func, data);
		if(res != 0) {
			fprintf(stderr, "failed to create thread: %s\n", strerror(res));
			return false;
		}
	}
	return true;
}

void ThreadPool::stop()
{
	stopping = true;
	clear_work();
	pthread_cond_broadcast(&work_pending_cond);	// just to wake them up

	for(int i=0; i<num_threads; i++) {
		ThreadData *data_ptr = 0;
		pthread_join(threads[i], (void**)&data_ptr);
		delete data_ptr;

		/*unsigned long total = get_msec() - stats[i].start_time;

		printf("-- thread %d stats --\n", i);
		printf("  tasks: %lu\n", stats[i].tasks);
		printf("  time total: %lu, proc: %lu, idle: %lu\n", total, stats[i].proc_time,
				stats[i].idle_time);*/
	}

	delete [] threads;
	delete [] stats;
	threads = 0;
	stats = 0;
	num_threads = 0;
}

/* inlined and moved to tpool.h
int ThreadPool::get_num_threads() const
{
	return num_threads;
}

const ThreadStats *ThreadPool::get_thread_stats(int tid) const
{
	return stats + tid;
}
*/

bool ThreadPool::add_work(const Task *tasks, int count)
{
	if(!count) return false;

	pthread_mutex_lock(&work_pending_mutex);
	num_tasks += count;
	work_left += count;

	for(int i=0; i<count; i++) {
		workq.push(tasks[i]);
	}

	// wake up all worker threads
	pthread_cond_broadcast(&work_pending_cond);
	pthread_mutex_unlock(&work_pending_mutex);

	return true;
}

void ThreadPool::clear_work()
{
	pthread_mutex_lock(&work_pending_mutex);

	while(!workq.empty()) {
		workq.pop();
	}
	
	work_left -= num_tasks;
	num_tasks = 0;
	
	pthread_mutex_unlock(&work_pending_mutex);
}

// waits for all the work to be completed
void ThreadPool::wait_work()
{
	pthread_mutex_lock(&work_pending_mutex);
	while(work_left > 0) {
		pthread_cond_wait(&done_cond, &work_pending_mutex);
	}
	pthread_mutex_unlock(&work_pending_mutex);
}

/* inlined and moved to tpool.h
bool ThreadPool::is_done() const
{
	return work_left == 0;
}
*/

// this is called by the worker thread when a task is finished
void ThreadPool::finish_task(const Task &task)
{
	// if the task has a done callback, call it
	if(task.done) {
		task.done(task.closure);
	}

	pthread_mutex_lock(&work_pending_mutex);
	if(!--work_left) {
		pthread_cond_signal(&done_cond);
	}
	pthread_mutex_unlock(&work_pending_mutex);
}

// this is the function all working threads are running
void *worker_thread_func(void *arg)
{
	int tid = ((ThreadData*)arg)->tid;
	ThreadPool *tpool = ((ThreadData*)arg)->tpool;

	tpool->stats[tid].start_time = get_msec();

	while(!tpool->stopping) {
		unsigned long msec = get_msec();

		pthread_mutex_lock(&tpool->work_pending_mutex);
		if(tpool->num_tasks) {
			// there's work to be done, grab a task and do it...
			Task task = tpool->workq.front();
			tpool->workq.pop();
			tpool->num_tasks--;

			tpool->stats[tid].tasks++;
			pthread_mutex_unlock(&tpool->work_pending_mutex);


			tpool->stats[tid].proc_start = msec;
			
			task.proc(task.closure);
			tpool->finish_task(task);	// this will call the done callback if avail.
			
			tpool->stats[tid].proc_time += get_msec() - tpool->stats[tid].proc_start;
			tpool->stats[tid].proc_start = 0;

		} else {
			tpool->stats[tid].idle_start = msec;
			// no work to be done, go to sleep & wait on the condvar
			while(!tpool->num_tasks && !tpool->stopping) {
				pthread_cond_wait(&tpool->work_pending_cond, &tpool->work_pending_mutex);
			}
			pthread_mutex_unlock(&tpool->work_pending_mutex);

			tpool->stats[tid].idle_time += get_msec() - tpool->stats[tid].idle_start;
			tpool->stats[tid].idle_start = 0;
		}
	}

	// return our thread data pointer, so that we may free the memory after the
	// join in the main thread (see ThreadPool::stop)
	return arg;
}

// ---- worker thread function ----

/* The following highly platform-specific code detects the number
 * of processors available in the system. It's used by the thread pool
 * to autodetect how many threads to spawn.
 * Currently works on: Linux, BSD, Darwin, and Windows.
 */

#if defined(__APPLE__) && defined(__MACH__)
# ifndef __unix__
#  define __unix__	1
# endif	/* unix */
# ifndef __bsd__
#  define __bsd__	1
# endif	/* bsd */
#endif	/* apple */

#if defined(unix) || defined(__unix__)
#include <unistd.h>

# ifdef __bsd__
#  include <sys/sysctl.h>
# endif
#endif

#if defined(WIN32) || defined(__WIN32__)
#include <windows.h>
#endif


int get_number_processors()
{
#if defined(unix) || defined(__unix__)
# if defined(__bsd__)
	/* BSD systems provide the num.processors through sysctl */
	int num, mib[] = {CTL_HW, HW_NCPU};
	size_t len = sizeof num;

	sysctl(mib, 2, &num, &len, 0, 0);
	return num;

# elif defined(__sgi)
	/* SGI IRIX flavour of the _SC_NPROC_ONLN sysconf */
	return sysconf(_SC_NPROC_ONLN);
# else
	/* Linux (and others?) have the _SC_NPROCESSORS_ONLN sysconf */
	return sysconf(_SC_NPROCESSORS_ONLN);
# endif	/* bsd/sgi/other */

#elif defined(WIN32) || defined(__WIN32__)
	/* under windows we need to call GetSystemInfo */
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#endif
}
