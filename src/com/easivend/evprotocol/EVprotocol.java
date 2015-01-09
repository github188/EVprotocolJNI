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
	
	//�����ӿ�  cabinet ���  column������ type֧����ʽ 0�ֽ� ��0 ���ֽ�   cost �ۿ���
	public native int trade(int cabinet,int column,int type,int cost);
	
	//JNI ��̬�ص�����
	public static void EV_callBackStatic(int i) 
	{
		Log.i("Java------------->","" +  i);
	}
		
	//JNI�ص�����
	public void EV_callBack(String json_msg)
	{
		Log.i("JSON", json_msg);

	}



	
}