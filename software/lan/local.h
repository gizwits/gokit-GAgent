
#ifndef  __LOCAL_H_
#define  __LOCAL_H_



#define LOCAL_HB_TIMEOUT_ONESHOT        55      /* 55S */
#define LOCAL_HB_TIMEOUT_MAX            180     /* 180S */
#define LOCAL_HB_TIMEOUT_CNT_MAX        (LOCAL_HB_TIMEOUT_MAX / LOCAL_HB_TIMEOUT_ONESHOT)
#define LOCAL_HB_REDUNDANCE             (LOCAL_HB_TIMEOUT_MAX % LOCAL_HB_TIMEOUT_ONESHOT)

extern void Local_Timer(void);

#endif
