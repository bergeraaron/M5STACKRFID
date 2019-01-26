//https://github.com/miguelbalboa/rfid/blob/master/examples/MifareClassicValueBlock/MifareClassicValueBlock.ino
#include <Wire.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>
#include <Adafruit_NeoPixel.h>

bool display_serial = false;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
#define PIN 15
#define num_pixels 10
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(num_pixels, PIN, NEO_GRB + NEO_KHZ800);

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
bool have_sd_card = false;
int statemachine = 0;

void turn_off_leds()
{
  for(byte i=0;i<num_pixels;i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

bool sd_card_setup()
{
  if(display_serial)
  Serial.begin(115200);
  if(!SD.begin()){
    if(display_serial)
    Serial.println("Card Mount Failed");
    return false;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    if(display_serial)
    Serial.println("No SD card attached");
    return false;
  }
  if(display_serial)
  {
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
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  if(display_serial)
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  return true;
}

bool writeFile(fs::FS &fs, const char * path, const unsigned char * message){
  if(display_serial)
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    if(display_serial)
    Serial.println("Failed to open file for writing");
    return false;
  }
  if(file.write(message,1024))
  {
    if(display_serial)
    Serial.println("File written");
    file.close();
    return true;
  }
  else
  {
    if(display_serial)
    Serial.println("Write failed");
    file.close();
    return false;
  }
}

bool save_file(unsigned char * buffer,char * uid)
{
  char file_name[64];memset(file_name,0x00,64);
  snprintf(file_name,64,"/%s.mfd",uid);
  bool status = writeFile(SD,file_name,buffer);
  return status;
}

void screentest()
{
	//for(byte i=0;i<300;i++)
	//{
    byte i = 400;
		M5.Lcd.setCursor(0,i);
		M5.Lcd.printf("i:%d",i);		
	//}
}

void setup() {

  M5.Lcd.begin();
  if(display_serial)
  Serial.begin(115200);           // Initialize serial communications with the PC
  Wire.begin();                   // Initialize I2C
  M5.begin();
  reset_screen();

  mfrc522.PCD_Init();             // Init MFRC522

  pixels.begin();
  turn_off_leds();
  pixels.show(); // Initialize all pixels to 'off'

  have_sd_card = sd_card_setup();

  ShowReaderDetails();            // Show details of PCD - MFRC522 Card Reader details
  if(display_serial)
  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
  if(!have_sd_card)
    M5.Lcd.println("SD Card Missing!");
}

void reset_screen()
{
    M5.Lcd.fillScreen( BLACK );
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor( WHITE );
    M5.Lcd.setTextSize(2);

    M5.Lcd.fillScreen( BLACK );
    M5.Lcd.setCursor(130, 210);
    M5.Lcd.print("Reset");
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Waiting for Card...");
}

void loop()
{
/**
  screentest();
  delay(100);
  return;
/**/
  //always check to see if the middle button was pushed, which will reset things
  if(M5.BtnB.wasPressed())
  {
    reset_screen();
    mfrc522.PCD_Init();
    statemachine = 0;
  }
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
        else if(piccType == MFRC522::PICC_TYPE_MIFARE_4K)//currently not really supported
          M5.Lcd.println("MIFARE 4K");
          statemachine = 2;

        break;
      }
      case 2:
      {
        if(parse_card())
          statemachine = 3;
        else
        {
          M5.Lcd.println("Error Reading Card!");
          delay(1000);
          statemachine = 5;
        }
        break;
      }
      case 3:
      {
        //look for user input
        M5.Lcd.println("Save to SD Card?");
        M5.Lcd.setCursor(50, 210);
        M5.Lcd.print("Yes");
        M5.Lcd.setCursor(130, 210);
        M5.Lcd.print("Reset");
        M5.Lcd.setCursor(240, 210);
        M5.Lcd.print("No");
        //need to draw the yes/no buttons
        statemachine = 4;
        break;
      }
      case 4:
      {
        //look for user input
        if(M5.BtnA.wasPressed())
        {
          //yes
          unsigned char tmp_buffer[1024];
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
          //clear the screen
          reset_screen();
          if(save_file(tmp_buffer,tmp_uid))
            M5.Lcd.printf("File saved %s.mfd\n",tmp_uid);
          else
            M5.Lcd.println("File not saved");
          delay(1000);  
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
        reset_screen();
        mfrc522.PCD_Init();
        statemachine = 0;
      }
      default:
      {
        // Look for new cards, and select one if present
        if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
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

bool parse_card()
{
// In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte trailerBlock = 0;
    //MFRC522::StatusCode 
    byte status;
    byte buffer[18];
    byte size = sizeof(buffer);
    int32_t value;

    byte valid_a_key[6];memset(valid_a_key,0x00,6);
    byte valid_b_key[6];memset(valid_b_key,0x00,6);

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
            if(display_serial)
            {
              Serial.printf("sector:%d block:%d Authenticating using key A %d...",sector,sector_blocks[sector],k);
              Serial.println();
            }
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, sector_blocks[sector], &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK) {
if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
  if(display_serial)
  Serial.println(F("No card was previously selected, and none are available. Failed to set UID."));
  return false;
}
            }
            else
            {
              //valid key a
              for (byte i = 0; i < 6; i++)
              {
                valid_a_key[i] = key.keyByte[i];
              }
              break;
            }
            // Authenticate using key B
            if(display_serial)
            {
              Serial.printf("sector:%d block:%d Authenticating using key B %d...",sector,sector_blocks[sector],k);
              Serial.println();
            }
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, sector_blocks[sector], &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK) {
if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
  if(display_serial)
  Serial.println(F("No card was previously selected, and none are available. Failed to set UID."));
  return false;
}
            }
            else
            {
              //valid key b
              for (byte i = 0; i < 6; i++)
              {
                valid_b_key[i] = key.keyByte[i];
              }
              break;
            }
        }
/**
        //acturally read it out
        // Show the whole sector as it currently is
        Serial.println(F("Current data in sector:"));
        mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
        Serial.println();      
/**/
        //sector complete
        //M5.Lcd.printf("%d ",sector);
        M5.Lcd.printf(".");
        
	      //dump to buffer
	      mfrc522.PICC_DumpMifareClassicSectorToBuffer(&(mfrc522.uid), &key, sector, full_card);

        //need to copy the keys to the right spot since the dump only uses one of them.
        //a 0-5
        //b 10-15
        //valid key a
        for (byte i = 0; i < 6; i++)
        {
          full_card[sector_blocks[sector]][i] = valid_a_key[i];
        }
        /**
        //valid key b
        for (byte i = 0; i < 6; i++)
        {
          full_card[sector_blocks[sector]][10+i] = valid_b_key[i];
        }
        /**/
    }

    if(display_serial)
    {
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
      Serial.println("Done Reading");
    }

    M5.Lcd.println("\nDone Reading");

    return true;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if(display_serial)
  {
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
}
