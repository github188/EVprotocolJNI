#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<assert.h>
#include<jni.h>
#include<android/log.h>
#include "ev_api/EV_com.h"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "jni_thread", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "jni_thread", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "jni_thread", __VA_ARGS__))

// 全局变量
JavaVM* g_jvm = NULL;
jobject g_obj = NULL;
jobject g_evpak = NULL;


JNIEnv *g_env = NULL;
jclass g_cls = NULL;

static volatile int g_threadStop = 0;


static int uart_fd = -1;


static jmethodID methodID_EV_callBackStatic = NULL;
static jmethodID methodID_EV_callBack = NULL;



typedef struct _st_ev_data_{

	int state;
	int portName;
	char data[120];

}ST_EV_DATA;

static ST_EV_DATA EV_stdata;


int jni_setdata()
{
	jclass j_class;
    jfieldID j_fid;

	EV_stdata.state = 99;
	EV_stdata.portName = 1999;

	sprintf(EV_stdata.data,"%s","Arr suc...");

	LOGI("jni_setdata0");
	
	
	j_class = (*g_env)->GetObjectClass(g_env, g_obj);
	if (0 == j_class) {
        LOGW("GetObjectClass returned 0\n");
        return (-1);
    }

	LOGI("jni_setdata1");
	j_fid = (*g_env)->GetFieldID(g_env, j_class, "state", "I");

	(*g_env)->SetIntField(g_env, g_obj, j_fid, EV_stdata.state);

	
	j_fid = (*g_env)->GetFieldID(g_env, j_class, "portName", "I");

	(*g_env)->SetIntField(g_env, g_obj, j_fid, EV_stdata.portName);


	j_fid = (*g_env)->GetFieldID(g_env, j_class, "str", "Ljava/lang/String;");

	jstring name = (*g_env)->NewStringUTF(g_env, (char *)EV_stdata.data);
    (*g_env)->SetObjectField(g_env, g_obj, j_fid, name);
	
}


void JNI_callBack(const int type,const void *ptr)
{
	jmethodID met;
	int i;
	char buf[512] = {0};
	unsigned char *data = NULL;
	
	jstring tag,msg;
	
	if(type == 1)
	{
		data =  (unsigned char *)ptr;
		for(i = 0;i < data[1] + 2;i++)
			sprintf(&buf[i*3],"%02x ",data[i]);
		LOGI("VMC-RECV[%d]:%s\n",data[1],buf);
	}
	else if(type == 2)
	{
		data =  (unsigned char *)ptr;
		for(i = 0;i < data[1] + 2;i++)
			sprintf(&buf[i*3],"%02x ",data[i]);
		LOGI("VMC-SEND[%d]:%s\n",data[1],buf);
	}
	else if(type == 3)
	{
		LOGI("VMC:%s",(char *)ptr);
	}
	else if(type == 4)
	{
		data = (unsigned char *)ptr;
		sprintf(buf,"%02x ",*data);	
		LOGI("%s",buf);
	}
	else
	{
		LOGI("VMC:%s",(char *)ptr);
		tag = (*g_env)->NewStringUTF(g_env,"JNI:");
		msg = (*g_env)->NewStringUTF(g_env,"I'm ..........");
		if(methodID_EV_callBack)
		{
			// 最后调用类中“成员”方法
			(*g_env)->CallVoidMethod(g_env, g_obj, methodID_EV_callBack,tag,msg);
		}
	}

	
	
	//jni_setdata();
	

}








static void jni_register()
{
	JNIEnv *env;
	jclass cls;
	//注册回调
	int ret = EV_register(JNI_callBack);
	LOGI("EV_register.....suc %d\n",ret);

	// Attach主线程
	if((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) != JNI_OK)
	{
		LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
		return ;
	}

	g_env = env;
	g_cls = cls;
	
	// 找到对应的类
	cls = (*env)->GetObjectClass(env, g_obj);
	if(cls == NULL)
	{
		LOGE("FindClass() Error ......"); 
		return ;
	}
	
	// 再获得类中的方法
	methodID_EV_callBackStatic = (*env)->GetStaticMethodID(env, cls, "EV_callBackStatic", "(I)V");
	if(methodID_EV_callBackStatic == NULL)
	{
		LOGE("GetStaticMethodID() Error ......");
	}
	else
	{
		// 最后调用java中的静态方法
		(*env)->CallStaticVoidMethod(env, cls, methodID_EV_callBackStatic,NULL);
	}
	
	//获得类中的“成员”方法
	methodID_EV_callBack = (*env)->GetMethodID(env,cls,"EV_callBack","(Ljava/lang/String;Ljava/lang/String;)V");
	if(methodID_EV_callBack == NULL)
	{
		LOGE("GetMethodID() Error ......");
	}

}

static void jni_release()
{
	LOGE("jni_release1");

	EV_release();
	uart_fd = -1;
	LOGE("jni_release2");
	methodID_EV_callBack = NULL;
	methodID_EV_callBackStatic = NULL;

	//Detach主线程
	if((*g_jvm)->DetachCurrentThread(g_jvm) != JNI_OK)
	{
		LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
	}
	g_jvm = NULL;
	g_obj = NULL;
}

void* thread_fun(void* arg)
{
	jni_register();

	JNI_callBack(3,"thread_fun ready...");
	while(!g_threadStop)
	{
		EV_task(uart_fd);
	}
	LOGI("JNI Thread stopped.ready...");
	
	jni_release();
	LOGI("JNI Thread stopped....");
	pthread_exit(0);
	
}


/*
* Set some test stuff up.
*
* Returns the JNI version on success, -1 on failure.
*/
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGE("GetEnv failed!");
		return -1;
	}
	return JNI_VERSION_1_4;
}



/*
 * Class:     com_easivend_ev_vmc_EV_vmc
 * Method:    vmcStart
 * Signature: ()V
 */
JNIEXPORT jint JNICALL
Java_com_easivend_evprotocol_EVprotocol_vmcStart
  (JNIEnv *env, jclass cls,jstring jport)
{
	//char portName[50] = "/dev/s3c2410_serial3";
	const char *portName = (*env)->GetStringUTFChars(env,jport, NULL);
	pthread_t pid;
	int i = 0;
	EV_closeSerialPort();
	int fd = EV_openSerialPort((char *)portName,9600,8,'N',1);
	(*env)->ReleaseStringUTFChars(env,jport,portName);
	if (fd <0){
			LOGE("Can't Open Serial Port:%s!",portName);
			uart_fd = -1;
			return -1;
	}

	uart_fd = fd;

	LOGI("EV_openSerialPort suc.....!\n");
	//串口打开成功  开启线程
	if(g_jvm)//线程已经开启了  关闭线程
	{
		//g_threadStop = 1;
		//while(g_jvm != NULL) EV_msleep(100000);
		g_threadStop = 0;
		LOGI("The serialport thread has runing!!!!");
		return 1;
	}
		
	g_threadStop = 0;
	(*env)->GetJavaVM(env, &g_jvm);// 保存全局JVM以便在子线程中使用
	g_obj = (*env)->NewGlobalRef(env, cls);// 不能直接赋值(g_obj = ojb)
	pthread_create(&pid, NULL, &thread_fun, (void*)i);
	
	return 1;


}

/*
 * Class:     com_easivend_ev_vmc_EV_vmc
 * Method:    vmcStop
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_easivend_evprotocol_EVprotocol_vmcStop
  (JNIEnv *env, jclass cls)
{
	LOGI("Java_com_easivend_evprotocol_EVprotocol_vmcStop....");
	g_threadStop = 1;
}

/*
 * Class:     com_easivend_ev_vmc_EV_vmc
 * Method:    trade
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_easivend_evprotocol_EVprotocol_trade
  (JNIEnv *env, jclass cls)
{
	int ret;
	unsigned char buf[20],ix = 0;
	buf[ix++] = 0x01;//pReq->cabinet & 0xFF;
	buf[ix++] = 2;//pReq->type  & 0xFF;
	buf[ix++] = 11;//pReq->id  & 0xFF;
	buf[ix++] = 5;//pReq->payMode  & 0xFF;
	buf[ix++] = 0x00;//pReq->cost / 256;
	buf[ix++] = 0x00;//pReq->cost % 256;
	ret = EV_pcReqSend(VENDOUT_IND,1,buf,ix);

}





