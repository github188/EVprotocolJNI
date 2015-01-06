package com.easivend.evprotocol;

import android.util.Log;

public class EVprotocol {
	static{
		System.loadLibrary("EVprotocol");
		
	}
	//public native String StringFromJni();	
	public native int register(EVpackage packege);
	public native int vmcStart();
	public native void vmcStop();
	public native int trade();
	
	//JNI ��̬�ص�����
	public static void EV_callBackStatic(int i) 
	{
		Log.i("Java------------->","" +  i);
	}
		
	//JNI�ص�����
	public void EV_callBack(String tag,String msg)
	{
		Log.i(tag, msg);
		//mCallBack.method();
	}
	
	public int state;
	public int portName;
	public String str;
	public EVpackage packege;
	
	
	
	public interface CallBack 
	{    
		public void method();    
	}

	//JAVA�ϲ�ص�����
	public CallBack mCallBack; 
	
}