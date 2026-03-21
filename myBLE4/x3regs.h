static const byte CMD_READ = 0x01;
static const byte CMD_WRITE = 0x02;
static const byte CMD_WRITE_NR = 0x03;
static const byte CMD_READ_RSP = 0x04;
static const byte CMD_WRITEACK = 0x05;

// 16bits
const unsigned long SUB_7e765289 = 0x7e765289;
const unsigned long SUB_DISTZ = 0xddaa7a89;
const unsigned long SUB_TRIPDIST = 0xac440ac5;
const unsigned long SUB_a3ddd1b3 = 0xa3ddd1b3;
const unsigned long SUB_a3ddd1b2 = 0xa3ddd1b2;
const unsigned long SUB_a3ddd1ad = 0xa3ddd1ad;
const unsigned long SUB_a3ddd1ac = 0xa3ddd1ac;
const unsigned long SUB_a3ddd1af = 0xa3ddd1af;
const unsigned long SUB_SPEED = 0x6fe56c44;
const unsigned long SUB_ad4a48ae = 0xad4a48ae;
const unsigned long SUB_9096f99e = 0x9096f99e;
const unsigned long SUB_BOOST = 0x9c9b65e4;
const unsigned long SUB_229a0092 = 0x229a0092;

// 8bits
const unsigned long SUB_BATTERY = 0xa4f6d064;
const unsigned long SUB_ECOBATTERY = 0x616a4512;
const unsigned long SUB_b3e2e070 = 0xb3e2e070;
const unsigned long SUB_e799388a = 0xe799388a;

// 4bits
const unsigned long SUB_CHARGING = 0x8689b2da;
const unsigned long SUB_DRIVEMODE = 0x7e0124a1;
const unsigned long SUB_f8f9cb4b = 0xf8f9cb4b;
const unsigned long SUB_HEADLIGHT = 0xa2bc1d75;

// flags
const unsigned long SUB_2WD = 0x42416f9f;
const unsigned long SUB_SABS = 0x17b28d98;
const unsigned long SUB_SDTC_DIS = 0xcfbc0a29;
const unsigned long SUB_ODO_TRIP = 0x88e7bacc;
const unsigned long SUB_IMPERIAL = 0x0c99e9e5;
const unsigned long SUB_TCS_DIS = 0x1841e961;
const unsigned long SUB_PARKED = 0x16b5a88a;
const unsigned long SUB_RBS = 0x70efd7b3;
const unsigned long SUB_IMPACTW = 0x751a2603;
const unsigned long SUB_ANTIBUMP = 0x06bfb039;
const unsigned long SUB_DOWNASST = 0x20e8263b;
const unsigned long SUB_UPPUSH = 0x7f1a2555;
const unsigned long SUB_HILLPARK = 0xf739712b;
const unsigned long SUB_ac4e646b = 0xac4e646b;
const unsigned long SUB_CRUISEC = 0x0086bd19;
const unsigned long SUB_EXT_BAT = 0x0c7d931a;

const byte VCU_AUTO_OFF = 0x49;
const byte VCU_START_SPEED = 0x43;
const byte VCU_KERS = 0x70;
const byte VCU_CHARGE_LIMIT = 0x82;
const byte VCU_BRAKE_FLASH = 0x5d;
const byte VCU_ALARM_SENS = 0x74;

const byte VCU_TEMP = 0x6b;
const byte VCU_LIGHT = 0xd2;
const byte VCU_PWRDIAG = 0x4b;
const byte VCU_BATTPCT = 0x55;
const byte VCU_DRIVE_MODE = 0x5a;
const byte VCU_VERSION = 0x17;
const byte VCU_THROTTLE = 0x57;
const byte VCU_PAT_LOCK = 0x71;

const byte VCU_BLE_HBPHASE = 0x2e;
const byte VCU_ODO = 0x62;



const byte BMS_CHARGE_LIMIT = 0x82;
const byte BMS_VERSION = 0x0e;
const byte BMS_SERIES_CELLS = 0x10;
const byte BMS_RATED_VOLTAGE = 0x11;

const byte BMS_CHARGE_CYCLES = 0x59;
const byte BMS_MAH_FACTORY = 0x5a;
const byte BMS_MAH_FULLCAP = 0x8a;
const byte BMS_MAH_AVAIL = 0x5b;


const byte BMS_VOLTAGE = 0x8c;
const byte BMS_CURRENT = 0x8d;

const byte BMS_FULL_CAP_PCT = 0x8e;
const byte BMS_CHARGE_PCT = 0x8f;
const byte BMS_CHARGE_PCT_ALT = 0xce;

const byte BMS_CHARGE_TIME = 0x94;
const byte BMS_TEMP = 0xf9;
const byte BMS_CHARGER_CON = 0xc0;

const byte MCU_SPEED = 0x86;
const byte MCU_MODE = 0x83;
const byte MCU_VOLTS = 0x8f;

const byte MCU_TEMP = 0x3e;
const byte MCU_TEMP_A = 0x48;
const byte MCU_TEMP_B = 0x49;
const byte MCU_TEMP_A_LASTMAX = 0x40;
const byte MCU_TEMP_B_LASTMAX = 0x41;

const byte MCU_UNK_SIGNED_A = 0x91;
const byte MCU_UNK_SIGNED_B = 0x92;
