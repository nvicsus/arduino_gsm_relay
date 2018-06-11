
/*
AT+CMGF=1   //set text mode
+CMTI: "SM",x  //indicates that SMS has arrived, where x is cell
AT+CMGDA="DEL READ" //delete all read messages
AT+CMGR=2 //read message #2
at+CMGL="REC UNREAD" //read all unread messages
AT+CMGD=1,2  //delete all read messages
AT+IPR=19200  //set baudrate
*/

// 14.... 19 equ A0 equ A5




#define RED 7        //LED pins
#define YELLOW 6
#define GREEN 5
#define CMD_SENT 0    //
#define LED_ON 1
#define AT 0      //bit #0
#define CPMS 1    //bit #1
#define CMGL 2    //bit #2
#define CMGD 3    //bit #3


long relay_on_millis;
String input_string;
String chan_str,dura_str;
unsigned char chan,dura;
char in_char;
char state;
char index1,index2,index3;


/*
0-idle
1-AT command was sent
2-OK for AT was received
3-
4-modem reported about SMS
5-request for reading SMS CMGL was sent
6-SMS was read
7-request for deleting SMS
8-fail state

*/


boolean  led_is_on;
long trigger_millis[8]; //  bit 0 - command was sent, bit 1 - LED was lightened, bits 7..0 relay,
long relay_millis[5];  //when relay was switched on
long dura_millis[5];  //when relay must be switched off
boolean trigger_relay[5]; // correspond relays
byte trigger_cmd_sent;   // bit 0 - "AT" was sent; bit 1 - "CPMS?" was sent, bit 2 "CMGL" was sent, bit 3 "CMGD" was sent




void send_cmd(char cmd)
{
  
    switch (cmd)
  {
    case AT:
      Serial.println("AT");
      trigger_cmd_sent |= (1<<AT);		// ping
      state=1;
      
      break;
    case CMGL:
      Serial.println("AT+CMGL=\"REC UNREAD\"");	//read all unread SMS
      trigger_cmd_sent |= (1<<CMGL);
      state=5;
      
      break;
    case CMGD:
      Serial.println("AT+CMGD=1,2");		//del all read SMS
      trigger_cmd_sent |= (1<<CMGD);
      state=7;
     
      break;
  }
      trigger_millis[CMD_SENT]=millis();
      led_on(GREEN);
}

void setup()
{
 Serial.begin(9600);
  delay(1000);
  Serial.println("ATE0");
  
  for (char i=0;i<5;i++) pinMode (i+14,OUTPUT);
  for (char i=0;i<5;i++) digitalWrite(i+14,LOW);
  pinMode(RED,OUTPUT);
  pinMode(YELLOW,OUTPUT);
  pinMode(GREEN,OUTPUT);
  pinMode(13,OUTPUT);
 
  state=0;
  trigger_cmd_sent=0;
  for (char i=0;i<8;i++)  {trigger_millis[i]=0;};
  input_string="";
  send_cmd(AT);
}


void decode_answer(String answer)
{
  input_string="";
  if (answer.indexOf("OK") > -1) {
  led_on(YELLOW);
  trigger_millis[LED_ON]=millis();

  switch (state){
    case 1:
      trigger_cmd_sent &= !(1<<AT);   //clear "AT" flag 
      
      send_cmd(CMGL);
      state=5;
      return;
    break;

    case 5:
      trigger_cmd_sent &= !(1<<CMGL);   //clear "CMGL"    
      send_cmd(CMGD) ;
      return;
      break;

    case 7:
      trigger_cmd_sent &= !(1<<CMGD);   //clear "CMGD"
      state=0;
      return;
      break;
    default:;                    
                  }
                                }
  index1=answer.indexOf("+CMTI:");
  if (index1 > -1)  if (state == 0) {
	send_cmd(CMGL);
	return;
				    };

  if (state == 5)  {
      index1=answer.indexOf(")");
      if ( index1 > -1) index2=answer.indexOf("@"); else return;
         if (index2 > -1) index3=answer.indexOf("("); else return;
            if (index3 > -1) { 
       chan_str=answer.substring(index1+1,index2);
       dura_str=answer.substring(index2+1,index3); 
       if (chan_str.length() == 0) chan=0; else chan=chan_str.toInt();
       if (dura_str.length() == 0) dura=1; else dura=dura_str.toInt();
       set_relay(chan,dura);
                                } else return;   
                    }
  if (answer.indexOf("ERROR") > -1) state=8;
         answer="";
 
}


void set_relay(char chan, char dura)
{

  if (chan > 5) chan=0;      //all channels
  if (dura == 0) dura=1;      // if duration parameter is empty - 1 sec
  if (chan == 0)             // if channel parameter is empty - all channels
    for (char i=0;i<5;i++) {
          digitalWrite(i+14,HIGH);
          relay_millis[i]=millis();
          dura_millis[i]=dura;
          trigger_relay[i]=true;
                            }
  else {
  digitalWrite(chan+13,HIGH);      //channels are 1..5 correspond pins 14..18
  relay_millis[chan-1]=millis();
  dura_millis[chan-1]=dura;
  trigger_relay[chan-1]=true;
        }                          
    
}

void led_on(int color)
{
  digitalWrite(color,HIGH);
  trigger_millis[LED_ON]=millis();
  led_is_on=true;
}

void led_off()
{
  digitalWrite(GREEN,LOW);digitalWrite(YELLOW,LOW);digitalWrite(RED,LOW);led_is_on=false;
}

void loop()
{
    
  if (led_is_on) if (millis() > (trigger_millis[LED_ON]+300)) {led_off();}
//  if (trigger_relay) if(millis() > (relay_on_millis+900)) {for (char i=0;i<5;i++) digitalWrite(i+14,LOW); trigger_relay=0;} //if relays were switched on more than 1.2 sec ago, turn them off
  for (char i=0;i<5;i++){
    if (trigger_relay[i]) if (millis() > (relay_millis[i]+dura_millis[i]*1000)) {
                digitalWrite(i+14,LOW);
                trigger_relay[i]=false;
                                                                                }
                        }
  
  if (trigger_cmd_sent)
     switch (state){
       case 1:          //"AT"
       if (millis() > (trigger_millis[CMD_SENT]+3000)) {trigger_cmd_sent &= !(1<<AT);state=8;} //answer for AT wasn't received
       break;
       case 5:         //"CMGL"
       if (millis() > (trigger_millis[CMD_SENT]+3000)) {trigger_cmd_sent &= !(1<<CMGL);state=8;} //answer for CMGL wasn't received
       break;
       case 7:        //"CMGD"
       if (millis() > (trigger_millis[CMD_SENT]+3000)) {trigger_cmd_sent &= !(1<<CMGD);state=8;} //answer for CMGD wasn't received
       break;
       default:;
                           }
  
  if (millis() > (trigger_millis[CMD_SENT]+10000)) if (state == 0) {    //send next AT command
          send_cmd(AT);

                                        }

  if (Serial.available()) {
        in_char=Serial.read(); input_string += in_char;
        if (in_char=='\n') decode_answer(input_string);
                              };
                              
  if (state == 8) {led_on(RED);state=0;}
    
}


