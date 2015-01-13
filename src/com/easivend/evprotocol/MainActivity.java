package com.easivend.evprotocol;



import java.io.Serializable;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import android.R.integer;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.TextView;


public class MainActivity extends Activity {

	
	private TextView textview = null;
	private EditText editText_port = null;
	private TradeDialog dialog  = null;
	
	private TextView textView_VMCState = null;
	private TextView textView_Amount = null;
	
	private String EV_HEAD = "EV_json";
	private String EV_TYPE = "EV_type";
	
	
	private BentoDialog bentoDialog = null;
	
	private Handler handler = new Handler(){
		@Override
		public void handleMessage(Message msg){
			if(msg.what == 1)
			{
				String text = (String)msg.obj;
				textview.setText(text);
				//解析json
				try {
					JSONObject jsonObject = new JSONObject(text); 
					JSONObject ev_head = (JSONObject) jsonObject.getJSONObject(EV_HEAD);
					String str_evType =  ev_head.getString(EV_TYPE);
					Log.i("JSON-EV_TYPE",str_evType);
						
					if(str_evType.equals("EV_INITING"))//正在初始化
					{
						textView_VMCState.setText("正在初始化");
						Log.i("JSON-EV_TYPE","正在初始化");
					}
					else if(str_evType.equals("EV_ONLINE"))//str_evType.equals("EV_PAYOUT_RPT")
					{
						//textView_VMCState.setText("成功连接");
					}
					else if(str_evType.equals("EV_OFFLINE"))
					{
						textView_VMCState.setText("断开连接");
					}
					else if(str_evType.equals("EV_RESTART"))
					{
						textView_VMCState.setText("主控板重启心动");
					}
					else if(str_evType.equals("EV_STATE_RPT"))
					{
						int state = ev_head.getInt("state");
						if(state == 0)
							textView_VMCState.setText("断开连接");
						else if(state == 1)
							textView_VMCState.setText("正在初始化");
						else if(state == 2)
							textView_VMCState.setText("正常");
						else if(state == 3)
							textView_VMCState.setText("故障");
						else if(state == 4)
							textView_VMCState.setText("维护");
					}
					
					else if(str_evType.equals("EV_PAYIN_RPT"))//投币上报
					{
						int amount = ev_head.getInt("remainAmount");
						System.out.println(amount);
						textView_Amount.setText(Integer.toString(amount));
						//textView_Amount.setText(ev_head.getString("remainAmount"));
					}
					else if(str_evType.equals("EV_PAYOUT_RPT"))
					{
						int amount = ev_head.getInt("remainAmount");
						System.out.println(amount);
						textView_Amount.setText(Integer.toString(amount));
						//textView_Amount.setText(ev_head.getString("remainAmount"));
					}
					else if(str_evType.equals("EV_ENTER_MANTAIN"))
					{
						textView_VMCState.setText("维护");
					}
					else if(str_evType.equals("EV_EXIT_MANTAIN"))
					{
						textView_VMCState.setText("退出维护");
					}
					
					
				} catch (JSONException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					Log.w("EV_JSON", "Read EV_json err!!!!!!!!!!!!!!");
				}
				
			}
			
			
			
		}
	};
	EVprotocol ev = new EVprotocol(handler);
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		textview =(TextView)this.findViewById(R.id.textView_Json);  
	    editText_port = (EditText)this.findViewById(R.id.editText_port);
	    
	    textView_VMCState = (TextView)this.findViewById(R.id.textView_VMCState);
	    textView_Amount = (TextView)this.findViewById(R.id.textView_Amount);
	    
		Button button_start = (Button)this.findViewById(R.id.button_start);  
		
		
		
		button_start.setOnClickListener(new View.OnClickListener()
		{
				@Override
				public void onClick(View v)
				{
					//ev.vmcStart("/dev/s3c2410_serial3");
					
					int ret = ev.vmcStart(editText_port.getText().toString());
					if(ret == 1)
						textview.setText("打开成功");
					else
						textview.setText("串口打开失败");
						
					//new Thread(new MyThread()).start();
				}
		});
		Button button_stop = (Button)this.findViewById(R.id.button_stop);  
		button_stop.setOnClickListener(new View.OnClickListener()
		{
				@Override
				public void onClick(View v)
				{
					ev.vmcStop();
					textView_VMCState.setText("断开连接");
				}
		});
		
		Button button_payout = (Button)this.findViewById(R.id.button_payout);
		button_payout.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				
				ev.payout(Integer.parseInt(textView_Amount.getText().toString()));
			}
		});
		
		
		Button button_bento =  (Button)this.findViewById(R.id.button_bento);
		button_bento.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				if(bentoDialog == null)
				{
					bentoDialog = new BentoDialog(MainActivity.this,
							new BentoDialog.LeaveMyDialogListener(){
						@Override
						public void onClick(View view) {
							
							EditText editText_port = (EditText)bentoDialog.findViewById(R.id.editText_port);
							EditText editText_cabinet = (EditText)bentoDialog.findViewById(R.id.editText_cabinet);
							EditText editText_box = (EditText)bentoDialog.findViewById(R.id.editText_box);
							TextView textView_oepnState = (TextView)bentoDialog.findViewById(R.id.textView_openState);
							
//							EditText editText_boxNum = (EditText)bentoDialog.findViewById(R.id.editText_boxNum);
//							EditText editText_id = (EditText)bentoDialog.findViewById(R.id.editText_id);
							switch(view.getId())
							{
								case R.id.button_ok:
									//bentoDialog.dismiss();
									
									ev.bentoRegister(editText_port.getText().toString());
									int cabinet =  Integer.parseInt(editText_cabinet.getText().toString());  
									int box =  Integer.parseInt(editText_box.getText().toString());  
									int ret = ev.bentoOpen(cabinet, box);									
									if(ret == 1)//成功
									{
										textView_oepnState.setText("成功");
										
									}
									else
									{
										textView_oepnState.setText("失败");
									}
									
									ev.bentoRelease();
									
									break;
								case R.id.button_cancel:
									bentoDialog.dismiss();
									break;
//								case R.id.button_check:
//									int cabinetno =  Integer.parseInt(editText_cabinet.getText().toString());  
//									ev.bentoRegister(editText_port.getText().toString());
//									String str = ev.bentoCheck(cabinetno);
//									if(!str.isEmpty())
//									{
//										try {
//											JSONObject jsonObject = new JSONObject(str);
//											JSONObject ev_head = (JSONObject) jsonObject.getJSONObject(EV_HEAD);
//											String str_evType =  ev_head.getString(EV_TYPE);
//											String ID = ev_head.getString("ID");
//											int boxNum = ev_head.getInt("boxNum");
//											editText_boxNum.setText(Integer.toString(boxNum));
//											editText_id.setText(ID);
//											
//										} catch (JSONException e) {
//											// TODO Auto-generated catch block
//											e.printStackTrace();
//										} 
//										
//										
//									}
//									ev.bentoRelease();
//									break;
									
//								case R.id.radioButton_lightON:
//									cabinetno =  Integer.parseInt(editText_cabinet.getText().toString());  
//									ev.bentoRegister(editText_port.getText().toString());
//									ev.bentoLight(cabinetno, 1);
//									ev.bentoRelease();
//									break;
//								case R.id.radioButton_lightOFF:
//									cabinetno =  Integer.parseInt(editText_cabinet.getText().toString());
//									ev.bentoRegister(editText_port.getText().toString());
//									ev.bentoLight(cabinetno, 0);
//									ev.bentoRelease();
//									break;
								default:break;
									
							}
						}
					});
					
					
				}
				bentoDialog.show();
			}
		});
		
		
		Button button_trade = (Button)this.findViewById(R.id.button_trade);  
		button_trade.setOnClickListener(new View.OnClickListener()
		{
				@Override
				public void onClick(View v)
				{
					if(dialog == null)
					{
						dialog = new TradeDialog(MainActivity.this,
								new TradeDialog.LeaveMyDialogListener() {
									
									@Override
									public void onClick(View view) {
										// TODO Auto-generated method stub
										switch(view.getId()){
											case R.id.button_ok:
												dialog.dismiss();
												
												EditText editText_cabinet = (EditText)dialog.findViewById(R.id.editText_cabinet);
												EditText editText_column = (EditText)dialog.findViewById(R.id.editText_column);
												RadioButton radioButton_pc = (RadioButton)dialog.findViewById(R.id.radioButton_pc);
												EditText editText_cost = (EditText)dialog.findViewById(R.id.editText_cost);
												int cabinet =  Integer.parseInt(editText_cabinet.getText().toString());  
												int column =  Integer.parseInt(editText_column.getText().toString());  
												int paymode,cost;
												if(radioButton_pc.isChecked())
												{
													paymode = 1;
													cost = 0;
												}
												else
												{
													paymode = 0;
													cost = Integer.parseInt(editText_cost.getText().toString());  
												}
												
												
												ev.trade(cabinet,column,paymode,cost);
												Log.d("Button_ok", "OK");
												System.out.println(cabinet);
												System.out.println(column);
												System.out.println(paymode);
												System.out.println(cost);
												break;
											case R.id.button_cancel:
												Log.d("button_cancel", "cancel");
												dialog.dismiss();
												break;
											default:break;
										}
									}
								});
						
					}

					dialog.show();
					
				}
		});
		
		
		



		
	}

//	public class MyThread implements Runnable{
//		@Override
//		public void run() {
//			// TODO Auto-generated method stub
//			if(0 == 1)
//			{
//				Message msg = Message.obtain();
//				//msg.obj = ev.str;
//				msg.what = 1;
//				handler.sendMessage(msg);
//			}
//		}
//	}		
//	
	
	public void onClick(View v){
		
	}
	
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	
	
	


}
