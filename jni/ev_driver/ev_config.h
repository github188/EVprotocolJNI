#ifndef _EV_CONFIG_H_
#define _EV_CONFIG_H_

#include<jni.h>
#include<android/log.h>



#define EV_LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "EV_thread", __VA_ARGS__))
#define EV_LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "EV_thread", __VA_ARGS__))
#define EV_LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "EV_thread", __VA_ARGS__))



//7级日志输出  数字越小级别越高
#define EV_LOGI1(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGI2(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGI3(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGI4(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGI5(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGI6(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGI7(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))

#define EV_LOGCOM(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))

#define EV_LOGTASK(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define EV_LOGFLOW(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))






/*********************************************************************************************************
**定义通用宏函数
*********************************************************************************************************/

#define HUINT16(v)   	(((v) >> 8) & 0xFF)
#define LUINT16(v)   	((v) & 0xFF)
#define INTEG16(h,l)  	(((unsigned int)h << 8) | l)





unsigned short EV_crcCheck(unsigned char *msg,unsigned char len);

#endif
