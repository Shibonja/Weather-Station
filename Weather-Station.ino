#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <string.h>
#include <dht11.h>
 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 128, 1, 222);
 
//REST API settings
#define apiKey                "937f0acbde5e456eb9834ba7e1d7ac60" 
#define thingId               "d9663163ee0f11e39da4b761b39ec13a"
#define tempraturePropertyId  "ee3e3106ee0f11e39da4b761b39ec13a"
#define humidityPropertyId    "f6a5d557ee0f11e39da4b761b39ec13a"
#define url                   "api.gadgetkeeper.com"
 
#define sendPeriod 10000      //API calling frequency in milliseconds
#define DHT11_PIN 2           //DHT11 sensor connected pin
#define PORT  80              //web request port
 
dht11 dht;
EthernetClient client;
HttpClient http(client);
float currentTemperature, currentHumidity;
 
void setup(){
    Serial.begin(9600);
    while (!Serial){
        ; // wait for serial port to connect. Needed for Leonardo only
    }
   
    Serial.println("Initializing system..");  //start the Ethernet connection:
    if (Ethernet.begin(mac) == 0){
        Serial.println("Failed to configure Ethernet using DHCP");
        Ethernet.begin(mac, ip);
    }
    delay(1000);  //give the Ethernet shield a second to initialize:
}
 
//API request handling
int apiCall(float value , const char*  propertyId){
  char msgBuffer[150], valueBuffer[10], msgContentLength = 0;
  int result;    
  memset(msgBuffer , 0 , 150);
  sprintf(msgBuffer , "/v1/things/%s/properties/%s/value.json" , thingId, propertyId);
  //Serial.println(msgBuffer);
  http.beginRequest();
  result = http.startRequest(url, PORT, msgBuffer, HTTP_METHOD_PUT, "Arduino");
  if(result == HTTP_ERROR_API){
      Serial.println("API error");
      http.endRequest();        
      http.stop();
      return 1;        
  }else if(result == HTTP_ERROR_CONNECTION_FAILED){
      Serial.println("Connection failed");    
      http.endRequest();    
      http.stop();
      return 1;                        
  }else if( result == HTTP_SUCCESS ){
      memset( msgBuffer , 0 , 50 );
      sprintf( msgBuffer , "X-ApiKey: %s" , apiKey );
      http.sendHeader(msgBuffer);
      http.sendHeader("Content-Type: text/json; charset=UTF-8");
      memset( valueBuffer , 0 , 10 );
      //sprintf( valueBuffer , "%d\r\n" , value );  //send int data
      dtostrf(value, 4, 2, valueBuffer);  //send float data (xx.xx format conversion)
      valueBuffer[strlen(valueBuffer)] = '\r';
      valueBuffer[strlen(valueBuffer)+1] = '\n';
      msgContentLength = strlen(valueBuffer) - 1 ;
      memset(msgBuffer , 0 , 50);
      sprintf( msgBuffer , "Content-Length: %d" , msgContentLength );
      http.sendHeader(msgBuffer);
      http.write((const uint8_t*) valueBuffer , msgContentLength + 2 );
      http.endRequest();
      http.stop();
      Serial.print("Sending value : ");                        
      Serial.println(valueBuffer);        
      return 0;
  }else{
      http.endRequest();    
      http.stop();                                    
      return 1;
  }        
}
 
//DHT11 sensor query
int querySensor(){
    int test;
    Serial.print("DHT11, \t");
    test = dht.read(DHT11_PIN);
    switch (test){
      case DHTLIB_OK:  
          Serial.print("OK\t"); 
          currentTemperature = dht.temperature;
          currentHumidity = dht.humidity;
          Serial.print(currentTemperature);
          Serial.print("C,\t");
          Serial.print(currentHumidity);
          Serial.println("RH");
          return 0;
          break;
      case DHTLIB_ERROR_CHECKSUM: 
          Serial.println("Checksum error");
          return 1; 
          break;
      case DHTLIB_ERROR_TIMEOUT: 
          Serial.println("Time out error");
          return 1; 
          break;
      default: 
          Serial.println("Unknown error"); 
          return 1;
          break;
    }  
}
 
void loop(){ 
    if(!querySensor()){
      Serial.println("connecting...");
      for(int i=0; i<2; i++){
        int progress; 
        if(i==0){
            progress = apiCall(currentTemperature , tempraturePropertyId);
            Serial.print("Sending temprature: ");
        }else if(i==1){
            progress = apiCall(currentHumidity , humidityPropertyId);
            Serial.print("Sending humidity: ");
        }
        if(!progress){
            Serial.println("OK ");
        }else{
            Serial.println("Fail");  
        } 
      }
    }else{
        Serial.println("Skip api request");  
    }
    delay(sendPeriod);
}
