// special thanks https://codeberg.org/NootNooot for identifying the app registers!



const byte ECU_BLE = 0x04;
const byte ECU_BLE_POWER = 0x51;



const byte ECU_TFT = 0x23;

const byte CMD_READ = 0x01;
const byte CMD_WRITE = 0x02;
const byte CMD_WRITE_NR = 0x03;
const byte CMD_READ_RESP = 0x04;
const byte CMD_WRITE_RESP = 0x05;

const byte CMD_WR_BIT = 0x06;
const byte CMD_CLEARERROR = 0x06;

const byte CMD_IAP_BEGIN = 0x07;  // 4 bytes length of firmware, start frame of firmware download /with reply from scooter
const byte CMD_IAP_WR = 0x08;     // (integral multiple of 8) /with reply from scooter
const byte CMD_IAP_CRC = 0x09;    // 4 bytes download data checksum /with reply from scooter.
const byte CMD_IAP_RESET = 0x0A;  // Chip reset instructon /without reply from scooter.
const byte CMD_IAP_ACK = 0x0B;    // Response frame of firmware download.

const byte CMD_ACTIVATE = 0x57;
// Handshake
const byte CMD_PRE_COMM = 0x5B;
const byte CMD_SET_PWD = 0x5C;
const byte CMD_AUTH = 0x5D;

const byte CMD_AUDIO_BEGIN = 0x76;  // Start audio upload
const byte CMD_AUDIO_WR = 0x77;     // Write audio data chunk
const byte CMD_AUDIO_CRC = 0x78;    // Verify audio CRC


const byte VCU_SN = 0x10;
const byte VCU_CtrlV = 0x17;
const byte VCU_MCUV = 0x18;
const byte VCU_BmsV = 0x19;
const byte VCU_Bms2V = 0x1A;
const byte VCU_FunDisplayBool = 0x1B;
const byte VCU_Bool = 0x1C;
const byte VCU_FunBool = 0x1D;
const byte VCU_FunBool2 = 0x1E;
const byte VCU_FunBool3 = 0x1F;
const byte VCU_InfoBool2 = 0x1F;

const byte VCU_PN = 0x20;
const byte VCU_InstumentKey = 0x2E;
const byte VCU_BLE_HBPHASE = 0x2e;

const byte VCU_FunBool4 = 0x2F;
const byte VCU_FunDisplayBool2 = 0x30;
const byte VCU_FunDisplayBool3 = 0x31;
const byte VCU_FunDisplayBool4 = 0x32;
const byte VCU_FunDisplayBool5 = 0x33;

const byte VCU_ActDate = 0x40;
const byte VCU_StartSpeed = 0x42;
const byte VCU_GearEDMin = 0x43;	// ECO, DRIVE MIN 
const byte VCU_GearSRMin = 0x44;	// SPORT, RACE MIN
const byte VCU_GearEDMax = 0x45;  // MAX
const byte VCU_GearSRMax = 0x46;  
//const byte VCU_MaxSpeed = 0x46;
const byte VCU_GearED = 0x47;
const byte VCU_GearSR = 0x48;
const byte VCU_AutoOffTime = 0x49;
const byte VCU_CustomKey = 0x4A;
const byte VCU_ChargeStartTime = 0x4B;
//const byte VCU_PWRDIAG = 0x4b;	//?
const byte VCU_ChargeEndTime = 0x4C;
const byte VCU_Battery = 0x55;
const byte VCU_BATTPCT = 0x55;
const byte VCU_Speed = 0x57;	//VCU_THROTTLE 0x0-0x1b4?
const byte VCU_ErrorCode = 0x58;
const byte VCU_WarnCode = 0x59;
const byte VCU_GearMode = 0x5A;
const byte VCU_DRIVE_MODE = 0x5a;
const byte VCU_LedMode = 0x5B;
const byte VCU_ProjectionLightMode = 0x5C;
const byte VCU_TailLightMode = 0x5D;	// brake_flash
const byte VCU_PreciseMileage = 0x5E;
const byte VCU_LeftMileage = 0x5F;

const byte VCU_Pwd = 0x61;	// comboLock
const byte VCU_Mileage = 0x62;
const byte VCU_Runtime = 0x64;
const byte VCU_RideTime = 0x66;
const byte VCU_SingleMileage = 0x68;
const byte VCU_RunningTime = 0x69;
const byte VCU_SingleRideTime = 0x6A;
const byte VCU_BodyTemp = 0x6B;	// deg c*10
const byte VCU_TEMP = 0x6b;  // c*10
const byte VCU_SGear = 0x6E;

const byte VCU_DecMode = 0x70;	// VCU_KERS ?
const byte VCU_KeyPwd = 0x71;
//const byte VCU_PAT_LOCK = 0x71;
const byte VCU_AlarmLevel = 0x74;	// sensitivity

const byte VCU_BumpyRoad = 0x75;
const byte VCU_VoiceVolume = 0x76;
const byte VCU_playSound = 0x77;
const byte VCU_MaintainCode = 0x78;
const byte VCU_EGear = 0x79;
const byte VCU_DGear = 0x7A;

const byte VCU_MCUCPUId = 0xc0;	// = MCU_CPUId (02:0xda)
const byte VCU_MCUFlag = 0xc6;
const byte VCU_MCURand = 0xc7;
const byte VCU_LIGHT = 0xd2;
const byte VCU_CPUId = 0xDA;
const byte VCU_RAND = 0xe4;
const byte VCU_FLAG = 0xe7;
const byte VCU_EncryptionFlag = 0xE8;


const byte BMS_BatterySN = 0x02;
const byte BMS_ManufactureDateLT = 0x0A;
const byte BMS_Ver = 0x0E;

const byte BMS_SERIES_CELLS = 0x10;
const byte BMS_RATED_VOLTAGE = 0x11;  // *10
const byte BMS_0x12_0x02 = 0x12;  // # temps?, # parallel cells?

const byte BMS_Capacity = 0x13;

const byte BMS_CELL_THRESH_0 = 0x15;  //  3340        3474
const byte BMS_CELL_THRESH_1 = 0x16;  //  3470  1.04  3568  1.028
const byte BMS_CELL_THRESH_2 = 0x17;  //  3580  1.03  3615  1.013
const byte BMS_CELL_THRESH_3 = 0x18;  //  3650  1.02  3648  1.01
const byte BMS_CELL_THRESH_4 = 0x19;  //  3740  1.025 3698  1.014
const byte BMS_CELL_THRESH_5 = 0x1a;  //  3830  1.025 3794  1.026
const byte BMS_CELL_THRESH_6 = 0x1b;  //  3920  1.023 3885  1.024
const byte BMS_CELL_THRESH_7 = 0x1c;  //  4030  1.03  3978  1.024
const byte BMS_CELL_THRESH_8 = 0x1d;  //  4080  1.01  4080  1.026
const byte BMS_CELL_THRESH_9 = 0x1e;  //  4180  1.025 4150  1.017


const byte BMS_CycleCountLT = 0x59;
const byte BMS_MAH_FACTORY = 0x5a;
const byte BMS_MAH_AVAIL = 0x5b;
const byte BMS_RB = 0x5B;

const byte BMS_MaxPower = 0x82;	// ChargeLimit
const byte BMS_DeepDischargeCountLT = 0x89;
const byte BMS_RemainCapacityLT = 0x8A;
const byte BMS_MAH_FULLCAP = 0x8a;
const byte BMS_Volt = 0x8C;
const byte BMS_Cur = 0x8D;	// fullCap%
const byte BMS_SOC = 0x8F;	// curCharge%

const byte BMS_VOLTAGE = 0x8c;
const byte BMS_CURRENT = 0x8d;
const byte BMS_FULL_CAP_PCT = 0x8e;
const byte BMS_CHARGE_PCT = 0x8f;

const byte BMS_ChargeStatus = 0x92;
const byte BMS_TimeFull = 0x94;     // charge minutes remaining
//const byte BMS_CHARGE_TIME = 0x94;
const byte BMS_Temps = 0x96;
const byte BMS_CellVolts = 0xA0;

const byte BMS_CHARGER_CON = 0xc0;	// ?
const byte BMS_CHARGE_PCT_ALT = 0xce;	// instant charge value

const byte BMS_CapacityThroughputLT = 0xE1;
const byte BMS_EnergyThroughputLT = 0xE3;
const byte BMS_MoreInfoLT = 0xEA;
const byte BMS_ExtremeUseTimeLT = 0xF5;
const byte BMS_ExtremeChargeTimeLT = 0xF7;
const byte BMS_TEMP = 0xf9;

// rMCUPN 	16 (0x10)

const byte MCU_PN = 0x10;

const byte MCU_TEMP = 0x3e;
const byte MCU_TEMP_A_LASTMAX = 0x40;
const byte MCU_TEMP_B_LASTMAX = 0x41;
const byte MCU_TEMP_A = 0x48;   // deg C * 10
const byte MCU_TEMP_B = 0x49;

const byte MCU_MODE = 0x83;
const byte MCU_SPEED = 0x86;
const byte MCU_VOLTS = 0x8f;

const byte MCU_UNK_SIGNED_A = 0x91;
const byte MCU_UNK_SIGNED_B = 0x92;

const byte MCU_0xE1_0x06 = 0xe1;
const byte MCU_0xE4_0x06 = 0xe4;


const byte BLE_VERSION = 0x01;
const byte BLE_MAC_ADDR = 0x02;
const byte BLE_SETPOWER = 0x51;
const byte BLE_unk1 = 0x61;


const byte DIS_unk = 0x24;
const byte DIS_SHOWPAGE = 0x25;
const byte DIS_VCU_TIME = 0x3d;
const byte DIS_VCU_TURN = 0x60;
const byte DIS_VCU_WHY0 = 0x62;  // unk, always set 00 00?  (but also 0c.. 14.. 24..)
const byte DIS_VCU_WHY1 = 0x64;  // countdown
const byte DIS_unk2 = 0xd1;
const byte DIS_COMBPASS = 0xd3;
const byte DIS_VCU_MESG = 0xd5;


// 16bits
const unsigned long SUB_7e765289 = 0x7e765289;
const unsigned long SUB_MILEAGE = 0xddaa7a89;   // max 3 digits
const unsigned long SUB_TRIP = 0xac440ac5;      // 1 mile on GT3, .1 miles on G3/F3
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

//                        IAP_OK,
//                        IAPERROR_SIZE,
//                        IAPERROR_ERASE,
//                        IAPERROR_WRITEFLASH,
//                        IAPERROR_UNLOCK,
//                        IAPERROR_INDEX,
//                        IAPERROR_BUSY,
//                        IAPERROR_FORM,
//                        IAPERROR_CRC,
//                        IAPERROR_OTHER


///////////////////// old, myBLEv2 compatibility
const byte hb0[] = { 0x00, 0x23, 0x16, 0x01, 0xfc };
const byte xALS0[] = { 0x02, 0x23, 0x16, 0x03, 0xd2, 0x00, 0x00 };  // minic BLE ALS - tell VCU ambient light level is dark

const byte bSub[] = { 0xbb, 0x23, 0x16, 0x02, 0xfd, 0x01, 0x0a, 0x89, 0x52, 0x76, 0x7e, 0x10, 0x89, 0x7a, 0xaa, 0xdd,
                      0x10, 0xc5, 0x0a, 0x44, 0xac, 0x10, 0xb3, 0xd1, 0xdd, 0xa3, 0x10, 0xb2, 0xd1, 0xdd, 0xa3, 0x10,
                      0xad, 0xd1, 0xdd, 0xa3, 0x10, 0xac, 0xd1, 0xdd, 0xa3, 0x10, 0xaf, 0xd1, 0xdd, 0xa3, 0x10, 0x44,
                      0x6c, 0xe5, 0x6f, 0x10, 0xae, 0x48, 0x4a, 0xad, 0x10, 0x9e, 0xf9, 0x96, 0x90, 0x10, 0xe4, 0x65,
                      0x9b, 0x9c, 0x10, 0x92, 0x00, 0x9a, 0x22, 0x10, 0x64, 0xd0, 0xf6, 0xa4, 0x08, 0x12, 0x45, 0x6a,
                      0x61, 0x08, 0x70, 0xe0, 0xe2, 0xb3, 0x08, 0x8a, 0x38, 0x99, 0xe7, 0x08, 0xda, 0xb2, 0x89, 0x86,
                      0x04, 0xa1, 0x24, 0x01, 0x7e, 0x04, 0x4b, 0xcb, 0xf9, 0xf8, 0x04, 0x75, 0x1d, 0xbc, 0xa2, 0x04,
                      0x9f, 0x6f, 0x41, 0x42, 0x01, 0x98, 0x8d, 0xb2, 0x17, 0x01, 0x29, 0x0a, 0xbc, 0xcf, 0x01, 0xcc,
                      0xba, 0xe7, 0x88, 0x01, 0xe5, 0xe9, 0x99, 0x0c, 0x01, 0x61, 0xe9, 0x41, 0x18, 0x01, 0x8a, 0xa8,
                      0xb5, 0x16, 0x01, 0xb3, 0xd7, 0xef, 0x70, 0x01, 0x03, 0x26, 0x1a, 0x75, 0x01, 0x39, 0xb0, 0xbf,
                      0x06, 0x01, 0x3b, 0x26, 0xe8, 0x20, 0x01, 0x55, 0x25, 0x1a, 0x7f, 0x01, 0x2b, 0x71, 0x39, 0xf7,
                      0x01, 0x6b, 0x64, 0x4e, 0xac, 0x01, 0x19, 0xbd, 0x86, 0x00, 0x01, 0x1a, 0x93, 0x7d, 0x0c, 0x01 };
