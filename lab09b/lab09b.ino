#include <MPU9250.h>
#include <U8g2lib.h>
#include <math.h>
#include <Wifi_S08_v2.h>
#include <SPI.h>
#include <Wire.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI
#define SPI_CLK 14

MPU9250 imu;
SCREEN oled(U8G2_R2, 10, 15,16);  
ESP8266 wifi = ESP8266(0,false);

// IOT variables and constants
const int IOT_UPDATE_INTERVAL = 4000;// how often to send/pull from server
const String KERBEROS = "";  // your kerberos (need to change)
String PARTNER_KERBEROS = ""; //your partner's kerberos (need to change)
String KEY = "KiTTehs";  //Don't use! Insecure!!

String last_request = "post";
String MAC = "";
String path = "/6S08dev/jodalyst/l09b/slack_enc.py";//points to Joe's working version. DO NOT CHANGE!





void caesar_cipher(String message_in, String& message_out, int shift, bool encrypt){
  //your code here!
}

void vigenere_cipher(String message_in, String& message_out, String keyword, bool encrypt){
  //your code here!
}




const int BUTTON_PIN = 9;
class Button{
    int state;
    int flag;
    elapsedMillis t_since_change; //timer since switch changed value
    elapsedMillis t_since_state_2; //timer since entered state 2 (reset to 0 when entering state 0)
    unsigned long debounce_time;
    unsigned long long_press_time;
    int pin;
    bool button_pressed;
  public:
    Button(int p) {    
      state = 0;
      pin = p;
      t_since_change = 0;
      t_since_state_2= 0;
      debounce_time = 10;
      long_press_time = 1000;
      button_pressed = 0;
    }
    void read() {
      bool button_state = digitalRead(pin);  // true if HIGH, false if LOW
      button_pressed = !button_state; // Active-low logic is hard, inverting makes our lives easier.
    }
int update() {
  read();
  flag = 0;
  if (state==0) { // Unpressed, rest state
    if (button_pressed) {
      state = 1;
      t_since_change = 0;
    }
  } else if (state==1) { //Tentative pressed
    if (!button_pressed) {
      state = 0;
      t_since_change = 0;
    } else if (t_since_change >= debounce_time) {
      state = 2;
      t_since_state_2 = 0;
    }
  } else if (state==2) { // Short press
    if (!button_pressed) {
      state = 4;
      t_since_change = 0;
    } else if (t_since_state_2 >= long_press_time) {
      state = 3;
    }
  } else if (state==3) { //Long press
    if (!button_pressed) {
      state = 4;
      t_since_change = 0;
    }
  } else if (state==4) { //Tentative unpressed
    if (button_pressed && t_since_state_2 < long_press_time) {
      state = 2; // Unpress was temporary, return to short press
      t_since_change = 0;
    } else if (button_pressed && t_since_state_2 >= long_press_time) {
      state = 3; // Unpress was temporary, return to long press
      t_since_change = 0;
    } else if (t_since_change >= debounce_time) { // A full button push is complete
      state = 0;
      if (t_since_state_2 < long_press_time) { // It is a short press
        flag = 1;
      } else {  // It is a long press
        flag = 2;
      }
    }
  }
  return flag;
}  
};


void pretty_print(int startx, int starty, String input, int fwidth, int fheight, int spacing, SCREEN &display){
  int x = startx;
  int y = starty;
  String temp = "";
  for (int i=0; i<input.length(); i++){
     if (fwidth*temp.length()<= (SCREEN_WIDTH-fwidth -x)){
        if (input.charAt(i)== '\n'){
          display.setCursor(x,y);
          display.print(temp);
          y += (fheight + spacing);
          temp = "";
          if (y>SCREEN_HEIGHT) break;
        }else{
          temp.concat(input.charAt(i));
        }
     }else{
      display.setCursor(x,y);
      display.print(temp);
      temp ="";
      y += (fheight + spacing);
      if (y>SCREEN_HEIGHT) break;
      if (input.charAt(i)!='\n'){
        temp.concat(input.charAt(i));
      }else{
          display.setCursor(x,y);
          y += (fheight + spacing);
          if (y>SCREEN_HEIGHT) break;
      } 
     }
     if(i==input.length()-1){
        display.setCursor(x,y);
        display.print(temp);
     }
  }
}


class SlackSystem{
  String alphabet=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  String message;
  String query_string = "";
  int char_index;
  int state;
  bool move_on;
  elapsedMillis scrolling_timer;
  int scrolling_threshold = 150;
  float angle_threshold = 0.3;
  elapsedMillis iot_last_update;
  public:
    SlackSystem(){
      state = 0;
      message ="";
      char_index = 0;
      iot_last_update=0;
      move_on = false;
    }
  void update(float angle, int button){
    if (state==0){
      if (button==2){
          char_index=0;
          move_on = true;
      }
      if (wifi.hasResponse()) {//if something is there
        message = wifi.getResponse(); //get the response
        int htmlindex = message.indexOf("</html>");
        message = message.substring(6,htmlindex);//remove htmls
        Serial.print("Raw Message: "); Serial.println(message); //print it in serial monitor
        String decoded = "";
        vigenere_cipher(message,decoded,KEY,false);
        oled.clearBuffer();
        pretty_print(0,20,"Message:\n"+message,10,20,0,oled); //print response
        oled.sendBuffer();
        if (move_on){
          move_on = false;
          state = 1;
        }
      }else if (!wifi.isBusy() && iot_last_update >IOT_UPDATE_INTERVAL) { //if wifi no busy and has been a while
        String total_query = "kerberos="+KERBEROS; //build query string
        wifi.sendRequest(GET, "iesc-s2.mit.edu", 80, path, total_query); //make request
        iot_last_update=0;  //reset timer
      } 
    }else if(state==1){
      if (button ==1){
        query_string = query_string+alphabet.substring(char_index,char_index+1);
        char_index = 0;
        oled.clearBuffer();
        pretty_print(0,20,query_string + alphabet.substring(0,1),10,20,0,oled); //print buiding string
        oled.sendBuffer();
      }else if(button==2){
        state = 2;
        oled.clearBuffer();
        pretty_print(0,20,"",10,20,0,oled); //print buiding string
        oled.sendBuffer();
      }else{
        if(angle<-1*angle_threshold && scrolling_timer >scrolling_threshold){
          if (char_index ==0) char_index = alphabet.length();
          char_index--;
          scrolling_timer=0;
        }else if(angle>angle_threshold && scrolling_timer >scrolling_threshold){
          char_index++;
          char_index %=alphabet.length();
          scrolling_timer=0;
        }
        String to_print = query_string + alphabet.substring(char_index,char_index+1);
        oled.clearBuffer();
        pretty_print(0,20,to_print,10,20,0,oled); //print buiding string
        oled.sendBuffer();
      }
    }else if(state==2){
      if (!wifi.isBusy()) {
        String output_string = "";
        vigenere_cipher(query_string,output_string,KEY,true);
        String total_query = "kerberos="+ PARTNER_KERBEROS +"&sender="+KERBEROS+ "&message="+query_string;
        Serial.println(total_query);
        wifi.sendRequest(POST, "iesc-s2.mit.edu", 80, path, total_query);
        state = 3;
        query_string = "";
      }
    }else if(state==3){
      if ( wifi.hasResponse()) {
        message = wifi.getResponse();
        int htmlindex = message.indexOf("</html>");
        message = message.substring(6,htmlindex);
        state=0;
        oled.clearBuffer();
        pretty_print(0,20,message,10,20,0,oled); //print buiding string
        oled.sendBuffer();
      }else{
        oled.clearBuffer();
        pretty_print(0,20,"Waiting for response",10,20,0,oled); //print buiding string
        oled.sendBuffer();
      }
    } 
  }
};


SlackSystem slacker;
Button button(BUTTON_PIN);


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("HIHI");
  Wire.begin();
  SPI.setSCK(14);
  oled.begin();
  Serial.println("HIHI");
  pinMode(BUTTON_PIN, INPUT_PULLUP);//set up pin!
  delay(100);
  setup_angle();
  Serial.println("HIHI");
  wifi.begin();
  wifi.connectWifi("6s08", "iesc6s08");
  oled.setFont(u8g2_font_10x20_mr); //small, stylish font
}

void loop() { 
  float x,y;
  get_angle(x,y); //get angle values
  int bv = button.update(); //get button value
  slacker.update(y,bv); //input: angle and button, updates OLED
}




void setup_angle(){
  Serial.print("Check IMU:");
  char c = imu.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  Serial.print("MPU9250: "); Serial.print("I AM "); Serial.print(c, HEX);
  Serial.print(" I should be "); Serial.println(0x73, HEX);
  if (c == 0x73){
    imu.MPU9250SelfTest(imu.selfTest);
    imu.initMPU9250();
    imu.calibrateMPU9250(imu.gyroBias, imu.accelBias);
    imu.initMPU9250();
    imu.initAK8963(imu.factoryMagCalibration);
  } // if (c == 0x73)
  else
  {
    while(1) Serial.println("NOT FOUND"); // Loop forever if communication doesn't happen
  }
    imu.getAres();
    imu.getGres();
    imu.getMres();
}

void get_angle(float&x, float&y){
  imu.readAccelData(imu.accelCount);
  x = imu.accelCount[0]*imu.aRes;
  y = imu.accelCount[1]*imu.aRes;
}



