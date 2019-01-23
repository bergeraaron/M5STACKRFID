#include <sd_defines.h>
#include <sd_diskio.h>
#include <SD.h>

//https://github.com/miguelbalboa/rfid/blob/master/examples/MifareClassicValueBlock/MifareClassicValueBlock.ino
#include <Wire.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>

// 0x28 is i2c address on SDA. Check your address with i2cscanner if not match.
MFRC522 mfrc522(0x28);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

byte LastUid[10];
byte CurrentUid[10];

int num_keys = 2;
//default keys
byte default_key[2][6]= {
                          {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
                          {0x8A,0x19,0xD4,0x0C,0xF2,0xB5}
                        };

byte sector_blocks[16] = {3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63};

byte full_card[1024];//1k mifare

void sd_card_setup(){
    Serial.begin(115200);
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
/**
    listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/", 2);
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    testFileIO(SD, "/test.txt");
/**/
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void setup() {
  
  memset(LastUid,0x00,10);
  memset(CurrentUid,0x00,10);
  memset(full_card,0x00,1024);

  M5.Lcd.begin();
  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor( WHITE );
  M5.Lcd.setTextSize(1);

  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("M5StackFire MFRC522");
  Serial.begin(115200);           // Initialize serial communications with the PC
  Wire.begin();                   // Initialize I2C
  M5.begin();
  mfrc522.PCD_Init();             // Init MFRC522

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++)
  {
    //key.keyByte[i] = 0xFF;
    key.keyByte[i] = default_key[1][i];
  }

  ShowReaderDetails();            // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
  M5.Lcd.println("Scan PICC to see UID, type, and data blocks...");
}

void loop() {
  // Look for new cards, and select one if present
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    delay(50);
    return;
  }

  // Show some details of the PICC (that is: the tag/card)
  //Serial.print(F("Card UID:"));
  //dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  //Serial.println();
  //Serial.print(F("PICC type: "));
  //MFRC522::PICC_Type
  byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  //Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // Check for compatibility
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("This sample only works with MIFARE Classic cards."));
      return;
  }

  // Now a card is selected. The UID and SAK is in mfrc522.uid.
  // Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //Serial.print(mfrc522.uid.uidByte[i], HEX);
    //Serial.println();
    CurrentUid[i] = mfrc522.uid.uidByte[i];
  }

  
  if(!compare_uid(CurrentUid,LastUid))
  {

    //copy the uid
    for(byte i=0;i<10;i++)
      LastUid[i]=CurrentUid[i];
      
    //new card
    M5.Lcd.println("New card");
    //then we need to do something with it, like try to read it and then maybe store it

    parse_card();
  }
}

bool compare_uid(byte * olduid, byte * newuid)
{
  for(byte i=0;i<10;i++)
  {
    //Serial.printf("old %02X new %02X \n",olduid[i],newuid[i]);
    if(olduid[i] != newuid[i])
      return false;
  }
  return true;
}

void parse_card()
{
// In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte trailerBlock = 0;
    //MFRC522::StatusCode 
    byte status;
    byte buffer[18];
    byte size = sizeof(buffer);
    int32_t value;

    for(byte sector=0;sector < 16;sector++)
    {
        for(int k=0;k<num_keys;k++)
        {
          //copy over key
          for (byte i = 0; i < 6; i++)
          {
            key.keyByte[i] = default_key[k][i];
          }
      
            // Authenticate using key A
            Serial.printf("sector:%d block:%d Authenticating using key A %d...",sector,sector_blocks[sector],k);
            Serial.println();
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, sector_blocks[sector], &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK) {
if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
  Serial.println(F("No card was previously selected, and none are available. Failed to set UID."));
  return;
}
            }
            else
              break;
            // Authenticate using key B
            Serial.printf("sector:%d block:%d Authenticating using key B %d...",sector,sector_blocks[sector],k);
            Serial.println();
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, sector_blocks[sector], &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK) {
                //Serial.print(F("PCD_Authenticate() failed: "));
                //Serial.println(mfrc522.GetStatusCodeName(status));
                //return;
            }
            else
              break;
        }
        //acturally read it out
        // Show the whole sector as it currently is
        Serial.println(F("Current data in sector:"));
        mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
        Serial.println();      
  
    }

/**

/**/
    Serial.println("Done Reading");
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}
/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
