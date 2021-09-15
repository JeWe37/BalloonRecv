#include <Arduino.h>

#include <ArduinoWebsockets.h>
#define RADIOLIB_EXCLUDE_HTTP

#include <RadioLib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "rs.hpp"


//#define DEBUG

#define BT            0               // user button

#define LED           2               // green LED
#define LED2          12              // red LED

#define LORA_CS         5             // LORA_DEFAULT_SS_PIN 
#define LORA_RESET      17            // LORA_DEFAULT_RESET_PIN
#define LORA_DIO0       16            // LORA_DEFAULT_DIO0_PIN

#define frequency 868.525
#define bw 250.0
#define spread 12
#define codeRate 8
#define syncWord 0x37

const char* ssid     = "AstronomieAG-AP";
const char* password = "Archigymnasium";

IPAddress local_IP(192,168,37,1);
IPAddress gateway(192,168,37,2);
IPAddress subnet(255,255,255,0);

WebServer server(80);
DNSServer dnsServer;

using namespace websockets;

WebsocketsServer ws;

WebsocketsClient clients[4+1];

int clientCnt = 0;

int idx;

namespace data {
  template<typename T, typename ...Ts> struct setTuple;

  template<typename T, typename ...Ts> struct setTuple {
    static constexpr size_t size() {
      return sizeof(T)+setTuple<Ts...>::size();
    }

    union {
      unsigned char arr[size()];
      struct {
        T val;
        setTuple<Ts...> tup;
      } __attribute__((packed));
    };

    String values() const {
      return String(val)+","+tup.values();
    }
  };

  template<typename T> struct setTuple<T> {
    static constexpr size_t size() {
      return sizeof(T);
    }

    union {
      unsigned char arr[size()];
      T val;
    };

    String values() const {
      return String(val);
    }
  };
  
  setTuple<long, float, float, float, float, int, int, float, float, float, float, float, float, float, float, float, float, float, float, int32_t, int32_t, float, float, float, uint32_t> values;
}


#ifdef DEBUG
int t = 0;
bool flag = false;
float lat = 515711000;
int cnt = 0;
#else
// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;
#endif

#ifndef DEBUG
#define MAX_PACKET_SIZE 64

#define MIN(a,b) (((a)<(b))?(a):(b))

void getData();

namespace gen {
  template<bool B, class T = void> struct enable_if {};

  template<class T> struct enable_if<true, T> {
    typedef T type;
  };
}

namespace impl {
  using seqType = uint16_t;

  template<int32_t N, typename Enable = void> struct calcPacketNum {
    static constexpr size_t value = calcPacketNum<N+sizeof(seqType)-MAX_PACKET_SIZE>::value+1;
  };

  template<int32_t N> struct calcPacketNum<N, typename gen::enable_if<N <= 0>::type> {
    static constexpr size_t value = 0;
  };
}

template<size_t len, size_t ecclen = 8> class receiver {
  template<size_t al, size_t numLong, size_t elems> struct rotArr {    
    static constexpr size_t arr_len = al+(numLong != 0);
  private:
    
    byte arr[elems][arr_len];

    size_t currElem = 0;

    size_t l = 0;
  public:
    void add(byte vals[arr_len]) {
      if (l < elems)
        l++;
      memcpy(arr[currElem++], vals, arr_len);
      currElem %= elems;
    }

    void reset() {
      l = 0;
    }

    bool valid() {
      return l == elems;
    }

    void read(byte* vals) {
      for (int i = 0; i < elems; ++i)
        memcpy(vals+i*al+MIN(i, numLong), arr[(currElem+i)%elems], al+(numLong > i));
    }
  };
  
  static constexpr size_t packetNum = impl::calcPacketNum<len+ecclen>::value;

  static_assert(packetNum == 2, "packetNum wrong!");

  static constexpr size_t encLen = packetNum*sizeof(impl::seqType)+ecclen+len;

  static_assert(encLen == 112, "encLen wrong!");

  static constexpr size_t baseLen = encLen/packetNum;

  static_assert(baseLen == 56, "baseLen wrong!");

  static constexpr size_t addOne = encLen%packetNum;

  static_assert(addOne == 0, "addOne wrong!");

  RS::ReedSolomon<encLen-ecclen, ecclen> rs;

  rotArr<baseLen, addOne, packetNum> lastPackets;

  static constexpr size_t actualLen = decltype(lastPackets)::arr_len;

  static_assert(actualLen == 56, "actualLen wrong!");

  struct slice {
    byte* beg;
    size_t l;
    size_t pos;
  };

  static slice getNth(byte* arr1, unsigned n) {
    return {arr1+n*baseLen+MIN(n, addOne), baseLen+(addOne > n)-sizeof(impl::seqType)-((n == packetNum-1)*ecclen), n*baseLen+MIN(n, addOne)-n*sizeof(impl::seqType)};
  }

  bool decode(byte vals[actualLen]) {
    lastPackets.add(vals);
    if (!lastPackets.valid())
      return false;
    byte encoded[encLen], decoded[encLen-ecclen];
    lastPackets.read(encoded);
    rs.Decode(encoded, decoded);
    for (int i = 0; i < packetNum; i++) {
      slice s = getNth(decoded, i);
      if (*reinterpret_cast<impl::seqType*>(s.beg)%packetNum != i)
        return false;
      memcpy(reinterpret_cast<byte*>(&data::values)+s.pos, s.beg+sizeof(impl::seqType), s.l);
    }
    lastPackets.reset();
    return true;
  }
public:
  SX1276 lora = new Module(LORA_CS, LORA_DIO0, LORA_RESET);
  
  void setupLora() {
    Serial.println("Setting up SX1276");
    int ret = lora.begin(frequency, bw, spread, codeRate, syncWord, 10, 8);  //..., currentLimit, preambleLength, TCXOVoltage
    Serial.println("[SX1276] Begun ... ");
    if (ret != ERR_NONE) {
      printf(PSTR("Error starting SX1276: %d\n"), ret);
      while (true);
    }
    if (lora.setCRC(false) != ERR_NONE) {
      printf(PSTR("Error disabling CRC\n"));
      while (true);
    }
    lora.setDio0Action(getData);
    restartRecv();
  }

  void restartRecv() {
    Serial.println(F("[SX1276] Starting to listen ... "));
    int ret = lora.startReceive();
    if (ret == ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(ret);
      while (true);
    }
  }

  friend void getData();
};

static_assert(data::values.size() == 100, "Wrong size!");

receiver<data::values.size()> recv;

void getData() {    
  Serial.println("Got data");
  
  if (!enableInterrupt)
    return;
  
  enableInterrupt = false;

  byte dat[decltype(recv)::actualLen];
  int ret = recv.lora.readData(dat, recv.lora.getPacketLength());
  if (ret != ERR_NONE) {
    Serial.print("Got error while reading data, code ");
    Serial.println(ret);
  }
  if (recv.decode(dat)) {
    Serial.println("received successfully!");
    receivedFlag = true;
  }

  recv.restartRecv();
  
  enableInterrupt = true;
}
#endif

void rmClient(int j) {
  for (int i = j; i < clientCnt; ++i)
    clients[i] = clients[i+1];
  clientCnt--;
  if (idx >= j)
    idx--;
  Serial.println("Client disconnected now " + String(clientCnt) + " clients");
}

String pack = "No data";
int rssi = 0;
float snr = 0;
long freqErr = 0;

size_t datPos = 0;

void setup() {
  pinMode(LED,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(BT,INPUT_PULLUP);

  Serial.begin(115200);
  while (!Serial);
#ifndef DEBUG
  recv.setupLora();
#endif
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    digitalWrite(LED2,HIGH);
    while (true);
  }

  WiFi.mode(WIFI_AP);

  WiFi.persistent(false);

  if (!WiFi.softAP(ssid, password)) {
    Serial.println("An Error has occurred while starting access point");
    digitalWrite(LED2,HIGH);
    while (true);
  }

  while(!(WiFi.softAPIP() == local_IP)){
    WiFi.softAPConfig(local_IP, gateway, subnet);    
  }

  Serial.println(WiFi.softAPIP());

  if (!dnsServer.start(53, "www.astronomieag.edu", local_IP)) {
    Serial.println("An Error has occurred while starting DNS server");
    digitalWrite(LED2,HIGH);
    while (true);
  }
  
  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/index.js", SPIFFS, "/index.js");
  server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
  server.serveStatic("/moment.min.js", SPIFFS, "/moment.min.js");
  server.serveStatic("/chart.js", SPIFFS, "/chart.js");
  server.serveStatic("/index.css", SPIFFS, "/index.css");
  server.serveStatic("/mapbox-gl.css", SPIFFS, "/mapbox-gl.css");
  server.serveStatic("/mapbox-gl.js", SPIFFS, "/mapbox-gl.js");
  server.serveStatic("/style.js", SPIFFS, "/style.js");
  server.serveStatic("/getData", SPIFFS, "/data.txt");
  server.on("/font/OSR/{}", []() {
    File f = SPIFFS.open("/font/OSR/"+server.pathArg(0)+".gz");
    server.streamFile(f, "");
    f.close();
  });
  server.on("/countries/{}/{}/{}", []() {
    File f = SPIFFS.open("/countries/"+server.pathArg(0)+"/"+server.pathArg(1)+"/"+server.pathArg(2)+".gz");
    server.streamFile(f, "");
    f.close();
  });
  server.onNotFound([]() {
    server.send(404, "text/html", "<h1>404: Not Found</h1>");
  });

  server.begin();

  ws.listen(8080);
  digitalWrite(LED, HIGH);
}

void loop() {
  if (digitalRead(BT) == LOW && SPIFFS.exists("/data.txt")) {
    SPIFFS.remove("/data.txt");
    Serial.println("Removed data.txt");
  }
  
#ifndef DEBUG
  if (receivedFlag) {
    receivedFlag = false;
    rssi = recv.lora.getRSSI();
    snr = recv.lora.getSNR();
    freqErr = recv.lora.getFrequencyError();
    Serial.print("[RSSI: " + String(rssi) + "; SNR: " + String(snr) + "dB; FreqErr: " + String(freqErr) + "Hz]: ");

    String pack = data::values.values();
    Serial.println(pack);

    File f = SPIFFS.open("/data.txt", "a+");
    f.println(pack);
    f.close();
  
    StaticJsonDocument<256> dat;
    dat["rssi"] = rssi;
    dat["snr"] = snr;
    dat["freqErr"] = freqErr;
    dat["data"] = pack;
#else
  if (millis()-t > 5000) {
    t = millis();
    flag = !flag;
    lat -= 100000;
    cnt++;
    if (flag)
      pack = "1500,1.5,2.5,2.5,20.5,21.5,90001.5,1001.5,20.5,1.5,2.5,3.5,4.5,0.5,1.5,2.5,3.5,4.5,2.25,81058000,"+String(lat)+",999.5,270.5,45.5,"+String(1581273187+cnt*5);
    else
      pack = "2000,7.5,6.5,6.5,21.5,20.5,89000.5,990.5,21.5,4.5,3.5,2.5,1.5,4.5,3.5,2.5,1.5,0.5,2.75,81059000,"+String(lat)+",1000.5,260.5,40.5,"+String(1581273187+cnt*5);

    File f = SPIFFS.open("/data.txt", FILE_APPEND);
    f.println(pack);
    f.close();
      
    StaticJsonDocument<256> dat;
    dat["rssi"] = millis();
    dat["snr"] = 3.1415;
    dat["freqErr"] = micros();
    dat["data"] = pack;
#endif
    String jsStr;
    serializeJson(dat, jsStr);

    for (int i = 0; i < clientCnt; ++i)
      clients[i].send(jsStr);
  }
    
  dnsServer.processNextRequest();

  if (ws.poll()) {
    clients[clientCnt] = ws.accept();
    clients[clientCnt++].onEvent([](WebsocketsClient& cl, WebsocketsEvent evnt, WSInterfaceString str) {
      switch (evnt) {
      case WebsocketsEvent::GotPing:
        cl.pong(str);
        break;
      case WebsocketsEvent::ConnectionClosed:
        rmClient(idx);
      }
    });
    Serial.println("Client connected now " + String(clientCnt) + " clients");
  }
  for (idx = 0; idx < clientCnt; ++idx)
    clients[idx].poll();

  server.handleClient();
  delay(1);
}
