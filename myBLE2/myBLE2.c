// myBLEv2, MIT license
// a Segway "3 series" BLE display replacement on Arduino ESP32
//
// this is beta two.  fixed some power bugs, blocked CMD 20 BLE debugs
// added auto-headlight, speed mode, boost and brake indicators
// added speedometer and battery gauge on 2x8 WS2812s
// bluetooth is not currently supported, but I've had it working in the past (myBLEv1
// project) so it shouldn't be too bad... this release spent a lot of time laying the
// groundwork with message fragmentation and packet validation
//
// for demo and discussion see my livestreams, https://www.youtube.com/@MacintoshKeyboardHacking/streams

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef ARDUINO
#define ESPLED sure

#include <HardwareSerial.h>
// GPIO pins to VCU UART
#define RXD1 16
#define TXD1 17
HardwareSerial VCU (1);

// GPIO pins to BLE UART
#define RXD2 26
#define TXD2 25
HardwareSerial BLE (2);

#include <FastLED.h>
#define NUM_LEDS 16
static CRGB leds[NUM_LEDS];
#define FASTLED_LED_OVERCLOCK 1.4

#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

// arduino compat
unsigned long millis(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts );
  return ( ts.tv_sec * 1000 + ts.tv_nsec / 1000000L );
}

typedef int16_t word;
#define DEBUG output
#endif

typedef unsigned char byte;

static const int maxIFB = 5;  // number of physical interfaces to route
static const int rxBS = 256;  // serial interface block transfer size

static byte ifRX[5][0x200] = { 0 }; // set to match maxIFB, per-interface RX queue
static byte ifTX[5][0x200] = { 0 }; // a valid packet could be over 260 bytes...
static int ifRXlen[5] = { 0 };
static int ifTXlen[5] = { 0 };  // per-interface buffer position
static byte *RXptr;    // convenience pointer

static const int maxECU = 16; // number of ECUs to virtualize
static int ECUlen = (0x200 * maxECU * 4); // 32 bits storage for each of 256, 16bit integers, per ECU
static byte *ECUbuf;          // ECU malloc() as u8
static unsigned long *ECU32;  // ECU as u32
static unsigned long *ECUptr; // convenience pointer

FILE *datIn;

static int flicker = 0;
static int flash = 0;

static int t, u;
static int i, j, k, l;

static int running = 1;
int comPort;

static unsigned long timeNow;

// myBLE (proprietary) service announce
static byte initStr[] = {0x5a, 0xa5, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff};

// heartbeat PING 04>16 7a:
static byte hb0[] = {0x5a, 0xa5, 0x00, 0x04, 0x16, 0x7a, 0x00, 0x6b, 0xff};

// heartbeat PONG 04>16 7b:
static byte hb1[] = {0x5a, 0xa5, 0x00, 0x04, 0x16, 0x7b, 0x02, 0x68, 0xff};

// heartbeat stop the error beep, any read from 0x23 to 0x16 works
static byte hb2[] = {0x5a, 0xa5, 0x00, 0x23, 0x16, 0x01, 0xfc, 0xc9, 0xfe};

// auto-headlight on (need test)
//static byte light[] = {0x5a, 0xa5, 0x00, 0x23, 0x16, 0x80, 0x00, 0x46, 0xff, 0x5a, 0xa5, 0x02, 0x23, 0x16, 0x03, 0xd2, 0x00, 0x00, 0xef, 0xfe};
static byte light[] = {0x5a, 0xa5, 0x02, 0x23, 0x16, 0x03, 0xd2, 0x00, 0x00, 0xef, 0xfe};

// VCU subscription like stock BLE
static byte bSub[] = {0x5a, 0xa5, 0xbb, 0x23, 0x16, 0x02, 0xfd, 0x01, 0x0a, 0x89, 0x52, 0x76, 0x7e, 0x10, 0x89, 0x7a, 0xaa, 0xdd, 0x10, 0xc5, 0x0a, 0x44, 0xac, 0x10, 0xb3, 0xd1, 0xdd, 0xa3, 0x10, 0xb2, 0xd1, 0xdd, 0xa3, 0x10, 0xad, 0xd1, 0xdd, 0xa3, 0x10, 0xac, 0xd1, 0xdd, 0xa3, 0x10, 0xaf, 0xd1, 0xdd, 0xa3, 0x10, 0x44, 0x6c, 0xe5, 0x6f, 0x10, 0xae, 0x48, 0x4a, 0xad, 0x10, 0x9e, 0xf9, 0x96, 0x90, 0x10, 0xe4, 0x65, 0x9b, 0x9c, 0x10, 0x92, 0x00, 0x9a, 0x22, 0x10, 0x64, 0xd0, 0xf6, 0xa4, 0x08, 0x12, 0x45, 0x6a, 0x61, 0x08, 0x70, 0xe0, 0xe2, 0xb3, 0x08, 0x8a, 0x38, 0x99, 0xe7, 0x08, 0xda, 0xb2, 0x89, 0x86, 0x04, 0xa1, 0x24, 0x01, 0x7e, 0x04, 0x4b, 0xcb, 0xf9, 0xf8, 0x04, 0x75, 0x1d, 0xbc, 0xa2, 0x04, 0x9f, 0x6f, 0x41, 0x42, 0x01, 0x98, 0x8d, 0xb2, 0x17, 0x01, 0x29, 0x0a, 0xbc, 0xcf, 0x01, 0xcc, 0xba, 0xe7, 0x88, 0x01, 0xe5, 0xe9, 0x99, 0x0c, 0x01, 0x61, 0xe9, 0x41, 0x18, 0x01, 0x8a, 0xa8, 0xb5, 0x16, 0x01, 0xb3, 0xd7, 0xef, 0x70, 0x01, 0x03, 0x26, 0x1a, 0x75, 0x01, 0x39, 0xb0, 0xbf, 0x06, 0x01, 0x3b, 0x26, 0xe8, 0x20, 0x01, 0x55, 0x25, 0x1a, 0x7f, 0x01, 0x2b, 0x71, 0x39, 0xf7, 0x01, 0x6b, 0x64, 0x4e, 0xac, 0x01, 0x19, 0xbd, 0x86, 0x00, 0x01, 0x1a, 0x93, 0x7d, 0x0c, 0x01, 0x37, 0xaf};

// delay between heartbeat packets
static int hbDelay = 2400;
static int hbPhase = 0;

static int myPower = 1;
static int myDrive = 0;

#ifdef ESPLED
#include "hal/gpio_ll.h"  // GPIO register functions

// gamma correct linear light
static word table[] = {0x0000, 0x0003, 0x0008, 0x000f, 0x0018, 0x0023, 0x0030, 0x003f, 0x0050, 0x0063, 0x0078, 0x008f, 0x00a8, 0x00c3, 0x00e0, 0x00ff, 0x0120, 0x0143, 0x0168, 0x018f, 0x01b8, 0x01e3, 0x0210, 0x023f, 0x0270, 0x02a3, 0x02d8, 0x030f, 0x0348, 0x0383, 0x03c0, 0x03ff, 0x0440, 0x0483, 0x04c8, 0x050f, 0x0558, 0x05a3, 0x05f0, 0x063f, 0x0690, 0x06e3, 0x0738, 0x078f, 0x07e8, 0x0843, 0x08a0, 0x08ff, 0x0960, 0x09c3, 0x0a28, 0x0a8f, 0x0af8, 0x0b63, 0x0bd0, 0x0c3f, 0x0cb0, 0x0d23, 0x0d98, 0x0e0f, 0x0e88, 0x0f03, 0x0f80, 0x0fff, 0x1080, 0x1103, 0x1188, 0x120f, 0x1298, 0x1323, 0x13b0, 0x143f, 0x14d0, 0x1563, 0x15f8, 0x168f, 0x1728, 0x17c3, 0x1860, 0x18ff, 0x19a0, 0x1a43, 0x1ae8, 0x1b8f, 0x1c38, 0x1ce3, 0x1d90, 0x1e3f, 0x1ef0, 0x1fa3, 0x2058, 0x210f, 0x21c8, 0x2283, 0x2340, 0x23ff, 0x24c0, 0x2583, 0x2648, 0x270f, 0x27d8, 0x28a3, 0x2970, 0x2a3f, 0x2b10, 0x2be3, 0x2cb8, 0x2d8f, 0x2e68, 0x2f43, 0x3020, 0x30ff, 0x31e0, 0x32c3, 0x33a8, 0x348f, 0x3578, 0x3663, 0x3750, 0x383f, 0x3930, 0x3a23, 0x3b18, 0x3c0f, 0x3d08, 0x3e03, 0x3f00, 0x3fff, 0x4100, 0x4203, 0x4308, 0x440f, 0x4518, 0x4623, 0x4730, 0x483f, 0x4950, 0x4a63, 0x4b78, 0x4c8f, 0x4da8, 0x4ec3, 0x4fe0, 0x50ff, 0x5220, 0x5343, 0x5468, 0x558f, 0x56b8, 0x57e3, 0x5910, 0x5a3f, 0x5b70, 0x5ca3, 0x5dd8, 0x5f0f, 0x6048, 0x6183, 0x62c0, 0x63ff, 0x6540, 0x6683, 0x67c8, 0x690f, 0x6a58, 0x6ba3, 0x6cf0, 0x6e3f, 0x6f90, 0x70e3, 0x7238, 0x738f, 0x74e8, 0x7643, 0x77a0, 0x78ff, 0x7a60, 0x7bc3, 0x7d28, 0x7e8f, 0x7ff8, 0x8163, 0x82d0, 0x843f, 0x85b0, 0x8723, 0x8898, 0x8a0f, 0x8b88, 0x8d03, 0x8e80, 0x8fff, 0x9180, 0x9303, 0x9488, 0x960f, 0x9798, 0x9923, 0x9ab0, 0x9c3f, 0x9dd0, 0x9f63, 0xa0f8, 0xa28f, 0xa428, 0xa5c3, 0xa760, 0xa8ff, 0xaaa0, 0xac43, 0xade8, 0xaf8f, 0xb138, 0xb2e3, 0xb490, 0xb63f, 0xb7f0, 0xb9a3, 0xbb58, 0xbd0f, 0xbec8, 0xc083, 0xc240, 0xc3ff, 0xc5c0, 0xc783, 0xc948, 0xcb0f, 0xccd8, 0xcea3, 0xd070, 0xd23f, 0xd410, 0xd5e3, 0xd7b8, 0xd98f, 0xdb68, 0xdd43, 0xdf20, 0xe0ff, 0xe2e0, 0xe4c3, 0xe6a8, 0xe88f, 0xea78, 0xec63, 0xee50, 0xf03f, 0xf230, 0xf423, 0xf618, 0xf80f, 0xfa08, 0xfc03, 0xfe00, 0xffff};
//  for (int a=1; a<257; a++) {int b=(a*a)-1; printf("0x%04x, ",b);}

#define LRpin 32
#define LGpin 5
#define LBpin 33

#define P1freq 100
#define P1chan 2
#endif

static int ledDly = 0;
static int ledBri = 1;
static unsigned long timeGo = 0;
static unsigned long timeLast = 0;
static int goPhase = 0;

void setup () {
#ifdef DEBUG
  printf ("setup\n");
#endif

  // initialize serial ports
#ifdef ARDUINO
  Serial.setTimeout (0);
  Serial.begin (115200);

  VCU.setTimeout (0);
  VCU.begin (115200, SERIAL_8N1, RXD1, TXD1);

  BLE.setTimeout (0);
  BLE.begin (115200, SERIAL_8N1, RXD2, TXD2);

  while (!Serial || !VCU || !BLE);
  Serial.write (initStr, sizeof (initStr));
  VCU.write (light, sizeof (light));    // because we don't have a light sensor

  FastLED.addLeds<WS2812, 23, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
#else
  comPort = open ("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NONBLOCK);
  datIn = fopen ("test.bin", "rb");  // read binary
  write(comPort, initStr, sizeof (initStr));
#endif

  // initialize ECU buffer
  ECUbuf = (byte *) malloc(ECUlen);
  ECU32 = (unsigned long *) ECUbuf;

  for (int u = 0; u < ECUlen; u++) {
    ECUbuf[u] = 0x0;
  }

#ifdef ESPLED
  ledcAttachChannel(LRpin, P1freq, 16, P1chan);
  ledcAttachChannel(LGpin, P1freq, 16, P1chan);
  ledcAttachChannel(LBpin, P1freq, 16, P1chan);

  //  ledcAttachPin(GPIO_pin, PWM_Ch);
  //  ledcSetup(PWM_Ch, PWM_Freq, PWM_Res);

  ledcWrite (LRpin, 0x80);
  ledcWrite (LGpin, 0x01);
  ledcWrite (LBpin, 0x80);

  // LED drive - invert GPIO
  GPIO.func_out_sel_cfg[LGpin].inv_sel = 1;
#endif

  timeNow = millis();
}


void loop () {
  // Take care of the heartbeat functionality
  if (millis() - timeNow > hbDelay) {
    if (myPower) {
#ifdef ARDUINO
      if (hbPhase) {
        VCU.write(hb1, sizeof(hb1));
        hbPhase = 0;
      } else {
        VCU.write(hb0, sizeof(hb0));
        hbPhase = 1;
      }
#else
      if (hbPhase) {
        write(comPort, hb1, sizeof(hb1));
        hbPhase = 0;
      } else {
        write(comPort, hb0, sizeof(hb0));
        hbPhase = 1;
      }
#endif
    }

    if (myDrive) {
#ifdef ARDUINO
      if (hbPhase) {
        VCU.write(hb2, sizeof(hb2));
      } else {
        VCU.write(bSub, sizeof(bSub));
      }
#else
      if (hbPhase) {
        write(comPort, hb2, sizeof(hb2));
      } else {
        write(comPort, bSub, sizeof(bSub));
      }
#endif
    }

    timeNow += hbDelay;
  }


  static byte RXbuf[0x200];
  static byte TXpkt[0x200];
  static int TXlen = 0;

  // per interface
  for (int ifb = 0; ifb < maxIFB; ifb++) {

    // first we transmit
    TXlen = ifTXlen[ifb];
    if (TXlen) {
      if (TXlen > rxBS) {
        TXlen = rxBS;
      }
#ifdef DEBUG
      printf("TX%d:", ifb);
      for (int w = 0; w < TXlen; w++) {
        printf (" %02x", ifTX[ifb][w]);
      }
      printf ("\n");
#endif

      int TXremaining = (ifTXlen[ifb] - TXlen);
      memcpy (&ifTX[ifb][0], &ifTX[ifb][TXlen], TXremaining);
      ifTXlen[ifb] -= TXlen;

      // static interface routing for now: ie: only transmit to VCU or USB
      if (myPower || 1) { // temp override
#ifdef ARDUINO
        if (ifb == 1) {
          VCU.write (ifTX[ifb], TXlen);
        } else if (ifb == 0) {
          Serial.write (ifTX[ifb], TXlen);
        } else if (ifb == 2) {
          BLE.write (ifTX[ifb], TXlen);
        }
#else
        if (ifb == 1) {
          write(comPort, ifTX[ifb], TXlen);
        }
#endif
      }
    }


    // Receive <= BS bytes from different interfaces
    int rxBytes = 0;
    switch (ifb) {
      case 0:   // The ESP32 USB serial or capture file playback on linux
#ifdef ARDUINO
        rxBytes = Serial.readBytes (RXbuf, rxBS);
#else
        fread (RXbuf, rxBS, 1, datIn);
        rxBytes = rxBS;   // fix this for better testing
        rxBytes = 0;
#endif
        break;
      case 1:   // ESP32>VCU or local serialport on linux
#ifdef ARDUINO
        rxBytes = VCU.readBytes(RXbuf, rxBS);
#else
        rxBytes = read(comPort, RXbuf, rxBS);
#endif
        break;
      case 2: // ESP32>BLE alternate
#ifdef ARDUINO
        rxBytes = BLE.readBytes(RXbuf, rxBS);
#endif
        break;
      case 3:
        break;
      case 4:
        break;
      default:
        break;
    }

    // Did we get RX data?  buffer and process
    if (rxBytes > 0) {
      myPower = 1;
      memcpy (&ifRX[ifb][ifRXlen[ifb]], &RXbuf, rxBytes);
      ifRXlen[ifb] += rxBytes;

      int doMore = 1;
      int rs = ifRXlen[ifb] - 9;  // is there room for another packet?

      int idx = 0;
      for (; (idx < rs) && doMore; idx++) {
        if ((ifRX[ifb][idx] == 0x5a) && doMore) {
          if ((ifRX[ifb][idx + 1] == 0xa5 && doMore)) {
            int len = ifRX[ifb][idx + 2];
            if (ifRXlen[ifb] < (idx + len + 9)) {
              // packet has not been completely received; come back later
              doMore = 0;
              break;
            }

            // verify packet checksum
            int cksum = len;
            int j = (idx + 3);
            for (; j < (idx + len + 7); j++) {
              cksum += ifRX[ifb][j];
            }
            cksum &= 0xffff;
            cksum ^= 0xffff;
            if ((cksum & 0xff) != ifRX[ifb][j]
                || (cksum >> 8) != ifRX[ifb][j + 1]) {
#ifdef DEBUG
              printf ("bad packet %04x %02x- %u %04x\n", idx, len, cksum);
#endif
              // need to signal the drop
            } else {
              // here we have a good packet
#ifdef DEBUG
              printf ("RX%d:", ifb);
#endif
              int doXmit = 1;

              // copy good packet to temp TX buffer
              TXlen = len + 9;
              for (int w = 0; w < TXlen; w++) {
                TXpkt[w] = ifRX[ifb][idx + w];
#ifdef DEBUG
                printf (" %02x", ifRX[ifb][idx + w]);
#endif
              }
#ifdef DEBUG
              printf ("\n");
#endif

              int wr_ack = 0; // generate write acknowledgement hack

              byte pLEN = TXpkt[2];
              byte pSRC = TXpkt[3];
              byte pDST = TXpkt[4];
              byte pCMD = TXpkt[5];
              byte pADR = TXpkt[6]; // don't forget ... words not bytes

              if (pCMD == 0x20) {
                doXmit = 0;
              }

              // detect a variable update and process against our ECU map
              int ECU = 0;
              if (pCMD == 0x04) {
                ECU = pSRC;
              }
              if ((pCMD == 0x02) || (pCMD == 0x03)) {
                ECU = pDST;
              }

#ifdef DEBUG
              printf("CMD %02x\n", pCMD);
#endif

              int dECU = 0;
              if (ECU) {
                switch (ECU) {
                  case 0:
                    break;
                  case 0x16:  // reserved 0x400 bytes
                    dECU = 1;
                    break;
                  case 4:
                    dECU = 3;
                    break;
                  case 7:
                    dECU = 4;
                    break;
                  case 0x23:  // reserved 0x400 bytes
                    dECU = 5;
                    break;
                  case 0x3e:
                    dECU = 7;
                    break;
#ifdef DEBUG
                  default:
                    printf("failed to map ECU %02x\n", ECU);
#endif
                }

                // these flags
                int ECU_read = (1 << 31);   // had read
                int ECU_write = (1 << 30);  // had write
                int ECU_grpChange = (1 << 29);  // did update
                int ECU_valChange = (1 << 28);  // did change
                int ECU_error = (1 << 27);  // had error
                int ECU_bglow = (1 << 26);  // low priority bg reading
                int ECU_noRemap = (1 << 24);// abort processing

                int pkt_writing = 0;
                int grpChange = 0;

                // update ECUmap based on variable access
                if (dECU) {
                  ECUptr = &ECU32[(0x200 * dECU) + (pADR * 2)];
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
                      ECUptr = &ECU32[(0x200 * dECU) + (pADR * 2) + v];
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

                      // byte flagged for modification
                      if (mapFX) {
                        switch (mapFX) {
                          case 0x01:  // static set
                            txValue = modifier;
                            break;
                          case 0x02:  // just lie about it
                            if (pkt_writing) {
                              txValue = modifier;
                            } else {
                              txValue = oldValue;
                              newValue = oldValue;  //
                            }
                            break;
                          case 0x10:  // linear add
                            if (pkt_writing) {
                              txValue += modifier;
                            } else {
                              txValue -= modifier;
                            }
                            break;
                          case 0x11:  // linear subtract
                            if (pkt_writing) {
                              txValue -= modifier;
                            } else {
                              txValue += modifier;
                            }
                            break;
                          case 0x12:  // linear shift
                            if (pkt_writing) {
                              txValue = newValue >> modifier;
                            } else {
                              txValue = newValue << modifier;
                            }
                            break;
                          case 0x13:  // linear shift
                            if (pkt_writing) {
                              txValue = newValue << modifier;
                            } else {
                              txValue = newValue >> modifier;
                            }
                            break;
                          case 0x20:  // fixed replace
                            if (pkt_writing) {
                              modifier = newValue ;
                              txValue = 0x0;
                            } else {
                              newValue = modifier;
                            }
                            break;
                        }
                      }
                      // replace value in new buffer
                      *ECUptr = (newFlags | (modifier << 8) | newValue);
                      TXpkt[v + 7] = txValue;
                    }

                    if (grpChange) {
                      for (int v = 0; v < pLEN; v++) {
                        ECU32[(0x200 * dECU) + (pADR * 2) + v] |= ECU_grpChange;
                      }
                    }
                  }
                }
                /*
                  // this is per the scooter hacking PDF
                            if (wr_ack) {
                              pLEN = 0;
                              TXpkt[3] = pDST;
                              TXpkt[4] = pSRC;
                              TXpkt[5] = 0x05; // write response
                              TXpkt[6] = 0; // write success
                            }
                */

                // this is per sniffing
                // BUG rewriting the buffer this way means the real write packet doesn't get forwarded
                if (wr_ack) {
                  pLEN = 2;
                  TXpkt[3] = pDST;
                  TXpkt[4] = pSRC;
                  TXpkt[5] = 0x05; // write response
                  // address
                  TXpkt[7] = 1; // write success
                  TXpkt[8] = 0; // write success
                }

                // recalculate packet checksum
                int cksum = pLEN;
                TXpkt[2] = pLEN;
                for (int v = 0; v < (pLEN + 4); v++) {
                  cksum += TXpkt[v + 3];
                }
                cksum &= 0xffff;
                cksum ^= 0xffff;
                TXpkt[pLEN + 7] = (cksum & 0xff);
                TXpkt[pLEN + 8] = (cksum >> 8);
              } // "if ECU"

              if (doXmit) {
#ifdef DEBUG
                printf("PKT:");
                for (int v = 0; v < TXlen; v++) {
                  printf(" %02x", TXpkt[v]);
                }
                printf("\n");
#endif
                // basic static routing for now
                if ((ifb == 0) || (ifb == 2) || (wr_ack)) {   // hardcode write ack BUG
                  memcpy (&ifTX[1][ifTXlen[1]],
                          &ifRX[ifb][idx], (len + 9));
                  ifTXlen[1] += (len + 9);
                  wr_ack = 0;
                } else if (ifb == 1) {
                  memcpy (&ifTX[0][ifTXlen[0]],
                          &ifRX[ifb][idx], (len + 9));
                  ifTXlen[0] += (len + 9);

                  memcpy (&ifTX[2][ifTXlen[2]],
                          &ifRX[ifb][idx], (len + 9));
                  ifTXlen[2] += (len + 9);
                }
#ifdef DEBUG
                usleep(1000);
#endif
              }

            }
          }
          // move next packet to start buffer
          int remaining = (sizeof (RXbuf) - idx);
          //BUG - when bad characters are present at the start, this copies them; they should be dropped.
          memcpy (&ifRX[ifb][0], &ifRX[ifb][idx], remaining);
          ifRXlen[ifb] -= idx;

          // here's a good place to background-read
          // if (ifRXlen[ifb] || ifTXlen[ifb]) unschedule
          // if ifRXlen[ifb]=0 && ifTXlen[ifb]=0 schedule for +20ms

        }   // just finished, "yes it's a packet"
      }
    }

    // BUG hardcoded offsets are bad
    //modinit     int mySpeed = ((ECU32[(0x200 * 5) + (0xfe * 2) + 1 ] & 0xff ) ); //23:fe>>8
    int mySpeed = ((ECU32[(0x200 * 5) + (0x106 * 2) + 1 ] & 0xff ) ); //23:fe>>8
    int myBoost = ((ECU32[(0x200 * 5) + (0x109 * 2) + 1 ] & 0xff ) );
    int myBoostFlag = ((ECU32[(0x200 * 5) + (0x10a * 2)  ] & 0xff ) ); // boost

    //modinit    int myBatt = (ECU32[(0x200 * 5) + (0xff * 2) + 1 ] ) & 0xff; //23:ff>>8
    int myBatt = (ECU32[(0x200 * 5) + (0x10b * 2) + 1 ] ) & 0xff; //23:ff>>8
    int myMode = (ECU32[(0x200 * 5) + (0x10d * 2) + 1 ] ) & 0xff;
    int myParm = (ECU32[(0x200 * 5) + (0x10e * 2) + 1 ] ) & 0x80; // 2WD
    int myBrake = (ECU32[(0x200 * 5) + (0x10e * 2) + 1 ] ) & 0x01; //eBrake
    //    int myTest = (((ECU32[(0x200 * 5) + (0x10e * 2) + 1 ] ) ^ 0x58)  & ~0xa2) + (ECU32[(0x200 * 5) + (0x10e * 2) + 2 ] );
    int myTest = (ECU32[(0x200 * 5) + (0x10f * 2) + 0 ] ) & 0xfc;

    int testPower = ((ECU32[(0x200 * 3) + (0x51 * 2)  ] & 0xffff ) );  // 04:51, actually means "unlocked"

    if ((testPower == 0) && (myPower > 0)) {
      myPower = 0;
#ifdef ESPLED
      FastLED.clear();
      FastLED.show();
      FastLED.show();
#endif
    }

    //23:25==1?  screen on
    if ((ECU32[(0x200 * 5) + (0x25 * 2)  ] & 0xffff ) == 1) {
      myDrive = 1;
    } else {
      myDrive = 0;
    }

    if (myPower == 0) {
      myDrive = 0;
    }

    static int myTopSpd = 0;
    static int myTopDly = 0;
    if (mySpeed > myTopSpd) {
      myTopSpd = mySpeed;
      myTopDly = 333333;  // BUG make it millis() based
    }
    static int mySpeedAvg;
    mySpeedAvg *= 7;
    mySpeedAvg += mySpeed;
    mySpeedAvg /= 8;
#ifdef ESPLED
    for (int l = 0; l < 8; l++) {
      int c1 = 0;
      int c2 = 0;
      int c3 = 0;

      if (myBoost > (l * 25)) {
        c1 = 15;
        c2 = 7;
        c3 = 1;
      }
      if (mySpeedAvg > (l * 10)) {
        c2 = 31;
        c3 = 3;
      }
      if (mySpeed > (l * 10)) {
        c1 = 255;
      }
      if (myTopSpd > (l * 10)) {
        c3 = 31;
      }
      leds[l].setRGB(c2, c1, c3);
    }
    if (1) {
      switch (myMode) {
        case 1: //walk
          leds[7].setRGB(0, 0, 32);
          break;
        case 2: //eco
          if (myParm) {
            leds[7].setRGB(1, 24, 8);
          } else {
            leds[7].setRGB(0, 26, 0);
          }
          break;
        case 3: //sport
          if (myParm) {
            leds[7].setRGB(14, 18, 2);
          } else {
            leds[7].setRGB(26, 12, 0);
          }
          break;
        case 4: //race
          if (myParm) {
            leds[7].setRGB(24, 1, 8);
          } else {
            leds[7].setRGB(34, 0, 0);
          }
          break;
        default:
          leds[7].setRGB(1, 1, 1);
          break;
      }
    }

    for (int l = 0; l < 8; l++) {
      int c1 = (myBatt * myBatt) >> 8;
      if (c1 > 40) {
        c1 = 40;
      }
      int c2 = 40 - c1;
      int c3 = 0;
      if (((l + 1) * 10) > myBatt) {
        c1 = 0;
        c2 = 0;
        c3 = 0;
      }
      leds[15 - l].setRGB(c2, c1, 0);
    }

    // check for driving events
    if (myTest & ! myBrake) {
      //      if (flicker) {
      if (flash) {
        leds[8].setRGB(255, 0, 255);
      } else {
        leds[8].setRGB(0, 255, 0);
      }
      //      }
    }
    if (myBrake & flicker) {
      leds[8].setRGB(255, 255, 255);
    }

    if (ledDly) {
      ledDly--;
    } else {
      ledDly = 2000;
      ledBri++;
      while (ledBri > 255) {
        ledBri -= 256;
      }
      ledcWrite (LGpin, table[ledBri]);
      if (myPower) {
        FastLED.show();
        flicker = 1 - flicker;
        if (flicker) {
          flash = 1 - flash;
        }
      }
    }
    if (myTopDly) {
      myTopDly--;
    } else {
      myTopDly = 3000;
      if (myTopSpd) {
        myTopSpd--;
      }
    }
#endif
  }
}


#ifndef ARDUINO
int main () {
  setup ();
  while (running)
  {
    loop ();
  }
}
#endif
