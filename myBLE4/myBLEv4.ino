// myBLEv4a
// yet another segMod myBLE rewrite

// focus on wifi and ota update to ease development (works great!)
// changed board to Waveshare ESP32-S3-Zero (ESP32-S3FH4R2) for added PSRAM
//
// issues with Serial0 (my previous module used a CP2102 for connection, this one native USB)
// waveshare says 'The "TX" and "RX" markings on the board indicate the default UART0 pins
// for ESP32-S3-Zero. Specifically, TX is GPIO43, and RX is GPIO44' but I couldn't get any output.
// also, "ESP32-S3 Boards with Native USB Not Booting without USB Host Serial"

#define wifiEnable yes
#define ESPLED yup
const byte MY_ECU = 0x3f;

const int maxECU = 8;                     // number of ECUs to virtualize
const int ECUlen = (0x200 * maxECU * 4);  // 32 bits storage for each of 256, 16bit integers, per ECU
byte *ECUbuf;                             // ECU malloc() as u8
const int ECU32len = (0x200 * maxECU);    // 32 bits storage for each of 256, 16bit integers, per ECU
unsigned long *ECU32buf;                  // ECU as u32
unsigned long *ECUptr;                    // convenience pointer


unsigned int bgECUaddr = 0;


#define ECU_SUB 0x02  // re: ECUbank
#define ECU_MOD 0x07  // re: ECUbank

// BUG disabled below
byte ecu_cmd_acl[] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


//notes to self...
// const variables might need DRAM_ATTR
//RTC_IRAM_ATTR static uint32_t rtcd;
//__NOINIT_ATTR WORD_ALIGNED_ATTR static uint32_t temp[1024];

__NOINIT_ATTR int powerState;

RTC_NOINIT_ATTR WORD_ALIGNED_ATTR byte RTCbuf[(0x200 * maxECU)];  // 4kB
EXT_RAM_NOINIT_ATTR WORD_ALIGNED_ATTR uint32_t PSRbuf[(0x200 * maxECU)];

int PKT_MAX_LEN = 264;
//static byte *TXpkt;                             // ECU malloc() as u8

const int rxBS = 0x20;  // serial interface block transfer size
WORD_ALIGNED_ATTR byte RXbuf[rxBS];
DRAM_ATTR const unsigned long mega = 0x100000;  // 2^20
int LastTXparity = 0;

// numbers relative maxIFB
DRAM_ATTR const int maxIFB = 5;  // number of physical interfaces to route
DRAM_ATTR const int IFBlen = (0x200 * maxIFB);
byte *ifRX;
byte *ifTX;

//typedef DRAM_ATTR unsigned long u32;


int ifRXlen[maxIFB] = { 0 };
int ifTXlen[maxIFB] = { 0 };
int ifErr[5] = { 0 };

unsigned long timeTXecu[maxECU + 1] = { 0 };
unsigned long timeRXecu[maxECU + 1] = { 0 };
unsigned long timeTXifb[maxIFB + 1] = { 0 };
unsigned long timeRXifb[maxIFB + 1] = { 0 };
int timeTXdelay[maxECU + 1] = { 0 };

int sniffDump = 0;

// convenience pointer
byte *RXptr;
int *RXlen;
//static byte *TXptr;  // convenience pointer

byte TXpkt[0x109] = { 0x5a, 0xa5 };
byte *TXpktBuf = &TXpkt[2];
byte *TXpktLen = &TXpkt[2];
byte *TXpktSrc = &TXpkt[3];
byte *TXpktDst = &TXpkt[4];
byte *TXpktCmd = &TXpkt[5];
byte *TXpktArg = &TXpkt[6];
byte *TXpktDat = &TXpkt[7];

const byte xMOD[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte xHB1[] = { 0x00, 0x23, 0x16, 0x7a, 0x00 };
const byte xHB2[] = { 0x00, 0x23, 0x16, 0x7b, 0x02 };

const byte xTurnOn[] = { 0x02, MY_ECU, 0x16, 0x79, 0x00, 0x01, 0x00 };
const byte xTurnOff[] = { 0x02, MY_ECU, 0x16, 0x79, 0x00, 0x02, 0x00 };
const byte xDeepSleep[] = { 0x02, MY_ECU, 0x07, 0x7a, 0x00, 0x00, 0x01 };


int fullRead = 1;
int temper = 0;
int tempad = 0;
int tempac = 7;



int subCount = 0;
unsigned long subVar[50];  // technical limit of 50 variables in sub packet
int subLen[50];
int subVal[50];

unsigned long volts = 0;
unsigned long temps = 0;

byte *myMap = &ECUbuf[(0x200 * 7 * 4)];

DRAM_ATTR const unsigned long timeTickFast = 1000;
DRAM_ATTR const unsigned long timeTickMid = 1000000;
DRAM_ATTR const unsigned long timeTickSlow = 100000000;

DRAM_ATTR unsigned long timeLastFast = 0;
DRAM_ATTR unsigned long timeLastMid = 0;
DRAM_ATTR unsigned long timeLastSlow = 0;

DRAM_ATTR const byte modECU[8] = { 0x02, 0x04, 0x07, 0x16, 0x23, 0x00, 0x00, 0x00 };
DRAM_ATTR const byte modECUpos[8] = { 0 };

DRAM_ATTR const unsigned long idleRX2TX = 50;  // threshold delay cycles RX to TX

unsigned long int RXinc = 0;
unsigned long int RXbad = 0;
unsigned long int TXyes = 0;
unsigned long int TXno = 0;
/* ECU32 flags

  31    // had read
  30  // had write
  29  // do process
  28  // is dynamic
  27  // writable
  26  // response pending
  25 24 8bit sign,
  8bit usign, flags, 16bit sign, 16bit usign

  3 rw flags 3 age 2 min 2 min AND 1 max 1 max OR 0 cur 0 cur

  31 - 24 flags 23 - 16 15 - 8 what was there 7 - 0 actual data "what should be there"

*/

int ECU_read = (1 << 31);       // had read
int ECU_write = (1 << 30);      // had write
int ECU_changed = (1 << 29);    // did update
int ECU_valChange = (1 << 28);  // did change
int ECU_error = (1 << 27);      // had error
int ECU_bglow = (1 << 26);      // low priority bg reading
int ECU_noRemap = (1 << 24);    // abort processing
int myECU = 0x01;

#include <SPIFFS.h>
#include "x3regs.h"
#include "secrets.h"

#ifdef wifiEnable
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include "html.h"

AsyncWebServer server(80);
AsyncEventSource live("/live");

#include <AsyncWebSerial.h>
AsyncWebSerial webSerial;

#endif

int diagLEDtiming = 2048;
int diagLEDtcount = 0;
int flicker = 0;

int diagLEDc = 0;
int diagLEDp = 1;
int diagLEDs = 1;

// system stuffs
__NOINIT_ATTR int formatting;
int rebooting = 0;
int doLogDst = 1;  // bitmask of log destinations

unsigned long timeOld = 0;
unsigned long timeNow = 0;
unsigned long calLast = 0;

unsigned long timeOut;
unsigned long loopTime = 0;
unsigned long hbDelay = 10000000;

unsigned long tickSlow = 0;
unsigned long tickMid = 0;
unsigned long tickFast = 0;

//#define RXD0 44 // 8 // 14
//#define TXD0 43 // 9 // 15

// GPIO pins to VCU UART
#define RXD1 12  // VCU
#define TXD1 13  // VCU
HardwareSerial VCU(1);

// GPIO pins to BLE UART
#define RXD2 10  // BLE
#define TXD2 11  // BLE
HardwareSerial BLE(2);

#ifdef ESPLED
#include <FastLED.h>
#define NUM_LEDS 17
CRGB leds[NUM_LEDS];
#define FASTLED_LED_OVERCLOCK 1.4  // adjust for your strip
#endif

void IRAM_ATTR doLog(const char *var) {
  if (doLogDst & 0x01) {
    webSerial.print(var);
  }
  if (doLogDst & 0x02) {
    VCU.println(var);
  }
  if (doLogDst & 0x04) {
    BLE.println(var);
  }
  if (doLogDst & 0x08) {
    // ToDo: send "standard" CMD20 log messages like the BLE does
  }
}

void IRAM_ATTR doLog1(const char *var) {
  Serial.println(var);
}

void IRAM_ATTR diagLED(const int tmp) {
  if (tmp < 0) {
    diagLEDc--;
    if (diagLEDc < 1) {
      diagLEDc = 5;
    }
  } else {
    diagLEDc = tmp;
  }

#ifdef ESPLED
  switch (diagLEDc) {
    case 0:
      {
        leds[0].setRGB(0, 0, 0);
        break;
      }  // off
    case 1:
      {
        leds[0].setRGB(0, 0, 4);
        break;
      }  // blue
    case 2:
      {
        leds[0].setRGB(0, 3, 2);
        break;
      }  // purple
    case 3:
      {
        leds[0].setRGB(0, 5, 0);
        break;
      }  // red
    case 4:
      {
        leds[0].setRGB(1, 3, 0);
        break;
      }  // yellow
    case 5:
      {
        leds[0].setRGB(2, 0, 0);
        break;
      }  // green
    case 6:
      {
        leds[0].setRGB(1, 0, 2);
        break;
      }  // cyan
    case 7:
      {
        leds[0].setRGB(2, 3, 2);
        break;
      }  // white
  }
  FastLED.show();
#endif
}

void IRAM_ATTR RTCbackup() {
  unsigned int cksum = 0;
  unsigned int idx = 0;
  while (idx < sizeof(RTCbuf) - 2) {
    RTCbuf[idx] = (ECU32buf[idx] & 0xff);
    cksum += RTCbuf[idx];
    idx++;
  }
  cksum &= 0xffff;
  cksum ^= 0xffff;

  RTCbuf[idx] = (cksum & 0xff);
  RTCbuf[idx + 1] = (cksum >> 8);
}

bool IRAM_ATTR RTCrestore() {
  // sum u8 bytes > u16 word
  unsigned int cksum = 0;
  unsigned int idx = 0;
  while (idx < sizeof(RTCbuf) - 2) {
    cksum += RTCbuf[idx];
    idx++;
  }
  cksum &= 0xffff;
  cksum ^= 0xffff;

  if (((cksum & 0xff) != RTCbuf[idx]) || ((cksum >> 8) != RTCbuf[idx + 1])) {
    doLog("bad RTC");
    return (1);
  } else {
    idx = 0;
    while (idx < sizeof(RTCbuf)) {
      ECUbuf[((4 * idx))] = RTCbuf[idx];
      idx++;
    }
    doLog("RTC good");
    return (0);
  }
}

void IRAM_ATTR PSRbackup() {
  unsigned long chk32 = 0;
  int idx = 0;
  int loops = sizeof(PSRbuf) / 4;
  while (idx < loops) {
    PSRbuf[idx] = ECU32buf[idx];
    chk32 += ECU32buf[idx];
    idx++;
  }
  ECU32buf[loops - 1] -= chk32;
}

bool IRAM_ATTR PSRrestore() {
  unsigned long chk32 = 0;
  int idx = 0;
  int loops = sizeof(PSRbuf) / 4;
  while (idx < loops) {
    chk32 += PSRbuf[idx];
    idx++;
  }

  if (chk32) {
    doLog("bad PSR");
    return (1);
  } else {
    idx = 0;
    while (idx < loops) {
      ECU32buf[idx] = PSRbuf[idx];
      idx++;
    }
    doLog("PSR good");
    return (0);
  }
}


void setup() {
  setCpuFrequencyMhz(80);
  doLog("INIT");
#ifdef ESPLED
  FastLED.addLeds<WS2812, 21, GRB>(leds, NUM_LEDS);
  FastLED.clear();
#endif
  diagLED(1);

  Serial.setTimeout(0);
  Serial.begin(115200);

  VCU.setTimeout(0);
  VCU.begin(115200, SERIAL_8N1, RXD1, TXD1);

  BLE.setTimeout(0);
  BLE.begin(115200, SERIAL_8N1, RXD2, TXD2);

  while (!VCU || !BLE) { ; }
  doLog("SETUP");
  diagLED(7);

  if (!SPIFFS.begin(false)) {
    doLog("spiffs fail");
    formatting = 1;
  }

  if (formatting == 1) {
    doLog("format");
    if (SPIFFS.format()) {
      formatting = 0;
      diagLED[0];
    }
    ESP.restart();
  }
  diagLED(-1);

#ifdef wifiEnable
  wifiConnect();
  diagLED(-1);
  setupAsyncServer();
#endif
  diagLED(-1);

  initECU();
  powerState = ECUbuf[(ECUbank(ECU_BLE) * 0x200 * 4) + (BLE_Power * 2 * 4)] == 1;

  // initialize IFB buffers
  ifRX = (byte *)malloc(IFBlen);
  ifTX = (byte *)malloc(IFBlen);

  diagLED(0);
  protoInit();

  doLog("READY");
  delay(1000);
  timeOld = micros();
}

// process BLE>VCU (0xfd) subscription packet, build dictionary of u32 id & size
void subMap() {
  subCount = 0;
  int subMapAdr = ((0x200 * ECU_SUB) + 2);
  unsigned long subID = 1;

  while (subID) {
    subID = (ECU32buf[subMapAdr] & 0xff) + ((ECU32buf[subMapAdr + 1] & 0xff) << 8) + ((ECU32buf[subMapAdr + 2] & 0xff) << 16) + ((ECU32buf[subMapAdr + 3] & 0xff) << 24);
    subVar[subCount] = subID;
    subLen[subCount] = ECU32buf[subMapAdr + 4] & 0xff;
    //  doLog(("mapping " + String(subCount) + " " + String(subLen[subCount])).c_str());
    subCount++;
    subMapAdr += 5;
  }
}

// deconstruct VCU>BLE (0xfe) realtime report into individual u16s.
IRAM_ATTR void subSet() {
  int subSetAdr = ((0x200 * ECU_SUB) + (0x80 * 2) + 1);
  int subID = 0;
  int oByte = 0;
  int oBit = 7;
  //doLog(("subset my " + String(subCount) + " ").c_str());

  int sVal = 0;
  while (subID < subCount) {
    sVal = 0;
    for (int bits = 0; bits < subLen[subID]; bits++) {
      sVal *= 2;
      if (ECU32buf[subSetAdr + oByte] & (1 << oBit)) {
        sVal += 1;
      }
      if (oBit) {
        oBit--;
      } else {
        oBit = 7;
        oByte++;
      }
    }
    //doLog(("settt " + String(subID) + " " + String(sVal, HEX)).c_str());
    //subVal[subID] = sVal;  // BUG
    subVal[subID] = (sVal >> 8) + ((sVal & 0xff) << 8);
    if (subLen[subID] < 0x10) {
      ECU32buf[(0x200 * ECU_SUB) + (0xc0 * 2) + (subID * 2)] = sVal & 0xff;
      ECU32buf[(0x200 * ECU_SUB) + (0xc0 * 2) + (subID * 2) + 1] = 0x00;
    } else {
      ECU32buf[(0x200 * ECU_SUB) + (0xc0 * 2) + (subID * 2)] = (sVal >> 8) & 0xff;
      ECU32buf[(0x200 * ECU_SUB) + (0xc0 * 2) + (subID * 2) + 1] = sVal & 0xff;
    }
    subID++;
    ECU32buf[(0x200 * ECU_SUB) + (0xc0 * 2) + (subID * 2)] = 0xff;
    ECU32buf[(0x200 * ECU_SUB) + (0xc0 * 2) + (subID * 2) + 1] = 0xff;
  }
}

// return regID of u32 service
IRAM_ATTR byte subReg(const unsigned long reg) {
  for (int tmp = 0; tmp < subCount; tmp++) {
    if (subVar[tmp] == reg) {
      return (tmp);
    }
  }
  return (0);
}

// return last u16 received from u32 service
IRAM_ATTR int subGet(const unsigned long reg) {
  for (int tmp = 0; tmp < subCount; tmp++) {
    if (subVar[tmp] == reg) {
      return (subVal[tmp]);
    }
  }
  return (-1);
}

IRAM_ATTR byte ECUbank(const int ECUid) {
  switch (ECUid) {
    case ECU_MCU:
      return {
        0x06
      };
    case ECU_VCU:
      return {
        0x01
      };
    case ECU_BLE:
      return {
        0x03
      };
    case ECU_BMS:
      return {
        0x04
      };
    case ECU_TFT:
      return {
        0x05
      };
  }
}

IRAM_ATTR byte setECUptr(const byte ECU, const byte ADR) {
  ECUptr = &ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2)];
}

IRAM_ATTR byte read8(const byte ECU, const byte ADR) {
  return (ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2)] & 0xff);
}

IRAM_ATTR int read16(const byte ECU, const byte ADR) {
  return ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2)] & 0xff) + ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2) + 4] & 0xff) << 8));
}

IRAM_ATTR int read16sign(const byte ECU, const byte ADR) {
  int tmp = ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2)] & 0xff) + ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2) + 4] & 0xff) << 8));
  if (tmp > 0x7fff) { tmp -= 0x10000; }
  return (tmp);
}

IRAM_ATTR unsigned long read32(const byte ECU, const byte ADR) {
  return ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2)] & 0xff) + ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2) + 0x04] & 0xff) << 8) + ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2) + 0x08] & 0xff) << 16) + ((ECU32buf[(0x200 * ECUbank(ECU)) + (ADR * 2) + 0x0c] & 0xff) << 24));
}

void initECU() {
  // initialize ECU buffers
  ECUbuf = (byte *)malloc(ECUlen);
  ECU32buf = (unsigned long *)ECUbuf;

  for (int u = 0; u < ECU32len; u++) {
    ECU32buf[u] = 0x00;
  }

  if (PSRrestore()) {
    RTCrestore();
  };
}
/* 
  void doCmdPkt(const int ifb) {
  switch (*TXpktCmd) {
    case 0:         // debug
    case CMD_READ:  //
      ECUpos = ECU32buf[(ECUbank[dECU] * 0x200) + (dADR * 2)];
      int tmp = *TXpktLen;
      byte *d = (byte *)ECU;
      if ((*ECUpos & ECU_proxy) && (*ECUpos & (ECU_read | ECU_write))) {
        // answer from cache
      }
    //while (tmp--) {
    //char *d = (char *)dest;
    //const char *s = (const char *)src;
    // Did we get RX data?  buffer and process
    // *d++ = *s++;
    //}

    case CMD_WRITE:
      if ((*ECUpos & ECU_proxy) && (*ECUpos & ECU_write)) {
        // *ECUpos |= ECU_pending
      }
    case CMD_WRITE_NA:
      ECUpos = ECU32buf[(ECUbank[dECU] * 0x200) + dADR];
       ECUpos |= ECU_WRITE;

    case CMD_READ_RSP:
      ECUpos = ECU32buf[(ECUbank[sECU] * 0x200) + dADR];
       ECUpos |= ECU_READ;

    case CMD_WRITEACK:
      //*ECUpos |= ECU_READ;
      // clear 'ack requested'
  }
  }
*/

/*  routine imported from myBLEv2
   // update ECUmap based on variable access
                if (dECU) {
                  ECUptr = &ECU32buf[(0x200 * dECU) + (pADR * 2)];
                  byte grpFX = (*ECUptr >> 16);
                  byte grpMod = (*ECUptr >> 8);
                  grpFX = 0;  // functionality not ready

                  if ( (*ECUptr & ECU_noRemap)) {
  #ifdef DEBUG
                    printf("NOREMAP %08x %08x %08x\n", *ECUptr, ECU_noRemap, (*ECUptr & ECU_noRemap));
  #endif
                  } else {
                    for (int v = 0; v < pLEN; v++) {
                      byte newValue = TXpkt[v + 7];
                      byte txValue = newValue;
                      ECUptr = &ECU32buf[(0x200 * dECU) + (pADR * 2) + v];
                      byte oldValue = *ECUptr;
                      long oldFlags = *ECUptr & 0xffff0000;
                      long newFlags = oldFlags;
                      byte mapFX = (*ECUptr >> 16);
                      byte modifier = (*ECUptr >> 8);

                      if (pCMD == 0x04) {
                        newFlags |= ECU_read;
                      }
                      if ((pCMD == 0x02) || (pCMD == 0x03)) {
                        newFlags |= ECU_write;
                        pkt_writing = 1;
                      }
                      if (pCMD == 0x02) {
                        wr_ack = 1;  // schedule write ack
                      }

                      if (oldValue != txValue) {
                        newFlags |= ECU_valChange;
                        grpChange = 1;
                      }
*/

// receive rxBS bytes from interface ifb
int IRAM_ATTR doRX(const int ifb) {
  // Receive <= BS bytes from different interfaces
  int rxBytes = 0;
  switch (ifb) {
    case 0:  // The ESP32 USB serial
      rxBytes = Serial.readBytes(RXbuf, rxBS);
      break;
    case 1:
      rxBytes = VCU.readBytes(RXbuf, rxBS);
      break;
    case 2:
      rxBytes = BLE.readBytes(RXbuf, rxBS);
      break;
  }
  if (rxBytes) {
    RXptr = &ifRX[(0x200 * ifb) + ifRXlen[ifb]];
    ifRXlen[ifb] += rxBytes;
    const char *s = (const char *)RXbuf;
    while (rxBytes--) {
      *RXptr++ = *s++;
    }
    return (1);
  }
  return (0);
  //rxBytes;
}

// decode a packet from ifb queue
int doRXpkt(const int ifb) {
  RXptr = &ifRX[(0x200 * ifb)];
  RXlen = &ifRXlen[ifb];
  //doLog("");
  //doLog(String(*RXlen).c_str());

  int skipBad = 0;
  int doMore = 1;
  int rs = (*RXlen - 8);  // enough bytes for a packet?
  int idx = 0;

  for (; (idx < rs) && doMore; idx++) {
    if ((RXptr[idx] == 0x5a) && (RXptr[idx + 1] == 0xa5) && doMore) {
      int len = RXptr[idx + 2];
      if (*RXlen < (idx + len + 9)) {
        //doLog("partial RX");
        RXinc++;
        ECUbuf[(0x200 * ECU_MOD * 4) + (0x0f * 4 * 2)] = (RXinc & 0xff);
        ECUbuf[(0x200 * ECU_MOD * 4) + (0x0f * 4 * 2) + 0x04] = (RXinc >> 8) & 0xff;
        doMore = 0;  // packet has not been completely received; come back later
        break;
      }

      // verify packet checksum
      int cksum = 0;
      int j = 2;
      for (; j < (len + 7); j++) {
        cksum += RXptr[j + idx];
        TXpkt[j] = RXptr[j + idx];
      }
      cksum &= 0xffff;
      cksum ^= 0xffff;

      if (((cksum & 0xff) != RXptr[j + idx]) || ((cksum >> 8) != RXptr[j + idx + 1])) {
        RXbad++;
        ECUbuf[(0x200 * ECU_MOD * 4) + (0x0e * 4 * 2)] = (RXbad & 0xff);
        ECUbuf[(0x200 * ECU_MOD * 4) + (0x0e * 4 * 2) + 0x04] = (RXbad >> 8) & 0xff;
      } else {
        int offset = (idx + j + 2);
        //doLog(("offset " + String(offset)).c_str());

        int k = offset;
        for (; (k < *RXlen); k++) {
          RXptr[k - offset] = RXptr[k];
        }
        *RXlen -= offset;  // shift buffer down
        doMore = 0;        // only return 1 packet
        return (1);
        break;
      }
    } else {
      skipBad = 1;
      RXbad++;
    }
  }
  return (0);
}

/*
211 5a a5 02 16 23 03 60 00 00 61 ff
    127 5a a5 02 16 23 03 60 01 00 60 ff
     85 5a a5 02 16 23 03 60 02 00 5f ff
      3 5a a5 06 16 23 03 3d 00 00 14 0c 2f 17 1a ff clock
*/

void doRXdecode() {
  byte *tmp = (byte *)TXpkt;

  int addr = 0;
  unsigned long tmpVar = 0;
  unsigned long newFlags = 0;

  switch (tmp[5]) {
    case CMD_READ:
      {
        // an opportunity to return false data
        break;
      }
    case CMD_WRITE:
    case CMD_WRITE_NR:
      {
        // maybe not the best... time calibration
        if ((tmp[4] == 0x23) & (tmp[6] == 0x3d)) {
          doLog(("calTime " + String(timeNow - calLast) + " " + String(timeNow) + " " + String(tmp[7], HEX) + " " + String(tmp[8], HEX) + " " + String(tmp[9], HEX) + " " + String(tmp[0x0a], HEX) + " " + String(tmp[0x0b], HEX) + " " + String(tmp[0x0c], HEX)).c_str());
          calLast = timeNow;
        }

        addr = ECUbank(tmp[4]) * 0x200;
        if ((tmp[4] == 0x23) & (tmp[6] == 0xfe)) {
          addr = 0x304;  // kludge, receive subscription parameters at ecu bank 2
          break;
        }
        if ((tmp[4] == ECU_VCU) & (tmp[6] == 0xfd)) {
          addr += 6;  // kludge, receive at ecu bank 2, 0x00
          break;
        }

        newFlags |= ECU_write;
        break;
      }
    case CMD_READ_RESP:
      {
        //doLog(("readR " + String(ECUbank(tmp[3]), HEX) + ":" + String(tmp[6], HEX)).c_str());
        if ((tmp[6] == 0xfc)) { break; }
        //if ((tmp[4] == 0x23) & (tmp[6] == 0xfc)) { break; }

        addr = ECUbank(tmp[3]) * 0x200;
        if (tmp[4] != MY_ECU) {
          newFlags |= ECU_read;
        }
        break;
      }
  }

  if (addr) {
    addr += (tmp[6] * 2);
    int tmpFlags = newFlags;

    for (int i = 0; i < tmp[2]; i++) {
      newFlags = tmpFlags;
      if (tmp[7 + i] != (ECU32buf[addr + i] & 0xff)) {
        if (ECU32buf[addr + i] & 0xff) {
          newFlags = (tmpFlags | ECU_changed);
        }
      }
      tmpVar = ((ECU32buf[addr + i] & 0xffffff00) | newFlags) + tmp[7 + i];
      ECU32buf[addr + i] = tmpVar;
    }
  }
}

void IRAM_ATTR pktXmitBuf(const int mask) {
  //timeTXecu[0]=timeNow;

  //timeTXifb[0]=timeNow;
  //if timeTXdelay[(*TXpktDst)]

  if (mask & 0x01) {
    Serial.write(TXpkt, (*TXpktLen + 9));
    //timeTX(0)=timeNow;
    //timeTX(1)=timeNow;
  }
  if (mask & 0x02) {
    //timeTX(0)=timeNow;
    //timeTX(2)=timeNow;
    VCU.write(TXpkt, (*TXpktLen + 9));
  }
  if (mask & 0x04) {
    //timeTX(0)=timeNow;
    //timeTX(3)=timeNow;
    BLE.write(TXpkt, (*TXpktLen + 9));
  }
  if (mask & 0x08) {
    String myStat = "";
    for (byte i = 0; i < (*TXpktLen + 9); i++) {
      int val = (TXpkt[i]);
      myStat += " ";
      myStat += (val < 0x10 ? "0" : "");
      myStat += String(val, HEX);
    }
    if (sniffDump) {
      doLog(myStat.c_str());
    }
  }
}

void IRAM_ATTR pktXmitCmd(const int mask, const byte pktDat[]) {
  int TXparity = 0;
  int tmp = 0;
  for (; tmp < (pktDat[0] + 5); tmp++) {
    TXpktBuf[tmp] = pktDat[tmp];
    TXparity += pktDat[tmp];
  }
  TXparity &= 0xffff;
  TXparity ^= 0xffff;

  TXpktBuf[tmp] = (TXparity & 0xff);
  TXpktBuf[tmp + 1] = (TXparity >> 8);

  // if (TXparity != LastTXparity) {  // skip duplicate
  if (true) {
    pktXmitBuf(mask);
    LastTXparity = TXparity;
    TXyes++;
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0c * 4 * 2)] = (TXyes & 0xff);
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0c * 4 * 2) + 0x04] = (TXyes >> 8) & 0xff;
  } else {
    TXno++;
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0d * 4 * 2)] = (TXno & 0xff);
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0d * 4 * 2) + 0x04] = (TXno >> 8) & 0xff;
  }
}

// fix CRC and send packet
void IRAM_ATTR pktSend(const int mask) {
  int TXparity = 0;
  int tmp = 0;
  for (; tmp < (*TXpktLen + 5); tmp++) {
    TXparity += TXpktBuf[tmp];
  }
  TXparity &= 0xffff;
  TXparity ^= 0xffff;

  TXpktBuf[tmp] = (TXparity & 0xff);
  TXpktBuf[tmp + 1] = (TXparity >> 8);

  //  if (TXparity != LastTXparity) {   // don't send duplicates
  if (true) {
    pktXmitBuf(mask);
    LastTXparity = TXparity;
    TXyes++;
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0c * 4 * 2)] = (TXyes & 0xff);
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0c * 4 * 2) + 0x04] = (TXyes >> 8) & 0xff;
  } else {
    TXno++;
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0d * 4 * 2)] = (TXno & 0xff);
    ECUbuf[(0x200 * ECU_MOD * 4) + (0x0d * 4 * 2) + 0x04] = (TXno >> 8) & 0xff;
  }
}

void protoInit() {
  int mask = 0x0e;
  pktXmitCmd(mask, xMOD);
  pktXmitCmd(mask, xALS0);
  subMap();
}

unsigned long getStatVars() {
  //const int battCells = 0x14;  // BUG: read from BMS instead
  int battCells = ECUbuf[(ECUbank(ECU_BMS) * 0x200 * 4) + (BMS_SERIES_CELLS * 2 * 4)];  // BUG: read from BMS instead
  const int battTemps = 0x08;                                                           // BUG: read from BMS

  static int battC = 0;
  static int battT = 0;

  volts = 0;
  int addr = (ECUbank(ECU_BMS) * 0x200) + (BMS_CellVolts * 2);
  for (int i = 0; i < battCells; i++) {
    volts += ECU32buf[addr + (i * 2)] & 0xff;
    volts += (ECU32buf[addr + (i * 2) + 1] & 0xff) << 8;
  }

  TXpktBuf[0] = 0x02;
  TXpktBuf[1] = MY_ECU;
  TXpktBuf[2] = ECU_BMS;
  TXpktBuf[3] = CMD_READ;
  TXpktBuf[4] = (BMS_CellVolts + battC);
  TXpktBuf[5] = 0x02;
  TXpktBuf[6] = 0x00;
  pktSend(0x0e);

  battC += 1;
  if (battC > battCells) {
    battC -= battCells;
  }

  // get the temps
  TXpktBuf[2] = ECU_MCU;
  TXpktBuf[4] = (BMS_CellVolts + battC);
  TXpktBuf[5] = 0x04;


  temps = 0;
  int count = 0;

  addr = (ECUbank(ECU_BMS) * 0x200) + (BMS_CellTemps * 2);
  for (int i = 0; i < battTemps; i++) {
    if ((ECU32buf[addr + (i * 2)] & 0xff) != 0xff) {
      temps += ECU32buf[addr + (i * 2)] & 0xff;
      temps += (ECU32buf[addr + (i * 2) + 1] & 0xff) << 8;
      count++;
    }
    //   temps *= 100;
    //   temps /= count;
  }

  return (volts);
}

void poop() {
  static int rxBytes = 0;
  rxBytes = BLE.readBytes(RXbuf, rxBS);
  if (rxBytes) { VCU.write(RXbuf, rxBytes); }

  rxBytes = VCU.readBytes(RXbuf, rxBS);
  if (rxBytes) { BLE.write(RXbuf, rxBytes); }
}
/*
lastRX(maxIFB);
lastTX(maxIFB);
*/

void loop() {
  static int ecuPhase = 0;
  static int ifbPhase = 0;

  static unsigned long timeCycle = 0;
  static unsigned long timeLoop = 0;
  static unsigned long timeOff = 0;

  static int scanAddr = 0x00;
  static int scanECU = 0;
  static int doRun = 1;


  while (doRun) {
    // timekeeping and scheduling
    timeNow = micros();
    if (timeNow > timeOld) {
      timeCycle = timeNow - timeOld;
      timeLoop = timeNow - timeOut;
      timeOff = timeNow - timeOut;
    }

    if ((timeNow - timeLastFast) > timeTickFast) {
      timeLastFast += timeTickFast;
      tickFast = 1;
    }

    if ((timeNow - timeLastMid) > timeTickMid) {
      timeLastMid += timeTickMid;
      tickMid = 1;
    }

    if ((timeNow - timeLastSlow) > timeTickSlow) {
      timeLastSlow += timeTickSlow;
      tickSlow = 1;
    }

    int count = 0;
    if (doRX(ifbPhase)) {
      while (doRXpkt(ifbPhase)) {
        doRXdecode();

        // not a good implementation
        if (TXpktBuf[4] == 0xfe) {

          temper++;
          if (temper > 0xff) { temper -= 0x100; }
          //TXpktBuf[0x0a]=ECUbuf[(0x200 * ECU_BMS * 4)+(0x8d * 4 * 2)];
          //TXpktBuf[0x0b]=ECUbuf[(0x200 * ECU_BMS * 4)+(0x8d * 4 * 2)+4];
          static int lastVolts = 0;
          lastVolts *= 3;
          lastVolts += volts;
          if (lastVolts) {
            lastVolts -= 2;
            lastVolts /= 4;
          }
          int oVolts = (lastVolts - 5) / 10;
          //TXpktBuf[0x08] = oVolts&0xff;  // debug volts in ODO
          //TXpktBuf[0x09] = (oVolts>>8)&0xff;
          TXpktBuf[0x08] = oVolts & 0xff;  // debug volts in ODO
          TXpktBuf[0x09] = (oVolts >> 8) & 0xff;
          subSet();
        }
        //TXpktBuf[10]=
        //ECUbuf[(0x200 * 6 * 4) + (2*4*2)]=(temper&0xff);
        if (ecu_cmd_acl[*TXpktCmd] & (1 << ECUbank(*TXpktDst))) {
          ECUbuf[(0x200 * 7 * 4) + (0x10 * 4 * 2)]++;  // packet metrics: acl yes
          if (ifbPhase == 1) {
            pktSend(0x0c);
          }
          if (ifbPhase == 2) {
            pktSend(0x0a);
          }
        } else {
          ECUbuf[(0x200 * 7 * 4) + (0x11 * 4 * 2)]++;  // packet metrics: acl no

          // store copy of denied packet
          for (int i = 0; i < 0x0c; i++) {
            if (i > *TXpktLen) {
              ECUbuf[(0x200 * 7 * 4) + (0x12 * 4 * 2) + (i * 4)] = 0;
            } else {
              ECUbuf[(0x200 * 7 * 4) + (0x12 * 4 * 2) + (i * 4)] = TXpktBuf[i];
            }
          }
        }
        count++;
      }
    }

    powerState = ECUbuf[(ECUbank(ECU_BLE) * 0x200 * 4) + (BLE_Power * 2 * 4)] == 1;

    if (tickMid) {
      tickMid = 0;

      live.send(String(subGet(SUB_Speed)).c_str(), "speed", timeNow);
      live.send(String(volts).c_str(), "volts", timeNow);
      live.send(String(temps).c_str(), "temp", timeNow);

      if (powerState) {
        unsigned long getBatt = getStatVars();

        if (fullRead) {
          //unsigned long getBatt=battStat();
          ECUbuf[(0x200 * 7 * 4) + (0x08 * 4)] = getBatt & 0xff;
          ECUbuf[(0x200 * 7 * 4) + (0x09 * 4)] = (getBatt >> 8) & 0xff;
          ECUbuf[(0x200 * 7 * 4) + (0x0a * 4)] = (getBatt >> 16) & 0xff;
          ECUbuf[(0x200 * 7 * 4) + (0x0b * 4)] = (getBatt >> 24) & 0xff;


          static byte xread[] = { 0x02, MY_ECU, 0x16, 0x01, 0x00, 0x10, 0x00 };
          *TXpktLen = 0x02;
          *TXpktSrc = MY_ECU;
          if (scanECU == 0) { *TXpktDst = ECU_VCU; }
          if (scanECU == 1) { *TXpktDst = ECU_BMS; }
          if (scanECU == 2) { *TXpktDst = ECU_MCU; }
          if (scanECU == 3) { *TXpktDst = ECU_BLE; }
          //if (scanECU == 4) { TXpktBuf[2] = 0x23; } // ECU not respond
          *TXpktCmd = 0x01;
          *TXpktArg = scanAddr;
          TXpktBuf[5] = 0x08;  // was 0x10
          TXpktBuf[6] = 0x00;
          pktSend(0x0e);

          scanECU += 1;
          if (scanECU > 3) {
            scanECU = 0;
            scanAddr += 0x04;  // was 0x08
            scanAddr &= 0xff;
            if (!scanAddr) {
              fullRead--;
              doLog("ECU scan complete");
            }
          }
        }
        ECUbuf[(0x200 * ECU_MOD * 4)]++;  // metrics: mid ticks
      }
      PSRbackup();
    }

    if (tickSlow) {
      tickSlow = 0;
      ECUbuf[(0x200 * ECU_MOD * 4) + 4]++;
      RTCbackup();
    }

    if (diagLEDtcount) {
      diagLEDtcount--;
    } else {
      diagLEDtcount = diagLEDtiming;  // how many cycles per change
      flicker = (1 - flicker);
      if (flicker) {
        diagLED(diagLEDp);
        if (diagLEDs) {
          diagLEDs--;
        } else {
          diagLEDs = 16;
          diagLEDp--;
          if (diagLEDp < 1) {
            diagLEDp = 5;
          }
          if (diagLEDp > 5) {
            diagLEDp = 1;
          }
        }
      } else {
        diagLED(0);
      }
    }

    // firmware update reboot
    if (rebooting) {
      diagLED(7);
      PSRbackup();
      RTCbackup();
      delay(500);
      ESP.restart();
    }

    ifbPhase++;
    if (ifbPhase > maxIFB) {
      ifbPhase = 0;
    }

    loopTime += micros();
    loopTime -= timeNow;
    loopTime /= 2;

    timeOld = timeNow;
    timeOut = micros();
    yield();
  }
}


#ifdef wifiEnable
void wifiConnect() {
  WiFi.setHostname(hostname);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  doLog("WLAN connecting");
  int i = 0;
  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      delay(500);
      //Serial.print(".");
      i++;
      if (i > 30) {
        doLog("FAIL");
        break;
      }
    } else {
      doLog(WiFi.localIP().toString().c_str());
      break;
    }
  }
}

void updateFlash(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
  }
  if (!Update.hasError()) {
    Update.write(data, len);
  }
  if (final) {
    Update.end(true);
  }
}

void uploadFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    request->_tempFile = SPIFFS.open("/" + filename, "w");
  }
  if (len) {
    request->_tempFile.write(data, len);
  }
  if (final) {
    request->_tempFile.close();
    request->redirect("/admin");
  }
}

String varProc(const String &var) {
  if (var == "SPIFFS_USED_BYTES") {
    return String(SPIFFS.usedBytes());
  }
  if (var == "SPIFFS_TOTAL_BYTES") {
    return String(SPIFFS.totalBytes());
  }
  return String();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404 badURL\n");
}

void setupAsyncServer() {
  server.on(
    "/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!request->authenticate(http_username, http_password)) {
        return request->requestAuthentication();
      }
      request->send_P(200, "text/html", admin_html, varProc);
    });

  server.on(
    "/update", HTTP_POST, [](AsyncWebServerRequest *request) {
      rebooting = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", rebooting ? ok_html : failed_html);
      response->addHeader("Connection", "close");
      request->send(response);
    },
    updateFlash);

  server.on(
    "/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->redirect("/admin");
    },
    uploadFile);

  server.on(
    "/format", HTTP_POST, [](AsyncWebServerRequest *request) {
      doLog("user set format");
      formatting = 1;
      request->send(200);
      rebooting = 1;
    });

  server.on(
    "/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      doLog("user reboot");
      request->send(200);
      rebooting = 1;
    });

  server.on(
    "/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      int mask = 0x0e;
      pktXmitCmd(mask, xTurnOn);
      delay(500);
      pktXmitCmd(mask, xTurnOn);
      request->redirect("/");
    });

  server.on(
    "/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      int mask = 0x0e;
      pktXmitCmd(mask, xTurnOff);
      request->redirect("/");
    });

  server.on(
    "/ecu/vcu", HTTP_GET, [](AsyncWebServerRequest *request) {
      myECU = 1;
      request->redirect("/");
    });

  server.on(
    "/ecu/mcu", HTTP_GET, [](AsyncWebServerRequest *request) {
      myECU = 6;
      request->redirect("/");
    });

  server.on(
    "/ecu/bms", HTTP_GET, [](AsyncWebServerRequest *request) {
      myECU = 4;
      request->redirect("/");
    });

  server.on(
    "/ecu/sub", HTTP_GET, [](AsyncWebServerRequest *request) {
      myECU--;
      //tempad--;
      doLog(("select " + String(myECU, HEX)).c_str());
      request->redirect("/");
    });

  server.on(
    "/ecu/add", HTTP_GET, [](AsyncWebServerRequest *request) {
      myECU++;
      //tempad++;
      doLog(("select " + String(myECU, HEX)).c_str());
      request->redirect("/");
    });

  server.on(
    "/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
      fullRead++;
      doLog("ECU scan start");
      request->redirect("/");
    });

  server.on(
    "/refresh", HTTP_GET, [](AsyncWebServerRequest *request) {
      for (int u = 0; u < ECU32len; u++) {
        ECU32buf[u] &= 0xff;
      }
      request->redirect("/");
    });

  server.on(
    "/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
      for (int u = 0; u < ECU32len; u++) {
        ECU32buf[u] = 0;
      }
      request->redirect("/");
    });

  server.on("/bms/volt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(getStatVars()).c_str());
  });

  server.on(
    "/", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncResponseStream *response = request->beginResponseStream("text/html");

      File headers = SPIFFS.open("/header.html", "r");
      while (headers.available()) {
        String headerTmp = headers.readStringUntil('\n');
        response->print(headerTmp);
      }
      headers.close();

      response->print("<br><p>");
      if (myECU == 4) {
        response->print("<table class=greenTable><thead>");
      } else if (myECU == 6) {
        response->print("<table class=redTable><thead>");
      } else {
        response->print("<table class=blueTable><thead>");
      }
      response->print("<tr><th>" + String(myECU, HEX) + "</th>");
      response->print("<th colspan=2>&lt;0&gt;</th><th colspan=2>&lt;1&gt;</th><th colspan=2>&lt;2&gt;</th><th colspan=2>&lt;3&gt;</th><th colspan=2>&lt;4&gt;</th><th colspan=2>&lt;5&gt;</th><th colspan=2>&lt;6&gt;</th><th colspan=2>&lt;7&gt;</th><th colspan=2>&lt;8&gt;</th><th colspan=2>&lt;9&gt;</th><th colspan=2>&lt;a&gt;</th><th colspan=2>&lt;b&gt;</th><th colspan=2>&lt;c&gt;</th><th colspan=2>&lt;d&gt;</th><th colspan=2>&lt;e&gt;</th><th colspan=2>&lt;f&gt;</th></tr>");
      response->print("</thead><tbody>");

      // print a table of 16bit hexes
      for (byte j = 0; j < 0x10; j++) {
        String myStat = "";

        //        myStat += "<span id=\"0x" + String(j, HEX) + "\"><tr><th>0x" + String(j, HEX) + "_</th>";
        myStat += "<tr id=\"0x" + String(j, HEX) + "\"><th>0x" + String(j, HEX) + "_</th>";

        for (byte i = 0; i < 0x20; i++) {
          myStat += "<td>";

          int tmpval = (ECU32buf[(0x200 * (myECU)) + (j * 0x20) + i]);
          int val = (tmpval & 0xff);
          if (tmpval & ECU_changed) {
            myStat += "<b>";
          }
          if (!((tmpval & ECU_read) | (tmpval & ECU_write))) {
            myStat += "<small>";
          }
          myStat += (val < 0x10 ? "0" : "");
          myStat += String(val, HEX) + "</td>";
          if (!((tmpval & ECU_read) | (tmpval & ECU_write))) {
            myStat += "</small>";
          }
          if (tmpval & ECU_changed) {
            myStat += "</b>";
          }
        }
        myStat += "</tr>";
        response->print(myStat);
      }

      response->print("</tbody></table><br><p>");
      response->print("</body></html>");
      request->send(response);
    });

  server.on(
    "/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/favicon.ico", "image/vnd.microsoft.icon");
    });

  server.onNotFound(notFound);  // 404
  server.serveStatic("/fs", SPIFFS, "/");

  // web events
  live.onConnect([](AsyncEventSourceClient *client) {
    client->send(hostname, NULL, timeNow, 10000);  // send connect event, id=timestamp, reconnect: 10000=1 second
  });

  server.addHandler(&live);
  webSerial.begin(&server, "/log");
  server.begin();
}
#endif
