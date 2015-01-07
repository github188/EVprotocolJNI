package com.easivend.evprotocol;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		final EVprotocol ev = new EVprotocol();
		
		
		
		Button button_start1 = (Button)this.findViewById(R.id.button1);        
        button_start1.setOnClickListener(new View.OnClickListener()
        {
        		@Override
        		public void onClick(View v)
        		{
        			Log.i("test", "State");
        			System.out.println(ev.state);
        			System.out.println(ev.portName);
        			System.out.println(ev.str);		  
        		}
        });
		
	
	
		Button button_start = (Button)this.findViewById(R.id.button_start);  
		button_start.setOnClickListener(new View.OnClickListener()
		{
				@Override
				public void onClick(View v)
				{
					ev.vmcStart("/dev/s3c2410_serial3");
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
					ev.trade();
				}
		});
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
