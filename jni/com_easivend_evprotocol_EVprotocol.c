#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<assert.h>
#include<jni.h>
#include<android/log.h>
#include "ev_api/EV_com.h"
#include "ev_api/json.h"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "EV_thread", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "EV_thread", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "EV_thread", __VA_ARGS__))

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




int JNI_setIntField(char *tag,int value)
{
	jclass j_class;
    jfieldID j_fid;
	j_class = (*g_env)->GetObjectClass(g_env, g_obj);
	if (0 == j_class) {
        LOGW("GetObjectClass returned 0\n");
        return (-1);
    }
	j_fid = (*g_env)->GetFieldID(g_env, j_class, "tag", "I");
	if(j_fid == NULL)
		return -1;
	(*g_env)->SetIntField(g_env, g_obj, j_fid, value);
	return 1;
}

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

int JNI_json(char *str[])
{
	char *text;
    json_t *root, *entry, *label, *value;

	root = json_new_object();
    entry = json_new_object();

	label = json_new_string("result");
	value = json_new_string("00");
	json_insert_child(label,value);
	json_insert_child(entry,label);
	
	label = json_new_string("remain");
	value = json_new_string("00");
	json_insert_child(label,value);
	json_insert_child(entry,label);

	label = json_new_string("state");
	value = json_new_string("00");
	json_insert_child(label,value);
	json_insert_child(entry,label);
	label = json_new_string("trade");
    json_insert_child(label, entry);
	json_insert_child(root, label);
	json_tree_to_string(root, &text);
	//msg = (*g_env)->NewStringUTF(g_env,text);
	//if(methodID_EV_callBack)// 最后调用类中“成员”方法
	//{
	//	(*g_env)->CallVoidMethod(g_env, g_obj, methodID_EV_callBack,msg);
	//}	
    free(text);
    json_free_value(&root);

	
}



void JNI_json_insert_str(json_t *json,char *label,char *value)
{
	json_t *j_label,*j_value;
	if(label == NULL || value == NULL || json == NULL )
		return;
	j_label = json_new_string(label);
	j_value = json_new_string(value);
	json_insert_child(j_label,j_value);
	json_insert_child(json,j_label);
}


void JNI_json_insert_int(json_t *json,char *label,int value,int no)
{
	json_t *j_label,*j_value;
	char buf[10] = {0};
	if(label == NULL || json == NULL )
		return;
	if(no == 2)
		sprintf(buf,"%02d",value);
	else if(no == 4)
		sprintf(buf,"%04d",value);
	else if(no == 8)
		sprintf(buf,"%08d",value);
	else
		sprintf(buf,"%d",value);
	j_label = json_new_string(label);
	j_value = json_new_number(buf);
	json_insert_child(j_label,j_value);
	json_insert_child(json,j_label);
}




void JNI_callBack(const int type,const void *ptr)
{
	jstring msg;
	char buf[512] = {0};
	char *text;
	unsigned int temp;
	unsigned char *data;
    json_t *root = NULL, *entry = NULL, *label, *value;

	switch(type)
	{
		case EV_SETUP_REQ:
			break;
		case EV_TRADE_RPT:
			root = json_new_object();
    		entry = json_new_object();
			data = (unsigned char *)ptr;		
			JNI_json_insert_int(entry,"cabinet",data[MT + 1],2);		
			JNI_json_insert_int(entry,"column",data[MT + 3],2);
			JNI_json_insert_int(entry,"result",data[MT + 2],2);
			JNI_json_insert_int(entry,"type",data[MT + 4],2);
			temp = INTEG16(data[MT + 5],data[MT + 6]);
			JNI_json_insert_int(entry,"cost",temp,4);
			temp = INTEG16(data[MT + 7],data[MT + 8]);
			JNI_json_insert_int(entry,"remainAmount",temp,4);
			JNI_json_insert_int(entry,"remainCount",data[MT + 9],2);
			label = json_new_string("trade");
			
			json_insert_child(label,entry);
			json_insert_child(root,label);
			break;
		case EV_ENTER_MANTAIN:
			root = json_new_object();
    		entry = json_new_object();
			JNI_json_insert_str(entry,"EV_ENTER_MANTAIN","01");
			label = json_new_string("ACTION");
			json_insert_child(label,entry);
			json_insert_child(root,label);
			break;
		case EV_EXIT_MANTAIN:
			root = json_new_object();
    		entry = json_new_object();

			JNI_json_insert_str(entry,"EV_EXIT_MANTAIN","01");
			label = json_new_string("ACTION");
			json_insert_child(label,entry);
			json_insert_child(root,label);
			break;
		case EV_OFFLINE:
			break;
		default:
			break;
	}
	if(root != NULL)
	{
		json_tree_to_string(root, &text);
		msg = (*g_env)->NewStringUTF(g_env,text);
		if(methodID_EV_callBack)// 最后调用类中“成员”方法
		{
			(*g_env)->CallVoidMethod(g_env, g_obj, methodID_EV_callBack,msg);
		}	
	    free(text);
		json_free_value(&root);
	}
    

	
	

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
	methodID_EV_callBack = (*env)->GetMethodID(env,cls,"EV_callBack","(Ljava/lang/String;)V");
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
  (JNIEnv *env, jclass cls,jint cabinet, jint column, jint type, jint cost)
{
	jint ret;
	unsigned char buf[20],ix = 0;
	buf[ix++] = cabinet;//pReq->cabinet & 0xFF;
	buf[ix++] = 2;//pReq->type  & 0xFF;
	buf[ix++] = column;//pReq->id  & 0xFF;
	buf[ix++] = type;//pReq->payMode  & 0xFF;
	buf[ix++] = HUINT16(cost);//pReq->cost / 256;
	buf[ix++] = LUINT16(cost);//pReq->cost % 256;
	ret = EV_pcReqSend(VENDOUT_IND,1,buf,ix);

	return ret;
}





