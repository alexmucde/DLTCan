#include <WCan.h>
#include <WTimer.h>

WCan can;
//WTimer timer;

char msgString[128];                        

byte data[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

void setup() {
  Serial.begin(115200);
  
  if(can.setup()==true)
  {
    Serial.write(0x7f); // Start of messages
    Serial.write(0x00); // Init OK
     ;//Serial.println("Initializing MCP2515 ok");  
  }
  else
  {
    Serial.write(0x7f); // Start of messages
    Serial.write(0xff); // Error Init
     ;//Serial.println("Error Initializing MCP2515 failed");
  }
  
  //timer.start(100);
}

void loop() 
{ 
  switch(can.event())
  {
    case WCan::Received:
           
      if((can.getId() & 0x80000000) == 0x80000000)     // Determine if ID is standard (11 bits) or extended (29 bits)
      {
        ;//sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (can.getId() & 0x1FFFFFFF), can.getLength());
        Serial.write(0x7f); // Start of messages
        Serial.write(0x81); // Extended Msg
        Serial.write(can.getLength()); // Msg
        Serial.write((can.getId()>>24)&0xff); // Id High Byte
        Serial.write((can.getId()>>16)&0xff); // Id High Byte
        Serial.write((can.getId()>>8)&0xff); // Id High Byte
        Serial.write(can.getId()&0xff); // Id Low Byte
        for(int num=0;num<can.getLength();num++)
        {
            Serial.write(can.getData()[num]); // Msg          
            if(can.getData()[num]==0x7f)
              Serial.write(0x7f); 
        }
      }
      else
      {
        ;//sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", can.getId(), can.getLength());
        Serial.write(0x7f); // Start of messages
        Serial.write(0x80); // Msg
        Serial.write(can.getLength()); // Msg
        Serial.write((can.getId()>>8)&0xff); // Id High Byte
        Serial.write(can.getId()&0xff); // Id Low Byte
        for(int num=0;num<can.getLength();num++)
        {
          Serial.write(can.getData()[num]); // Msg          
          if(can.getData()[num]==0x7f)
            Serial.write(0x7f); 
       }
      }
      ;//Serial.print(msgString);
    
      if((can.getId() & 0x40000000) == 0x40000000){    // Determine if message is a remote request frame.
        ;//sprintf(msgString, " REMOTE REQUEST FRAME");
        ;//Serial.print(msgString);
      } else {
        for(byte i = 0; i<can.getLength(); i++){
          ;//sprintf(msgString, " 0x%.2X", can.getData()[i]);
          ;//Serial.print(msgString);
        }
      }
          
      ;//Serial.println();
      break;
  }

/*  switch(timer.event())
  {
    case WTimer::Expired:
      if(can.send(0x100,data)==true)
      {
        Serial.write(0x7f); // Start of messages
        Serial.write(0x01); // Send OK
          ;//Serial.println("Send succesful");  
      }
      else
      {
        Serial.write(0x7f); // Start of messages
        Serial.write(0xfe); // Error Send
           ;//Serial.println("Error sending message");
      }
      timer.start(100);
      break;      
  }*/
}
