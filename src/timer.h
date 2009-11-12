#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

unsigned long get_msec(void);
void msleep(int msec);

#ifdef __cplusplus
}
#endif

#endif	/* TIMER_H_ */
