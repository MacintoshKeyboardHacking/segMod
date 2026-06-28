/*
 * BLEbot_app_connect.ino — BLEbot3 BLE setup + Encryption2 protocol
 * 
 * Uses the SAME BLE services as BLEbot3 (Nordic UART + FE95) which the App can find.
 * Adds Encryption2 crypto + 9bot handshake protocol handling.
 *
 * BLE555 Manufacturer Data format triggers the App's G30/Max connection flow.
 * 
 * CRITICAL: The Ninebot Custom Service (6E400001-0000-0000-006E-696E65626F74)
 * MUST be created + advertised for the app to see Encryption2 support.
 * Without it, the app falls back to plaintext Protocol 2 and gets stuck.
 * 
 * CRITICAL: Manufacturer data byte[3]=0x02 tells the app this device supports
 * Encryption2 (encrypt version = 2). Without it, the app never enables Encryption2.
 * 
 * CRITICAL: sendFrame must reply on the SAME channel the request came from.
 * If the request came via Nordic UART, response goes via Nordic UART.
 * If via Ninebot Custom, response goes via Ninebot Custom.
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha1.h>

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
*/


// ============================================================
// BLE Services — MUST match BLEbot3 for App discovery
// ============================================================
#define UART      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_RX   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_TX   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define SERVICE2_UUID "FE95"
// Ninebot Custom Service — used by modern app versions for Encryption2
// This service MUST be created + advertised for the app to attempt Encryption2!
#define NBSVC     "6E400001-0000-0000-006E-696E65626F74"
#define NB_RX     "6E400002-0000-0000-006E-696E65626F74"  // App → Device writes
#define NB_TX     "6E400004-0000-0000-006E-696E65626F74"  // Device → App notify

// ============================================================
// CONFIGURATION
// ============================================================
// BLE555 format with Encryption2 version flag.
// Byte layout: [0x4E, 0x42, HW_ID, ENC_VER, 0x00, 0x00, 0x00, CHK]
//   Company ID (LE): 0x4E42 = 20034 (Ninebot)
//   Byte[2] = 0x24: HW_ID = 36 (Ninebot Max G30)
//   Byte[3] = 0x02: encrypt version = 2 (Encryption2!)  ← CRITICAL for phone app
//   Byte[4] = 0x02: duplicate for byte-swapped CID path	// ?
//   Byte[7] = 0xD7: checksum/footer byte
// static char MANUFDAT[8] = { 0x4E, 0x42, 0x24, 0x02, 0x02, 0x00, 0x00, 0xD7 };
// ============================================================                            
// Segway ZT3 format with Encryption2 version flag.                                            
// Byte layout: [0x4E, 0x43, HW_ID_H, HW_ID_L, ENC_VER, 0x00, 0x00, CHK]                       
//   Company ID (LE): 0x4E43 = 20035 (Ninebot x3)                                             
//   Byte[2] = 0x01: HW_ID_H = 1 (Ninebot ZT3)                                          
//   Byte[3] = 0x00: HW_ID_L = 0 (Ninebot ZT3)                                          
//   Byte[4] = 0x02: encrypt version = 2 (Encryption2!)  ← CRITICAL for phone app          
//   Byte[7] = 0xFC: checksum/footer byte                                                  
static char MANUFDAT[8] = { 0x4E, 0x43, 0x01, 0x00, 0x02, 0x00, 0x00, 0xFC };           

// Device name — BLEbot3 uses "ESP32" or "Ninebot_1234", both work
#define DEVICE_NAME "1K1EA2452P0889"	// ZT3

// Serial number (14 chars) — used as V2 encryption key and in auth
#define DEVICE_SERIAL "1K1EA2452P0889"	// ZT3

// 6-digit BLE password (stored in register 0x17 on BLE555 devices)
#define BLE_PASSWORD "123456"

// FW_DATA from libnbcrypto.so (Gen2 non-SN ECB input)
static const uint8_t FW_DATA[16] = {
  0x97, 0xCF, 0xB8, 0x02, 0x84, 0x41, 0x43, 0xDE,
  0x56, 0x00, 0x2B, 0x3B, 0x34, 0x78, 0x0A, 0x5D
};

// Protocol constants
static const uint8_t CMD_READ = 0x01, CMD_WRITE = 0x02, CMD_WRITE_NR = 0x03;
static const uint8_t CMD_READ_RESP = 0x04, CMD_WRITE_RESP = 0x05;
static const uint8_t CMD_PRE_COMM = 0x5B, CMD_SET_PWD = 0x5C, CMD_AUTH = 0x5D;

// Helpers for setting LE16 values: put_16(buf, addr, value) writes value in LE at addr
#define put_16(r, a, v) do { r[(a)] = (v)&0xFF; r[(a)+1] = ((v)>>8)&0xFF; } while(0)
#define put_32(r, a, v) do { r[(a)] = (v)&0xFF; r[(a)+1] = ((v)>>8)&0xFF; r[(a)+2] = ((v)>>16)&0xFF; r[(a)+3] = ((v)>>24)&0xFF; } while(0)

// Board IDs
static const uint8_t BOARD_DIS = 0x01, BOARD_BLE = 0x04;
static const uint8_t BOARD_CTRL = 0x20, BOARD_BMS1 = 0x22, BOARD_BMS2 = 0x23;
static const uint8_t BOARD_VCU = 0x09, BOARD_VCU2 = 0x16, BOARD_TFT = 0x23;

// ============================================================
// Encryption2 Crypto
// ============================================================
static uint8_t aes_key[16];
static uint8_t auth_param[16] = {0};
static uint8_t ecb_input[16];
static int crypto_counter = 0;  // 0 = non-SN, >0 = SN mode

void derive_key(const uint8_t* k1, int k1l, const uint8_t* k2, int k2l, uint8_t* out) {
  uint8_t buf[32]; memset(buf, 0, 32);
  memcpy(buf, k1, k1l > 16 ? 16 : k1l);
  if (k2 && k2l > 0) memcpy(buf + 16, k2, k2l > 16 ? 16 : k2l);
  uint8_t sha[20];
  mbedtls_sha1(buf, 32, sha);
  memcpy(out, sha, 16);
}

void aes_enc(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, key, 128);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, in, out);
  mbedtls_aes_free(&ctx);
}

void set_key(const uint8_t* k1, int k1l, const uint8_t* k2, int k2l) {
  derive_key(k1, k1l, k2, k2l, aes_key);
}

void ctr_xor(const uint8_t* data, uint8_t* out, int len, int sb) {
  uint8_t ab[16]; int off = 0, bi = sb;
  while (off < len) {
    ab[0] = 0x01;
    if (crypto_counter > 0) {
      ab[1]=(crypto_counter>>24)&0xFF; ab[2]=(crypto_counter>>16)&0xFF;
      ab[3]=(crypto_counter>>8)&0xFF;  ab[4]=crypto_counter&0xFF;
    } else { memset(&ab[1],0,4); }
    memcpy(&ab[5], auth_param, 8);
    ab[13]=0; ab[14]=0; ab[15]=bi&0xFF;
    uint8_t ks[16]; aes_enc(aes_key, ab, ks);
    int c = (len-off>16)?16:(len-off);
    for (int j=0;j<c;j++) out[off+j]=data[off+j]^ks[j];
    off+=c; bi++;
  }
}

void compute_tag(const uint8_t* pt, int ptlen, int ctr, uint8_t tag[4]) {
  uint8_t nonce[13];
  nonce[0]=(ctr>>24)&0xFF; nonce[1]=(ctr>>16)&0xFF;
  nonce[2]=(ctr>>8)&0xFF;  nonce[3]=ctr&0xFF;
  memcpy(&nonce[4], auth_param, 8); nonce[12]=0;
  
  uint8_t b0[16], x[16];
  b0[0]=0x59; memcpy(&b0[1],nonce,13); b0[14]=0; b0[15]=(ptlen-3)&0xFF;
  aes_enc(aes_key, b0, x);
  uint8_t aad[16]; memset(aad,0,16); memcpy(aad,pt,3);
  for (int i=0;i<16;i++) x[i]^=aad[i];
  aes_enc(aes_key, x, x);
  int off=0, plen=ptlen-3;
  while (off<plen) {
    uint8_t blk[16]; int c=(plen-off>16)?16:(plen-off);
    memset(blk,0,16);
    for (int i=0;i<c;i++) blk[i]=pt[3+off+i];
    for (int i=0;i<16;i++) x[i]^=blk[i];
    aes_enc(aes_key, x, x); off+=16;
  }
  tag[0]=x[0]; tag[1]=x[1]; tag[2]=x[2]; tag[3]=x[3];
}

int encrypt_frame(const uint8_t* pt, int ptlen, uint8_t* out) {
  int len=ptlen-3;
  out[0]=pt[0]; out[1]=pt[1]; out[2]=pt[2];
  if (crypto_counter>0) {
    crypto_counter++; int ctr=crypto_counter;
    uint8_t nonce[13];
    nonce[0]=(ctr>>24)&0xFF; nonce[1]=(ctr>>16)&0xFF;
    nonce[2]=(ctr>>8)&0xFF;  nonce[3]=ctr&0xFF;
    memcpy(&nonce[4],auth_param,8); nonce[12]=0;
    ctr_xor(&pt[3],&out[3],len,1);
    uint8_t raw_tag[4]; compute_tag(pt,ptlen,ctr,raw_tag);
    uint8_t a0[16]={0x01}; memcpy(&a0[1],nonce,13); a0[14]=0; a0[15]=0;
    uint8_t ks0[16]; aes_enc(aes_key,a0,ks0);
    for (int i=0;i<4;i++) out[3+len+i]=raw_tag[i]^ks0[i];
    out[3+len+4]=(ctr>>8)&0xFF; out[3+len+5]=ctr&0xFF;
    return 3+len+6;
  } else {
    uint8_t ks[16]; aes_enc(aes_key,ecb_input,ks);
    for (int i=0;i<len;i++) out[3+i]=pt[3+i]^ks[i%16];
    uint16_t sum=0; for(int i=0;i<len;i++) sum+=pt[3+i]; sum=(~sum)&0xFFFF;
    out[3+len+0]=0; out[3+len+1]=0;
    out[3+len+2]=sum&0xFF; out[3+len+3]=(sum>>8)&0xFF;
    out[3+len+4]=0; out[3+len+5]=0;
    return 3+len+6;
  }
}

int decrypt_frame(const uint8_t* f, int flen, uint8_t* pt) {
  if (flen<9) return -1;
  int len=flen-9;
  pt[0]=f[0]; pt[1]=f[1]; pt[2]=f[2];
  const uint8_t* tail=&f[3+len];
  uint16_t recv_ctr=(tail[4]<<8)|tail[5];
  if (recv_ctr>0) {
    int saved=crypto_counter; crypto_counter=recv_ctr;
    uint8_t nonce[13];
    nonce[0]=(recv_ctr>>24)&0xFF; nonce[1]=(recv_ctr>>16)&0xFF;
    nonce[2]=(recv_ctr>>8)&0xFF;  nonce[3]=recv_ctr&0xFF;
    memcpy(&nonce[4],auth_param,8); nonce[12]=0;
    ctr_xor(&f[3],&pt[3],len,1);
    uint8_t a0[16]={0x01}; memcpy(&a0[1],nonce,13); a0[14]=0; a0[15]=0;
    uint8_t ks0[16]; aes_enc(aes_key,a0,ks0);
    uint8_t recv_tag[4]; for(int i=0;i<4;i++) recv_tag[i]=tail[i]^ks0[i];
    crypto_counter=saved;
    uint8_t expected[4]; compute_tag(pt,len+3,recv_ctr,expected);
    if (memcmp(recv_tag,expected,4)!=0) {
      Serial.printf("[CRYPTO] TAG MISMATCH! ctr=%d\n", recv_ctr);
    } else {
      Serial.printf("[CRYPTO] TAG OK! ctr=%d\n", recv_ctr);
    }
    return len+7;
  } else {
    uint8_t ks[16]; aes_enc(aes_key,ecb_input,ks);
    for(int i=0;i<len;i++) pt[3+i]=f[3+i]^ks[i%16];
    return len+7;
  }
}

// ============================================================
// Globals
// ============================================================
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;    // Nordic UART TX (0003)
BLECharacteristic* pNBCharacteristic = NULL;    // Ninebot Custom TX (0004)
BLECharacteristic* pNBWrite = NULL;             // Ninebot Custom RX (0002)
bool deviceConnected = false, oldDeviceConnected = false, authDone = false;

// Tracks which BLE characteristic received the last incoming frame.
// This ensures sendFrame replies on the SAME channel.
enum RxChannel { CH_UART, CH_NB };
static RxChannel activeChannel = CH_UART;

static uint8_t regs[8][256];
static uint8_t rxBuf[512];
static int rxBufLen = 0;
static unsigned long rxCount = 0, txCount = 0, badCrc = 0;

int boardIdx(uint8_t b) {
  switch(b) {
    case 0x01: return 0; case 0x04: return 1; case 0x09: return 2;
    case 0x16: return 3; case 0x20: return 4; case 0x22: return 5;
    case 0x23: return 6; default: return -1;
  }
}

uint16_t calcCRC(uint8_t* buf, int s, int e) {
  uint16_t sum=0; for(int i=s;i<e;i++) sum+=buf[i];
  return 0xFFFF-(sum&0xFFFF);
}

static bool appSendingEncrypted = false;

void sendFrame(uint8_t src, uint8_t dst, uint8_t cmd, uint8_t idx,
               uint8_t* data, uint8_t len) {
  uint8_t pt[264];
  pt[0]=0x5A; pt[1]=0xA5; pt[2]=len;
  pt[3]=src; pt[4]=dst; pt[5]=cmd; pt[6]=idx;
  for(int i=0;i<len;i++) pt[7+i]=data[i];
  int ptlen;
  if (appSendingEncrypted) {
    ptlen = 7 + len;
  } else {
    uint16_t crc=calcCRC(pt,2,7+len);
    pt[7+len]=crc&0xFF; pt[8+len]=crc>>8;
    ptlen=9+len;
  }
  
  uint8_t outbuf[280];
  int outlen;
  
  if (appSendingEncrypted) {
    outlen = encrypt_frame(pt, ptlen, outbuf);
  } else {
    memcpy(outbuf, pt, ptlen);
    outlen = ptlen;
  }
  
  // CRITICAL: Reply on the SAME channel the request came from.
  // - If the request came via Ninebot Custom (CH_NB), respond on NB_TX
  // - If the request came via Nordic UART (CH_UART), respond on UART_TX
  BLECharacteristic* txChar;
  if (activeChannel == CH_NB && pNBCharacteristic) {
    txChar = pNBCharacteristic;
  } else {
    txChar = pTxCharacteristic;
  }
  
  if (deviceConnected && txChar) {
    txChar->setValue(outbuf, outlen);
    txChar->notify();
    txCount++;
    Serial.printf("[TX] %02X->%02X CMD:%02X IDX:%02X L:%d enc=%s ctr=%d ch=%s\n",
                  src,dst,cmd,idx,len,
                  (crypto_counter>0)?"Y":"N", crypto_counter,
                  (txChar == pNBCharacteristic) ? "NB" : "UART");
  }
}

void processFrame(uint8_t* f, int flen) {
  if (flen<7) return;
  uint8_t len=f[2], src=f[3], dst=f[4], cmd=f[5], idx=f[6];
  uint8_t* data=&f[7];
  Serial.printf("[RX] %02X->%02X CMD:%02X IDX:%02X LEN:%d\n",src,dst,cmd,idx,len);
  if (len > 0) {
    Serial.print("  DATA: ");
    for(int i=0;i<len&&i<32;i++){if(data[i]<16)Serial.print("0");Serial.print(data[i],HEX);Serial.print(" ");}
    Serial.println();
  }

  switch(cmd) {
    case CMD_PRE_COMM: {
      Serial.println("[HANDLER] PRE_COMM -> 30 bytes (non-SN response)");
      uint8_t resp[30]; memset(resp,0,30);
      memcpy(resp,auth_param,16);
      memcpy(resp+16,DEVICE_SERIAL,14);
      sendFrame(BOARD_BLE,src,CMD_PRE_COMM,0x00,resp,30);
      set_key((const uint8_t*)DEVICE_NAME, strlen(DEVICE_NAME), auth_param, 16);
      crypto_counter = 1;
      Serial.printf("[HANDLER] Key updated to SN mode\n");
      break;
    }
    case CMD_READ: {
      uint8_t rlen=len>0?data[0]:2;
      int bi=boardIdx(dst);
      Serial.printf("[HANDLER] READ board=0x%02X idx=0x%02X len=%d\n", dst, idx, rlen);
      uint8_t resp[32]; memset(resp,0,32);
      if(bi>=0) {
        for(int i=0;i<rlen&&i<32;i++) resp[i]=regs[bi][idx+i];
      }
      Serial.print("  RSP: ");
      for(int i=0;i<rlen&&i<32;i++){if(resp[i]<16)Serial.print("0");Serial.print(resp[i],HEX);Serial.print(" ");}
      Serial.println();
      sendFrame(dst,src,CMD_READ_RESP,idx,resp,rlen);
      break;
    }
    case CMD_WRITE:
    case CMD_WRITE_NR: {
      int bi=boardIdx(dst);
      if(bi>=0) for(int i=0;i<len;i++) regs[bi][idx+i]=data[i];
      if(cmd==CMD_WRITE) { uint8_t ack[]={0x01}; sendFrame(dst,src,CMD_WRITE_RESP,idx,ack,1); }
      break;
    }
    case CMD_SET_PWD: {
      Serial.println("[HANDLER] SET_PWD -> success");
      uint8_t pwd[16]; memset(pwd,0,16);
      for(int i=0;i<len&&i<16;i++) pwd[i]=data[i];
      uint8_t ack[]={0x01}; sendFrame(BOARD_BLE,src,CMD_SET_PWD,0x01,ack,1);
      set_key(pwd,16,auth_param,16);
      authDone=true; break;
    }
    case CMD_AUTH: {
      Serial.println("[HANDLER] AUTH -> success");
      uint8_t resp[14]; memset(resp,0,14);
      memcpy(resp,DEVICE_SERIAL,14);
      sendFrame(BOARD_BLE,src,CMD_AUTH,0x01,resp,14);
      authDone=true; break;
    }
    default:
      Serial.printf("[HANDLER] Unknown CMD 0x%02X\n",cmd);
      break;
  }
}

void initRegs() {
  memset(regs,0,sizeof(regs));
  const char* sn=DEVICE_SERIAL;
  
  // ═══════════════════════════════════════════════════════════════
  // DISPLAY (board 0x01, index 0) — register map from nb_protocol.py
  // NOTE: Some registers overlap because they map to different
  // physical device registers that happen to share address space
  // in the protocol definition. We write in client-read order.
  // ═══════════════════════════════════════════════════════════════
  
  // rSN at 0x10 (14 bytes ASCII)
  for(int i=0;i<14&&sn[i];i++) regs[0][0x10+i]=sn[i];
  
  put_16(regs[0], 0x1A, 0x0403); // rDisVersion: v4.3 (client shows DIS=3.0.4 from LE16 [0x04,0x03])
  put_16(regs[0], 0x1C, 0x3036); // rAlarm: 0x3036 (typical value)
  put_16(regs[0], 0x24, 250);    // rSigMaxSpeed: 25.0 km/h = 250 (note: shared byte 0x25!)
  put_16(regs[0], 0x25, 150);    // rLeftMileage: 15.0 km = 150 (LE16 at 0x25: [0x96,0x00])
  put_16(regs[0], 0x26, 0);      // rSpeed: 0.0 km/h
  put_16(regs[0], 0x27, 125);    // rAveSpeed: 12.5 km/h = 125
  put_16(regs[0], 0x28, 0x0102); // rMCUVersion: v1.2 → [0x02, 0x01] in LE? client reads LE16
  put_16(regs[0], 0x30, 0x0201); // rBms1Version: v2.1 → LE16 [0x01,0x02]
  put_16(regs[0], 0x31, 0x0201); // rBms2Version: v2.1
  put_16(regs[0], 0x39, 120);    // rTimeFull: 120 min
  put_16(regs[0], 0x48, 250);    // rRatedSpeed: 25.0 km/h = 250
  put_16(regs[0], 0x4C, 0x0100); // rEcuVersion: v1.0 → LE16 [0x00,0x01] — but client shows ECU=1.0.0 from LE16
  put_16(regs[0], 0x74, 0);      // rCfgMode: 0=metric
  put_16(regs[0], 0x75, 50);     // rWarn: volume=50%, preset=0
  put_16(regs[0], 0x76, 80);     // rAlarmVolume: 80%
  put_16(regs[0], 0x77, 2);      // rAlarmLevel: 2=Standard
  put_16(regs[0], 0x7B, 100);    // rLightLevel: 100
  put_16(regs[0], 0x7C, 0x2400); // rCTLBool2: bit10+bit13
  put_16(regs[0], 0x7D, 0x7420); // rFunBool: [0x20,0x74]
  put_16(regs[0], 0x84, 0);      // rCTLBool: 0
  put_16(regs[0], 0x85, 10);     // rAutoLock: 10s
  put_16(regs[0], 0x8A, 0x0010); // rFunAppBool: bit4=elec brake
  put_16(regs[0], 0x8C, 0x0002); // rFunAppBool2: bit1=TCS
  put_16(regs[0], 0x93, 30);     // rLimitSpeed: 30 km/h
  put_16(regs[0], 0xAA, 0x0005); // rBool2: ECO|Furious (bit0|bit2)
  put_16(regs[0], 0xAF, 0);      // rPreciseMileage: 0
  put_16(regs[0], 0xB2, 0x0B41); // rBool: 0x0B41
  put_16(regs[0], 0xB5, 100);    // rBattery: 100%
  put_32(regs[0], 0xB7, 123456); // rMileage: 123456 = 123.456 km (LE32)
  put_16(regs[0], 0xB9, 250);    // rSingleMileage: 2.5 km = 250 (overlaps with running_time at 0xBA!)
  put_16(regs[0], 0xBA, 360);    // rRunningTime: 360s (overlaps with single_mileage hi byte at 0xBA)
  put_16(regs[0], 0xBD, 0);      // rPower: 0W (signed LE16)
  
  // ═══ BLE (board 0x04, index 1) ═══
  // CRITICAL: BLE registers 0x00-0x02 tell the phone app what protocol/encryption to use.
  // From protocol.md: protocol=2 AND encrypt=2 → BleEncryption2Protocol2
  regs[1][0x00]=0x02;  // Protocol version: 2 = Encryption2
  regs[1][0x01]=0x02;  // Encrypt version: 2 = AES-128
  regs[1][0x02]=0x01;  // Feature/status flags
  put_16(regs[1], 0x68, 0x0604); // rBleVersion: v1.6.4 → LE16 [0x04,0x06]
  regs[1][0xA2]=0x01;
  
  // ═══ VCU (board 0x09, index 2) ═══
  put_16(regs[2], 0x03, 511);    // rMainPower: 511=on
  const char* ecu_pn = "ECU-PN-9G12345";
  for(int i=0;i<14&&ecu_pn[i];i++) regs[2][0x28+i]=ecu_pn[i];
  put_16(regs[2], 0x52, 1);      // rStateBool: running
  
  // ═══ CTRL (board 0x20, index 4) ═══
  for(int i=0;i<14&&sn[i];i++) regs[4][0x10+i]=sn[i];
  const char* pwd=BLE_PASSWORD;
  // rBlePwd at 0x17 (6 bytes ASCII): "123456" = 31 32 33 34 35 36
  for(int i=0;i<6&&pwd[i];i++) regs[4][0x17+i]=pwd[i];
  // CRITICAL REGISTERS for Encryption2 handshake trigger:
  regs[4][0x00]=0x02;  // protocol=2 (Encryption2)
  regs[4][0x01]=0x02;  // encrypt=2 (AES-128)
  // rEncryptionFlag at 0x91 (LE16) — must be non-zero to enable Encryption2 mode
  put_16(regs[4], 0x91, 0x0001); // Encryption2 enabled
  regs[4][0xB2]=0x02; regs[4][0xB3]=0x28;
  regs[4][0xB4]=0x64;
  put_16(regs[4], 0x68, 0x0164);
  regs[4][0x75]=0x02;
  regs[4][0x7D]=0x20; regs[4][0x7E]=0x74;
  
  // ═══ BMS1 (board 0x22, index 5) ═══
  put_16(regs[5], 0x1A, 3792);   // rVoltage: 37.92V = 3792
  put_16(regs[5], 0x32, 100);    // rBmsSOC: 100%
  put_16(regs[5], 0x34, 3792);   // SOC voltage
  
  Serial.printf("[INIT] Registers initialized. SN=%s PWD=%s BMS=%.2fV\n",
    sn, pwd, 3792/100.0);
}

// ============================================================
// BLE Callbacks
// ============================================================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s, esp_ble_gatts_cb_param_t* p) {
    deviceConnected = true; authDone = false;
    crypto_counter=0;
    appSendingEncrypted = false;
    activeChannel = CH_UART;  // Default to UART until we know which channel
    memcpy(ecb_input,FW_DATA,16);
    for(int i=0;i<16;i++) auth_param[i]=random(0,256);
    set_key((const uint8_t*)DEVICE_NAME,strlen(DEVICE_NAME),FW_DATA,16);
    Serial.printf("[BLE] Connected: %02x:%02x:%02x:%02x:%02x:%02x\n",
      p->connect.remote_bda[0],p->connect.remote_bda[1],
      p->connect.remote_bda[2],p->connect.remote_bda[3],
      p->connect.remote_bda[4],p->connect.remote_bda[5]);
    s->updateConnParams(p->connect.remote_bda,6,12,0,200);
  }
  void onDisconnect(BLEServer* s) {
    deviceConnected=false; authDone=false; crypto_counter=0; appSendingEncrypted=false; activeChannel=CH_UART;
    Serial.println("[BLE] Disconnected");
  }
  void onMtuChanged(BLEServer* s, esp_ble_gatts_cb_param_t* p) {
    uint16_t connId = s->getConnId();
    uint16_t mtu = s->getPeerMTU(connId);	// this needs to be global
    Serial.printf("[BLE] Peer MTU: %d\n", mtu);
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
private:
  RxChannel m_channel;
public:
  MyCallbacks(RxChannel ch) : m_channel(ch) {}
  
  void onWrite(BLECharacteristic* c) {
    String v=c->getValue();
    if(v.length()==0) return;
    
    // Track which channel this frame came through
    activeChannel = m_channel;
    
    rxCount++;
    int rxLen=v.length();
    
    Serial.printf("[BLE RAW %d] %s: ",rxLen, (m_channel==CH_NB)?"NB":"UART");
    for(int i=0;i<rxLen&&i<32;i++){if((uint8_t)v[i]<16)Serial.print("0");Serial.print((uint8_t)v[i],HEX);Serial.print(" ");}
    Serial.println();
    
    if(rxBufLen+rxLen<(int)sizeof(rxBuf)){memcpy(&rxBuf[rxBufLen],v.c_str(),rxLen);rxBufLen+=rxLen;}
    
    int idx=0;
    while(idx<rxBufLen-7){
      if(rxBuf[idx]==0x5A&&(rxBuf[idx+1]==0xA5||rxBuf[idx+1]==0xB5)){
        uint8_t dLen=rxBuf[idx+2];
        
        // Try plaintext CRC first (LEN+9) - Protocol 2 unencrypted format
        if(idx+dLen+9<=rxBufLen){
          uint16_t crc=calcCRC(rxBuf,idx+2,idx+2+dLen+5);
          if((crc&0xFF)==rxBuf[idx+dLen+7]&&(crc>>8)==rxBuf[idx+dLen+8]){
            processFrame(&rxBuf[idx],dLen+9);
            idx+=dLen+9;
            continue;
          }
        }
        
        // Try SN-encrypted (LEN+13) - Encryption2 format with counter>0
        if(idx+dLen+13<=rxBufLen){
          uint8_t pt[280];
          int ptlen=decrypt_frame(&rxBuf[idx],dLen+13,pt);
          if(ptlen>0){
            appSendingEncrypted=true;
            Serial.printf("[DECRYPTED SN]: ");
            for(int i=0;i<ptlen&&i<24;i++){if(pt[i]<16)Serial.print("0");Serial.print(pt[i],HEX);Serial.print(" ");}
            Serial.println();
            processFrame(pt,ptlen);
            idx+=dLen+13;
            continue;
          }
        }
        
        // Try non-SN encrypted (LEN+9, counter=0 in tail)
        if(idx+dLen+9<=rxBufLen){
          uint16_t tail_ctr=(rxBuf[idx+dLen+7]<<8)|rxBuf[idx+dLen+8];
          if(tail_ctr==0){
            uint8_t pt[280];
            int saved_ctr=crypto_counter;
            crypto_counter=0;
            int ptlen=decrypt_frame(&rxBuf[idx],dLen+9,pt);
            crypto_counter=saved_ctr;
            if(ptlen>0){
              appSendingEncrypted=true;
              Serial.printf("[DECRYPTED nonSN]: ");
              for(int i=0;i<ptlen&&i<24;i++){if(pt[i]<16)Serial.print("0");Serial.print(pt[i],HEX);Serial.print(" ");}
              Serial.println();
              processFrame(pt,ptlen);
              idx+=dLen+9;
              continue;
            }
          }
        }
        
        idx++;
      } else {idx++;}
    }
    if(idx>0){
      if(idx<rxBufLen){memmove(rxBuf,&rxBuf[idx],rxBufLen-idx);rxBufLen-=idx;}
      else rxBufLen=0;
    }
  }
};

// FE95 callback
class tweakCB : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* t) {
    String manStr=t->getValue();
    if(manStr.length()>0){
      Serial.print("[FE95] TWEAK: ");
      for(int i=0;i<manStr.length();i++){if(manStr[i]<16)Serial.print('0');Serial.print(manStr[i],HEX);}
      Serial.println();
    }
    BLEAdvertising* pAdv=BLEDevice::getAdvertising();
    BLEAdvertisementData ad;
    ad.setManufacturerData(manStr);
    pAdv->setAdvertisementData(ad);
  }
};

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BLEbot App Connect (BLEbot3 BLE + Encryption2) ===");
  Serial.printf("Device: %s  Password: %s\n", DEVICE_NAME, BLE_PASSWORD);
  
  initRegs();
  memcpy(ecb_input,FW_DATA,16);

  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // ── Nordic UART Service (compatibility — plaintext Protocol 2) ──
  BLEService* pService = pServer->createService(UART);
  BLECharacteristic* pRx = pService->createCharacteristic(UART_RX,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRx->setCallbacks(new MyCallbacks(CH_UART));  // Track: UART channel
  pTxCharacteristic = pService->createCharacteristic(UART_TX, BLECharacteristic::PROPERTY_NOTIFY);
  BLE2902* p2902 = new BLE2902();
  p2902->setIndications(true);
  pTxCharacteristic->addDescriptor(p2902);
  pService->start();

  // ── Ninebot Custom Service (REQUIRED for Encryption2 / app handshake!) ──
  BLEService* pNBSvc = pServer->createService(NBSVC);
  pNBWrite = pNBSvc->createCharacteristic(NB_RX,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pNBWrite->setCallbacks(new MyCallbacks(CH_NB));  // Track: Ninebot channel
  pNBCharacteristic = pNBSvc->createCharacteristic(NB_TX, BLECharacteristic::PROPERTY_NOTIFY);
  BLE2902* pNB2902 = new BLE2902();
  pNB2902->setIndications(true);
  pNBCharacteristic->addDescriptor(pNB2902);
  pNBSvc->start();
  Serial.println("[SVC] Ninebot Custom Service started (Encryption2 ready)");

  // ── FE95 Service (manufacturer data override) ──
/*
  BLEService* pS2 = pServer->createService(SERVICE2_UUID);
  BLECharacteristic* pS2C = pS2->createCharacteristic("0014",
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pS2C->setValue(MANUFDAT);
  pS2C->setCallbacks(new tweakCB());
  pS2->start();
*/

  // ── Advertising ──
  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x0);
  
  Serial.print("[ADV] Manufacturer data: ");
  for(int i=0;i<8;i++){if((uint8_t)MANUFDAT[i]<16)Serial.print("0");Serial.print((uint8_t)MANUFDAT[i],HEX);Serial.print(" ");}
  Serial.println();
  
  String manStr(MANUFDAT, 8);
  BLEAdvertisementData advData;
  advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advData.setName(DEVICE_SERIAL);
  advData.setManufacturerData(manStr);
  pAdv->setAdvertisementData(advData);

  BLEAdvertisementData scanData;
  scanData.setCompleteServices(BLEUUID(UART));
  pAdv->setScanResponseData(scanData);
  
  BLEDevice::startAdvertising();
  Serial.println("[BLE] Ready...");
  Serial.println("[BLE] Services: UART (Nordic) + Ninebot Custom (Encryption2) + FE95");
}

void loop() {
  if(deviceConnected && !oldDeviceConnected){
    oldDeviceConnected=deviceConnected;
    Serial.println("[SYS] Connected!");
  }
  if(!deviceConnected && oldDeviceConnected){
    oldDeviceConnected=deviceConnected;
    Serial.println("[SYS] Disconnected, re-advertising...");
    delay(500);
    BLEDevice::startAdvertising();
  }
  static unsigned long lastStat=0;
  if(millis()-lastStat>10000){
    lastStat=millis();
    Serial.printf("[STAT] C:%s A:%s RX:%lu TX:%lu CRC:%lu B:%d Ctr:%d Enc:%s Ch:%s\n",
      deviceConnected?"Y":"N",authDone?"Y":"N",rxCount,txCount,badCrc,rxBufLen,crypto_counter,
      appSendingEncrypted?"Y":"N",
      activeChannel==CH_NB?"NB":"UART");
  }
  delay(1);
}
