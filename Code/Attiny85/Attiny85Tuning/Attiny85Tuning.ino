#include <SoftwareSerial.h>

#define comRx 1
#define comTx 0

#define PRINT_TIMES 1000
#define Serial compSer

SoftwareSerial compSer = SoftwareSerial(comRx, comTx);

void setup() {
  Serial.begin(9600);

  double osc = OSCCAL;
  Serial.print("Original OSCCAL = ");
  Serial.println(OSCCAL);
  delay(5000);
  
  for(int i=floor(osc*0.8)-5;i<ceil(osc*1.2)+5;i++){
    OSCCAL = i;
    for(int j=0;j<PRINT_TIMES;j++){
      Serial.print("OSCCAL = ");
      Serial.println(i);
    }
  }
}

void loop() {}
