package com.easivend.evprotocol;

import android.os.Handler;
import android.os.Message;
import android.util.Log;


public class EVprotocol {
	static{
		System.loadLibrary("EVprotocol");
		
	}
	//public native String StringFromJni();	
	
	//jni �����ӿ�  �ɹ� ����1  ʧ�ܷ��� -1
	public native int vmcStart(String portName);
	public native void vmcStop();
	
	//�����ӿ�  cabinet ���  column������ type֧����ʽ 0�ֽ� ��0 ���ֽ�   cost �ۿ���
	public native int trade(int cabinet,int column,int type,int cost);
	
	//�˱�����  type 0Ӳ�ҳ���  1ֽ�ҳ���  ;value ���ҵĽ��
	public native int payout(long value);
	
	//���ӹ������� ���� 1�ɹ�  0ʧ��
	public native int bentoOpen(int cabinet,int box);
	public native int bentoLight(int cabinet,int flag);//flag 1����  0�ص�
	public native String bentoCheck(int cabinet);
	public native int bentoRegister(String portName);
	public native int bentoRelease();
	
	//JNI ��̬�ص�����
	public static void EV_callBackStatic(int i) 
	{
		Log.i("Java------------->","" +  i);
	}
		
	//JNI�ص�����
	public void EV_callBack(String json_msg)
	{
		Log.i("JSON", json_msg);
		//str = json_msg;
		if(handler != null)
		{
			Message msg = Message.obtain();
			msg.what = 1;
			msg.obj = json_msg;
			handler.sendMessage(msg);
		}
	}

	
	public EVprotocol(){}
	public EVprotocol(Handler handler){this.handler = handler;}

//	public String str;
	public Handler handler = null;
	
}