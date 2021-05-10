#include <WCan.h>
#include <WTimer.h>
#include <WSerial.h>

WCan can;
WTimer timer;
WSerial serial(WSerial::Binary);

char msgString[256];
unsigned char canMessage[256];                        

void setup() {
  serial.setup();
  
  if(can.setup()==true)
  {
    Serial.write(0x7f); // Start of messages
    Serial.write(0x00); // Init OK
  }
  else
  {
    Serial.write(0x7f); // Start of messages
    Serial.write(0xff); // Error Init
  }
  
  timer.start(100);
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
              Serial.write(0x7f); // add stuff byte to be able to detect unique header
        }
      }
      else
      {
        Serial.write(0x7f); // Start of messages
        Serial.write(0x80); // Msg
        Serial.write(can.getLength()); // Msg
        Serial.write((can.getId()>>8)&0xff); // Id High Byte
        Serial.write(can.getId()&0xff); // Id Low Byte
        for(int num=0;num<can.getLength();num++)
        {
          Serial.write(can.getData()[num]); // Msg          
          if(can.getData()[num]==0x7f)
            Serial.write(0x7f); // add stuff byte to be able to detect unique header 
       }
      }
    
      if((can.getId() & 0x40000000) == 0x40000000){    // Determine if message is a remote request frame.
      } else {
        for(byte i = 0; i<can.getLength(); i++){
        }
      }
          
      ;//Serial.println();
      break;
  }

  switch(serial.event())
  {
    case WSerial::Connected:
      break;
    case WSerial::Disconnected:
      break;
    case WSerial::Line:
      break;
    case WSerial::Data:
      int length;
      unsigned char *data = serial.getData(length);
      if(length>=1)
      {
        if(data[0]==0x7f)
        {
          if(length>=2)
          {
            if(data[1]==0x80)
            {
              if(length>=3)
              {
                int msgLength = data[2];
                if(length>=(5+msgLength))
                {
                  unsigned short id = ((unsigned short)data[3]<<8)|data[4];
                  int pos=0;
                  bool stuffFound=false;
                  for(int num=5;num<length;num++)
                  {
 /*                   if(data[num]==0x7f)
                    {
                      if(stuffFound)
                      {
                        stuffFound=false;
                        canMessage[pos++]=data[num];                      
                      }
                      else
                      {
                        stuffFound=true;
                      }
                    }
                    else*/
                    {
                      canMessage[pos++]=data[num];
                    }
                    if(pos>=msgLength)
                      break; // full message found
                  }
                  if(pos<msgLength)
                    break; // no full message yet found
                  if(can.send(id,canMessage,msgLength)==true)
                  { 
                    Serial.write(0x7f); // Start of messages
                    Serial.write(0x01); // Send OK
                  }
                  else
                  {
                    Serial.write(0x7f); // Start of messages
                    Serial.write(0xfe); // Error Send
                  }
                  serial.clearData();                        
                }
              }
            }
            else
            {
              serial.clearData();          
            }
          }
        }
        else
        {
          serial.clearData();          
        }
      }
      break;
  }
  
  switch(timer.event())
  {
    case WTimer::Expired:
      Serial.write(0x7f); // Start of messages
      Serial.write(0x02); // Watchdog
      timer.start(1000);
      break;      
  }
}
