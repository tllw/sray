#ifndef TPOOL_H_
#define TPOOL_H_

#include <queue>
#include <pthread.h>

// returns the number of processors (cores) in the system
extern "C" int get_number_processors();

void *worker_thread_func(void *arg);


struct Task {
	void *closure;
	void (*proc)(void *cls);
	void (*done)(void *cls);

	Task();
	Task(void (*proc)(void*), void (*done)(void*), void *cls);
};

struct ThreadStats {
	unsigned long start_time;
	unsigned long proc_time, idle_time;
	unsigned long idle_start, proc_start;

	unsigned long tasks;
};

class ThreadPool {
private:
	std::queue<Task> workq;
	int num_tasks;

	pthread_t *threads;
	ThreadStats *stats;
	int num_threads;

	pthread_cond_t work_pending_cond;
	pthread_mutex_t work_pending_mutex;

	volatile int work_left;
	pthread_cond_t done_cond;
	//pthread_mutex_t done_mutex;

	void finish_task(const Task &task);

	bool stopping;

public:
	ThreadPool();
	~ThreadPool();

	bool start(int num_threads = 0);
	void stop();

	inline int get_num_threads() const;
	inline const ThreadStats *get_thread_stats(int tid) const;

	bool add_work(const Task *tasks, int count);
	void clear_work();

	void wait_work();
	inline bool is_done() const;

	friend void *worker_thread_func(void *arg);
};


// --- threadpool inline functions ---
inline int ThreadPool::get_num_threads() const
{
	return num_threads;
}

inline const ThreadStats *ThreadPool::get_thread_stats(int tid) const
{
	return stats + tid;
}

inline bool ThreadPool::is_done() const
{
	return work_left == 0;
}

#endif	// TPOOL_H_
