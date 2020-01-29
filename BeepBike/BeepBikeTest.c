#include <Wire.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_PN532.h>


//Copyright 2017 zhongfu & ttwj
//Beep Technologies Pte Ltd

static const char hexMap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

// CEPAS command constants
static const uint8_t selectMf[] = { 0x00, 0xa4, 0x00, 0x00, 0x02, 0x3f, 0x00, 0x00 }; // size 8, to select ezlink master file
static const uint8_t selectEf[] = { 0x00, 0xa4, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00 }; // size 8, to select ezlink elementary file
static const uint8_t getChal[] = { 0x00, 0x84, 0x00, 0x00, 0x08 }; // size 6, get challenge
static const uint8_t readPurseAuth[] = { 0x90, 0x32, 0x03, 0x00, 0x0a, 0x15, 0x02, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0x71 }; // size 16, authed read purse
// note: termRand is hardcoded in readPurseAuth (ab cd ef 01 23 45 67 89). should be changed in prod

// I/O pins for communication
#define PN532_SS   (5)
#define BUZZ_PIN   (10)

// WiFi credentials
const char* ssid     = "beepbike";
const char* password = "596d566c63413d3d";

// Server details
const String host = "test-bike.beepbeep.tech";
const uint16_t port = 8181;

Adafruit_PN532 nfc(PN532_SS); // hw spi for esp8266

void setup() {
  // Pin 10 on ESP8266 is high by default; we need to shut it up
  tone(BUZZ_PIN, 2000);
  noTone(BUZZ_PIN);

  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // This halts the ESP8266
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443B Card ...");
}

void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  success = nfc.readPassiveTargetID(PN532_ISO14443B, uid, &uidLength); // Blocks until 14443B card is detected
  if (!success) {
    Serial.println("Read card failed for whatever reason");
    return;
  }

  // Display some basic information about the card
  Serial.println("Found an ISO14443B card");
  Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
  Serial.print("  UID Value: ");
  nfc.PrintHex(uid, uidLength);
  Serial.println();

  // Variables used to hold responses from PN532
  uint8_t response[250];
  uint8_t responseLength;

  // select 14443B master file
  Serial.println("doing selectMf");
  success = nfc.inDataExchange(selectMf, 8, response, &responseLength);
  if (!success) {
    Serial.println("selectMf failed");
    return;
  }
  Serial.println();

  // select 14443B elementary file
  Serial.println("doing selectEf");
  success = nfc.inDataExchange(selectEf, 8, response, &responseLength);
  if (!success) {
    Serial.println("selectEf failed");
    return;
  }
  Serial.println();

  // get card random
  uint8_t challenge[8];
  Serial.println("doing getChal");
  success = nfc.inDataExchange(getChal, 5, response, &responseLength);
  if (!success) {
    Serial.println("getChal failed");
    return;
  } else {
    memcpy(challenge, response, 8); // store the getChallenge response + sw1,2
  }
  Serial.println();

  // get purse data
  uint8_t purseLen;
  Serial.println("doing readPurseAuth");
  success = nfc.inDataExchange(readPurseAuth, 16, response, &purseLen);
  if (!success) {
    Serial.println("readPurseAuth failed");
    return;
  }
  Serial.println();
  uint8_t purseData[purseLen];
  memcpy(purseData, response, purseLen); // store readPurse response + sw1,2

  // send the cardrand + purse data to the server in a HTTP POST request to http://host:port/transaction_final
  // where POST data is "purseData=xxxx&getChallenge=yyyy". xxxx is 73 bytes i.e. 146 hex chars. yyyy is 10 bytes/20 hex chars. 189 bytes total.
  String payload;
  HTTPClient http;
  http.setTimeout(30000);
  success = http.begin(host, port, "/transaction_final"); // open TCP connection only
  if (!success) {
    Serial.println("http connection init failed");
    return;
  }

  // build payload (POST data, see above)
  payload += "purseData=";
  hexAppend(&payload, purseData, purseLen); // convert raw bytes to ASCII representation in hex
  payload += "&getChallenge=";
  hexAppend(&payload, challenge, 8); // same as above
  Serial.println(payload);

  success = http.POST(payload); // actually send the POST to the server
  if (success != HTTP_CODE_OK) {
    Serial.printf("http post failed: %d: ", success); Serial.println(http.errorToString(success));
    return;
  }
  Serial.print("http post: "); Serial.println(success, DEC);

  // parse the debit cryptogram from the server into raw bytes
  String deductStr = http.getString();
  const char *hin = deductStr.c_str();
  int clen = deductStr.length()/2;
  uint8_t deduct[clen];
  for (int i=0; i < 2*clen; i+=2) {
    deduct[i/2] = dehex(hin[i])<<4 | dehex(hin[i+1]);
  }

  // send debit cryptogram to card
  Serial.println("doing deduct");
  success = nfc.inDataExchange(deduct, clen, response, &responseLength);
  if (!success) {
    Serial.println("deduct failed");
    return;
  }
  Serial.println();

  // print out debit receipt cryptogram to serial
  // we'll need to send this back to the server so that ezlink (and etc) can credit us the amount deducted from the card
  for (uint8_t i = 0; i < responseLength; i++)
  {
          if (i > 0)
          {
              Serial.print(":");
          }
          Serial.printf("%02X", response[i]);
  }

  // delay so we don't accidentally deduct multiple times
  delay(1000);
}

// converts raw bytes to ascii hex representation, then appends to a string
void hexAppend(String * str, uint8_t * hex, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    *str += hexMap[hex[i] >> 4];
    *str += hexMap[hex[i] & 0x0f];
  }
}

// converts ascii hex representation to raw bytes
byte dehex(char c) { // Get nibble value 0...15 from character c
  // Treat digit if c<'A', else letter
  return c<'A'? c & 0xF : 9 + (c & 0xF);
  // Above assumes that c is a 'hex digit' in 0...9, A or a ... F or f.
  // It would make more sense to just use 16 consecutive characters,
  // like eg 0123456789:;<=>? or @ABCDEFGHIJKLMNO so the above
  // could just say `return c & 0xF;`
}

