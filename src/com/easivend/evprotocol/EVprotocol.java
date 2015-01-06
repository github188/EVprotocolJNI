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
	
	//JNI 静态回调函数
	public static void EV_callBackStatic(int i) 
	{
		Log.i("Java------------->","" +  i);
	}
		
	//JNI回调函数
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

	//JAVA上层回调函数
	public CallBack mCallBack; 
	
}