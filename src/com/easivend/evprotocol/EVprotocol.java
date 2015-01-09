package com.easivend.evprotocol;

import android.os.Handler;
import android.os.Message;
import android.util.Log;


public class EVprotocol {
	static{
		System.loadLibrary("EVprotocol");
		
	}
	//public native String StringFromJni();	
	public native int vmcStart(String portName);
	public native void vmcStop();
	
	//出货接口  cabinet 柜号  column货道号 type支付方式 0现金 非0 非现金   cost 扣款金额
	public native int trade(int cabinet,int column,int type,int cost);
	
	//JNI 静态回调函数
	public static void EV_callBackStatic(int i) 
	{
		Log.i("Java------------->","" +  i);
	}
		
	//JNI回调函数
	public void EV_callBack(String json_msg)
	{
		Log.i("JSON", json_msg);

	}



	
}