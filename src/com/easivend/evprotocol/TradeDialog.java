package com.easivend.evprotocol;

import android.app.Dialog;
import android.content.Context;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;



public class TradeDialog extends Dialog implements android.view.View.OnClickListener{


	private Context context;
	private LeaveMyDialogListener listener;
	public TradeDialog(Context context) {
		super(context);
		this.context = context;
		// TODO Auto-generated constructor stub
		initDialog();
		
		
		Button button_ok = (Button)this.findViewById(R.id.button_ok);
		button_ok.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				
			}
		});
		
		
		
		Button button_cancel = (Button)this.findViewById(R.id.button_cancel);
		button_cancel.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				dismiss();
			}
		});
		

		
	
	}
	

	 public TradeDialog(Context context,LeaveMyDialogListener listener) {
	        super(context);
	        // TODO Auto-generated constructor stub
	        this.context = context;
	        this.listener = listener;
	    }
	
	 
	 @Override
	 protected void onCreate(Bundle savedInstanceState){
		 super.onCreate(savedInstanceState);
		 initDialog();
		 Button button_ok = (Button)this.findViewById(R.id.button_ok);
		 Button button_cancel = (Button)this.findViewById(R.id.button_cancel);
		 
		 
		 button_ok.setOnClickListener(this);
		 button_cancel.setOnClickListener(this);
		 
	 }
	 
	 

	
	public interface LeaveMyDialogListener{   
        public void onClick(View view);   
    }  
	

	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		listener.onClick(v);
	}
	
	
	
	public void initDialog(){
		setContentView(R.layout.trade_dialog);
		setTitle("³ö»õ²âÊÔ");
		
		RadioButton radioButton_pc = (RadioButton)this.findViewById(R.id.radioButton_pc);
		radioButton_pc.setChecked(true);
		
	}






	
	

	
}
