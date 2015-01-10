package com.easivend.evprotocol;



import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;


public class MainActivity extends Activity {

	
	TextView textview = null;
	private Handler handler = new Handler(){
		@Override
		public void handleMessage(Message msg){
			if(msg.what == 1)
			{
				String text = (String)msg.obj;
				textview.setText(text);
			}
			
			
			
		}
	};
	EVprotocol ev = new EVprotocol(handler);
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		textview =(TextView)this.findViewById(R.id.textView_Json);  
		Button button_start = (Button)this.findViewById(R.id.button_start);  
		button_start.setOnClickListener(new View.OnClickListener()
		{
				@Override
				public void onClick(View v)
				{
					ev.vmcStart("/dev/s3c2410_serial3");
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
				}
		});
		Button button_trade = (Button)this.findViewById(R.id.button_trade);  
		button_trade.setOnClickListener(new View.OnClickListener()
		{
				@Override
				public void onClick(View v)
				{
					ev.trade(1,11,1,0);
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
