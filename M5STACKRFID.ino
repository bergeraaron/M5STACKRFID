#include <sd_defines.h>
#include <sd_diskio.h>
#include <SD.h>

//https://github.com/miguelbalboa/rfid/blob/master/examples/MifareClassicValueBlock/MifareClassicValueBlock.ino
#include <Wire.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>

#include <Adafruit_NeoPixel.h>

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
#define PIN 15
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

// 0x28 is i2c address on SDA. Check your address with i2cscanner if not match.
MFRC522 mfrc522(0x28);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

int num_keys = 2;
//default keys
byte default_key[2][6]= {
                          {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
                          {0x8A,0x19,0xD4,0x0C,0xF2,0xB5}
                        };

byte sector_blocks[16] = {3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63};

//byte full_card[1024];//1k mifare
byte full_card[64][16];
//int full_card_ctr = 0;

int statemachine = 0;

void sd_card_setup()
{
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

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void save_file(char * buffer,char * uid)
{
  char file_name[64];memset(file_name,0x00,64);
  snprintf(file_name,64,"/%s.mfd",uid);
  writeFile(SD,file_name,buffer);
}

void setup() {

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
  //turn off the neopixels
  pinMode(15,OUTPUT);
  digitalWrite(15,1);
    
  mfrc522.PCD_Init();             // Init MFRC522

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  sd_card_setup();

  ShowReaderDetails();            // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
  M5.Lcd.println("Waiting for Card...");
}

void loop()
{

  switch (statemachine)
  {
      case 1://we have a card
      {
        //print the UID
        for (byte i = 0; i < mfrc522.uid.size; i++)
          M5.Lcd.printf("%02X",mfrc522.uid.uidByte[i]);
        M5.Lcd.printf(" ");        

        //MFRC522::PICC_Type
        byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

        // Check for compatibility
        if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
            M5.Lcd.println(F("This sample only works with MIFARE Classic cards."));
            statemachine = 0;
            break;
        }

        if(piccType == MFRC522::PICC_TYPE_MIFARE_MINI)
          M5.Lcd.println("MIFARE Mini");
        else if(piccType == MFRC522::PICC_TYPE_MIFARE_1K)
          M5.Lcd.println("MIFARE 1K");
        else if(piccType == MFRC522::PICC_TYPE_MIFARE_4K)
          M5.Lcd.println("MIFARE 4K");
          statemachine = 2;

        break;
      }
      case 2:
      {
        parse_card();
        statemachine = 3;
        break;
      }
      case 3:
      {
        //look for user input
        M5.Lcd.println("Save to Card?");
        statemachine = 4;
        break;
      }
      case 4:
      {
        //look for user input
        if(M5.BtnA.wasPressed())
        {
          //yes
          char tmp_buffer[1024];
          short tmpctr=0;
          char tmp_uid[32];
          //for (byte i = 0; i < mfrc522.uid.size; i++)//not the safest way to do it...
            //sprintf(tmp_uid,"%s%02X",tmp_uid,mfrc522.uid.uidByte[i]);
          sprintf(tmp_uid,"%02X%02X%02X%02X",mfrc522.uid.uidByte[0],mfrc522.uid.uidByte[1],mfrc522.uid.uidByte[2],mfrc522.uid.uidByte[3]);
          for(int b=0;b<64;b++)
          {
              for(int d=0;d<16;d++)
              {
                tmp_buffer[tmpctr] = full_card[b][d];
                tmpctr++;
              }
          }
          save_file(tmp_buffer,tmp_uid);
          statemachine = 5;
        }
        else if(M5.BtnC.wasPressed())
        {
          statemachine = 5;
        }
        break;
      }
      case 5:
      {
        //reset
        M5.Lcd.println("Reset");
        mfrc522.PCD_Init();
        statemachine = 0;
      }
      default:
      {
        // Look for new cards, and select one if present
        if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() )
        {
          delay(50);
        }
        else
          statemachine = 1;
        break;
      }
  }
  m5.update();

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
if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
  Serial.println(F("No card was previously selected, and none are available. Failed to set UID."));
  return;
}
            }
            else
              break;
        }
/**
        //acturally read it out
        // Show the whole sector as it currently is
        Serial.println(F("Current data in sector:"));
        mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
        Serial.println();      
/**/
        //sector complete
        M5.Lcd.printf("%d ",sector);
        
	      //dump to buffer
	      mfrc522.PICC_DumpMifareClassicSectorToBuffer(&(mfrc522.uid), &key, sector, full_card);

    }

/**/
        int bctr=0;
        for(int s=0;s<16;s++)//16
        {
          for(int b=0;b<4;b++)
          {
              Serial.printf("Sector:%d Block:%d ",s,b);
              for(int d=0;d<16;d++)
              {
                Serial.printf("%02X ",full_card[bctr][d]);
              }
              Serial.printf("\n");
              bctr++;
          }
          Serial.printf("\n");
        }

/**/
    Serial.println("Done Reading");
    M5.Lcd.println("Done Reading");

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
