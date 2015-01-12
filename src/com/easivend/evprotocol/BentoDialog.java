package com.easivend.evprotocol;



import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;

public class BentoDialog extends Dialog implements android.view.View.OnClickListener{

	public BentoDialog(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
	}

	public BentoDialog(Context context,LeaveMyDialogListener listener) {
        super(context);
        // TODO Auto-generated constructor stub
        this.context = context;
        this.listener = listener;
    }
	
	
	 @Override
	 protected void onCreate(Bundle savedInstanceState){
		 super.onCreate(savedInstanceState);
		 setContentView(R.layout.bento_dialog);
		 setTitle("格子柜出货");
		 
		 Button button_ok = (Button)this.findViewById(R.id.button_ok);
		 Button button_cancel = (Button)this.findViewById(R.id.button_cancel);
		// Button button_check =  (Button)this.findViewById(R.id.button_check);
		 //button_check.setOnClickListener(this);
		 
		 
		 
//		 RadioButton radioButton_lightOn = (RadioButton) this.findViewById(R.id.radioButton_lightON);
//		 radioButton_lightOn.setChecked(true);
//		 radioButton_lightOn.setOnClickListener(this);
//		 
//		 RadioButton radioButton_lightOff = (RadioButton) this.findViewById(R.id.radioButton_lightOFF);
//		 radioButton_lightOff.setOnClickListener(this);
		 
		 button_ok.setOnClickListener(this);
		 button_cancel.setOnClickListener(this);
		 
	 }
	 
	 
	 public interface LeaveMyDialogListener{   
	        public void onClick(View view);   
	    } 
	 
	
	private Context context;
	private LeaveMyDialogListener listener;
	
	
	
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		listener.onClick(v);
	}


}
