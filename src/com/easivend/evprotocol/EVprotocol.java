package com.easivend.evprotocol;

import android.util.Log;

public class EVprotocol {
	static{
		System.loadLibrary("EVprotocol");
		
	}
	//public native String StringFromJni();	
	public native int vmcStart(String portName);
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
	}
	
	
	//数据结构包
	public int state;
	public int portName;
	public String str;

	
}