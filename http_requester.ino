#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>;
#include <StandardCplusplus.h>
#include <vector>


//Constants
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define INTERVAL 10000

class Param{
  public:
  char* param;
  char* value;
  
  Param(char* p_param, char* p_value){
    param = p_param;
    value = p_value;
  }

  char* toString(){
    char buffer[256];
    sprintf(buffer, "%s=%s", param, value);
    return buffer;
  }
};

class PostRequest {
  public:
  EthernetClient client;
  char* host;
  int port;
  char* route;
  std::vector<Param> params;

  PostRequest(EthernetClient p_client, char* p_host, char* p_route){
    client = p_client;
    host = p_host;
    route = p_route;
    port = 80;
  }

  void addParam(Param p_param){
    params.push_back(p_param);
  }

  void setPort(int p_port){
    port = p_port;
  }

  boolean send(){
    client.stop();
    Serial.print("connecting.. ");
    if (client.connect(host, port)) {
      char request[1024];
      sprintf(request, "POST %s HTTP/1.1\n"
                       "User-Agent: arduino-http\n"
                       "Connection: close\n"
                       "Content-Type: application/x-www-form-urlencoded\n"
                       "Content-Length: %i\n\n"
                       "%s\n", route, strlen(paramString()), paramString());
                       
      Serial.print("making request with size ");
      Serial.println(strlen(request));
      client.print(request);
      return true;
    } else {
      Serial.println("connection failed");
      return false;
    }
  }

  char* paramString(){
    char buffer[1024];
    memset(buffer,0,1024);
    for(int i = 0; i < params.size() ; i++){
      strcat(buffer, params[i].toString());
      strcat(buffer, "&");
    }
    return buffer;
  }
};

// Inicializacion del ethernet
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192,168,1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


// variables
EthernetClient client;
char server[] = "192.168.1.100";
unsigned long lastConnectionTime = 0;
DHT dht(DHTPIN, DHTTYPE); //// Termohigrometro


void setup() {
  Serial.begin(9600);
  delay(1000); // tiempo para que bootee el shield
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
  Serial.print("Board IP: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  //recibir data y volcarla en el serial
  if (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  if (millis() - lastConnectionTime > INTERVAL) {
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();
    boolean result = logValues(hum,temp);
    if(result)
      lastConnectionTime = millis();
  }
}

boolean logValues(float hum, float temp) {
  char hum_s[32];
  char temp_s[32];
  dtostrf(hum, 0, 2, hum_s);
  dtostrf(temp, 0, 2, temp_s);
  
  PostRequest req = PostRequest(client, "192.168.1.100", "/lecture");
  req.setPort(4567);
  req.addParam(Param("hum",hum_s));
  req.addParam(Param("temp",temp_s));
  
  return req.send();
}

