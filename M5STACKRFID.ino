//https://github.com/miguelbalboa/rfid/blob/master/examples/MifareClassicValueBlock/MifareClassicValueBlock.ino
//#include <Wire.h>
#include "MFRC522_I2C.h"
#include "states.h"
#include <M5Stack.h>
#include <Adafruit_NeoPixel.h>

bool display_serial = true;
bool audio_feedback = false;//use audio to let us know if a card backuped
bool neopixel_feedback = false;//use neopixels to let us know if a card backuped

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

MFRC522::MIFARE_Key key;//we copy the keys from below into this one when we actually try to authenticate

//int num_keys = 4;
#define num_keys 115
//default keys (ordered by frequency seen so far)
byte default_key[num_keys][6]= {
                                {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
                                {0x8A,0x19,0xD4,0x0C,0xF2,0xB5},
                                {0x2A,0x2C,0x13,0xCC,0x24,0x2A},
                                {0x10,0xC5,0x29,0x4D,0x73,0xF8},
                                {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},
                                {0x9E,0x9F,0x2B,0xBB,0x43,0x30},
                                {0xA9,0x70,0xB4,0xE0,0xBE,0xB0},
                                {0xA7,0xA4,0x17,0x97,0x3C,0x68},
                                {0x30,0x8A,0x3C,0xB1,0x73,0xEB},
                                {0xA7,0xA0,0x12,0x21,0xEF,0x68},
                                {0xB8,0x5A,0xE4,0x0A,0x51,0x78},
                                {0x22,0xB4,0xFE,0x28,0x2F,0x9B},
                                {0x9F,0x2F,0xB5,0xF7,0xEA,0x30},
                                {0xE6,0x60,0x66,0x6A,0x5D,0xE9},
                                {0x48,0x7D,0x20,0x9B,0xC0,0xE0},
                                {0xA1,0xE5,0xF0,0x0B,0x42,0xAF},
                                {0x76,0xBF,0x1F,0xEB,0x73,0x5B},
                                {0xB8,0x3A,0xFA,0x11,0x41,0x78},
                                {0x8E,0xB8,0x61,0x4C,0x12,0x20},
                                {0x43,0x1B,0x90,0x75,0xA8,0x87},
                                {0xF6,0xD2,0x32,0xB0,0x0A,0xF9},
                                {0xD1,0xC7,0xBD,0x08,0x04,0x6E},
                                {0x22,0xD5,0xB1,0x21,0x2F,0x9B},
                                {0xD2,0x36,0x33,0x45,0xDB,0x6E},
                                {0x43,0xAA,0xDA,0x77,0xA8,0x87},
                                {0xB8,0x8B,0x25,0xC0,0x96,0x78},
                                {0xF6,0x8B,0x73,0xF9,0x1D,0xF9},
                                {0x33,0xA5,0x30,0x5F,0x1F,0x77},
                                {0xB1,0xD1,0x32,0xDA,0xC7,0xBF},
                                {0x30,0xEA,0x9F,0xE8,0x3F,0xEB},
                                {0xE2,0x05,0x28,0x25,0x14,0xAA},
                                {0x20,0xB0,0x75,0xB1,0xB5,0xDF},
                                {0x21,0x5E,0x0C,0x3B,0x4A,0xDF},
                                {0x10,0xD5,0x00,0x6B,0x61,0xF8},
                                {0xD1,0xD2,0x26,0xB6,0xAB,0x9A},
                                {0x13,0x59,0x60,0xD3,0x41,0x8B},
                                {0xE2,0x07,0xEA,0xFC,0x04,0xAA},
                                {0x9F,0x32,0xB4,0x18,0x98,0x30},
                                {0xE2,0x26,0xE5,0xD3,0x3F,0xAA},
                                {0x76,0xAB,0xEA,0x9F,0x0C,0x5B},
                                {0x01,0x3A,0x33,0x75,0xF7,0xE8},
                                {0x38,0x5D,0x5B,0xA8,0xBE,0xD0},
                                {0xB8,0x8B,0x2E,0x0B,0x4C,0x78},
                                {0x20,0x96,0x68,0x43,0xC4,0xDF},
                                {0x31,0x1A,0x21,0xBC,0x73,0xEB},
                                {0xA9,0x00,0x22,0xEB,0xBE,0xB0},
                                {0x76,0x1B,0xDB,0x9F,0x0C,0x5B},
                                {0xA8,0x70,0x46,0x15,0xF4,0x68},
                                {0x13,0x25,0x3B,0x76,0xE0,0x8B},
                                {0x66,0xC2,0xA1,0x51,0xE7,0x4B},
                                {0x22,0xD5,0xA3,0x24,0x2D,0x9B},
                                {0xB7,0xBB,0x38,0x0B,0x4C,0x78},
                                {0xA9,0x1F,0xFC,0x8A,0x27,0xB0},
                                {0x48,0x7D,0x4B,0x97,0xC0,0xE0},
                                {0xA7,0xBA,0xBA,0x59,0x09,0x68},
                                {0x6B,0x3B,0x9B,0xDF,0x0E,0x15},
                                {0x30,0xC4,0xB2,0xA3,0xD9,0xEB},
                                {0xB8,0x7A,0xF5,0xCA,0x94,0x78},
                                {0x48,0x1D,0x3F,0x9A,0xCE,0xE0},
                                {0x58,0x65,0xCD,0x9D,0x40,0xF0},
                                {0x12,0xD3,0x0D,0x58,0x92,0x56},
                                {0x6B,0x52,0x24,0x61,0x17,0x15},
                                {0x7B,0x5D,0x9B,0x61,0x64,0x25},
                                {0x9E,0xB8,0xE3,0x90,0xD1,0x30},
                                {0xA8,0x01,0x2F,0x72,0x8C,0x68},
                                {0x7A,0xDD,0x98,0x61,0x64,0x25},
                                {0xC1,0xCB,0xA4,0xA1,0x39,0x5E},
                                {0x7B,0x3D,0xD8,0x88,0x41,0x25},
                                {0x6B,0xC1,0xF0,0x04,0x84,0x15},
                                {0xA1,0xD6,0x78,0x45,0xF1,0xAF},
                                {0xF6,0x71,0xEF,0xB2,0x0A,0xF9},
                                {0x38,0x2E,0x0D,0xB2,0xBF,0xD0},
                                {0x21,0x14,0xF5,0x1F,0x05,0xDF},
                                {0x00,0xDE,0x1C,0xAC,0x8B,0xE8},
                                {0x7B,0x5D,0xC3,0x27,0xA3,0x25},
                                {0x23,0xBA,0x26,0x97,0xE2,0x66},
                                {0x22,0xF7,0x0C,0xE6,0xBC,0x66},
                                {0xE2,0xA8,0x33,0xAF,0x53,0xAA},
                                {0x6B,0x13,0x02,0x72,0x02,0x15},
                                {0x9E,0xA1,0x44,0xB8,0x21,0x30},
                                {0xA1,0xB3,0xE1,0x8C,0xE1,0xAF},
                                {0x9E,0xCE,0xF0,0xBB,0x43,0x30},
                                {0x7B,0xCD,0x1D,0x12,0xAF,0x25},
                                {0x13,0x61,0x1D,0x25,0x71,0x8B},
                                {0x9F,0x2B,0xF7,0xF2,0x42,0x30},
                                {0x48,0x73,0x43,0x89,0xED,0xC3},
                                {0x17,0x19,0x37,0x09,0xAD,0xF4},
                                {0x1A,0xCC,0x31,0x89,0x57,0x8C},
                                {0xC2,0xB7,0xEC,0x7D,0x4E,0xB1},
                                {0x36,0x9A,0x46,0x63,0xAC,0xD2},
                                {0xEE,0xB4,0x20,0x20,0x9D,0x0C},
                                {0x91,0x1E,0x52,0xFD,0x7C,0xE4},
                                {0x75,0x2F,0xBB,0x5B,0x7B,0x45},
                                {0x66,0xB0,0x3A,0xCA,0x6E,0xE9},
                                {0x0D,0x25,0x8F,0xE9,0x02,0x96},
                                {0x4D,0x57,0x41,0x4C,0x56,0x48},
                                {0x4D,0x48,0x41,0x4C,0x56,0x48},
                                {0x79,0x5F,0xD7,0x25,0x1D,0xC7},
                                {0x2C,0x7E,0x15,0x35,0x90,0x7F},
                                {0xA1,0xF0,0x26,0xD4,0x5F,0x92},
                                {0x3F,0x82,0xD2,0x9A,0x18,0xC1},
                                {0xB2,0x20,0xBA,0xFC,0x9E,0xBF},
                                {0xF9,0x7D,0x37,0x0E,0x76,0xA5},
                                {0xF7,0xA8,0x7D,0xDD,0x5D,0x29},
                                {0x19,0x88,0xBF,0xEF,0xF8,0xB1},
                                {0x96,0xA3,0x01,0xBC,0xE2,0x67},
                                {0x00,0x00,0x01,0x4B,0x5C,0x31},
                                {0x68,0x01,0xFC,0x51,0x3E,0x1D},
                                {0x8A,0x00,0xA2,0x0F,0x96,0x02},
                                {0xB5,0x78,0xF3,0x8A,0x5C,0x61},
                                {0xDF,0xCC,0xBD,0xA0,0x7C,0xD8},
                                {0xF1,0x58,0x9A,0x61,0x31,0xF8},
                                {0x5D,0x7A,0xDF,0xE4,0x64,0x8E},
                                {0x36,0x23,0x90,0x53,0x3F,0xE3},
                                {0x76,0xE2,0x2E,0xAF,0x02,0x2A}
                        };

byte sector_blocks[16] = {3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63};

//byte full_card[1024];//1k mifare
byte full_card[64][16];
//int full_card_ctr = 0;
bool have_sd_card = false;
int statemachine = DEFAULTLOOP;

void turn_off_leds()
{
  for(byte i=0;i<num_pixels;i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void turn_on_led(int num, int r, int g, int b)
{
  if(num > num_pixels)
    num = num_pixels;
  pixels.setPixelColor(num, pixels.Color(r, g, b));
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

void setup() {

  M5.Lcd.begin();
  if(display_serial)
  Serial.begin(115200);           // Initialize serial communications with the PC
  Wire.begin();                   // Initialize I2C
  M5.begin();
  reset_screen();

  mfrc522.PCD_Init();             // Init MFRC522

  //if(neopixel_feedback)
  //{
    pixels.begin();
    turn_off_leds();
    pixels.show(); // Initialize all pixels to 'off'
  //}

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

void happy_sound()
{
  //freq,time(ms)
  M5.Speaker.tone(200, 200);
  M5.Speaker.tone(300, 200);
}

void sad_sound()
{
  //freq,time(ms)
  M5.Speaker.tone(200, 200);
  M5.Speaker.tone(100, 200);
}

void loop()
{

  //always check to see if the middle button was pushed, which will reset things
  if(M5.BtnB.wasPressed())
  {
    reset_screen();
    mfrc522.PCD_Init();
    statemachine = DEFAULTLOOP;
  }
  switch (statemachine)
  {
      case CARDFOUND://we have a card
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
            statemachine = DEFAULTLOOP;
            break;
        }

        if(piccType == MFRC522::PICC_TYPE_MIFARE_MINI)
          M5.Lcd.println("MIFARE Mini");
        else if(piccType == MFRC522::PICC_TYPE_MIFARE_1K)
          M5.Lcd.println("MIFARE 1K");
        else if(piccType == MFRC522::PICC_TYPE_MIFARE_4K)//currently not really supported
          M5.Lcd.println("MIFARE 4K");
          statemachine = PARSECARD;

        break;
      }
      case PARSECARD:
      {
        if(parse_card())
        {
          statemachine = SAVECARDDRAW;
          if(audio_feedback)
            happy_sound();
        }
        else
        {
          if(audio_feedback)
            sad_sound();
          if(neopixel_feedback)
          {
            for(int i=0;i<num_pixels;i++)
              turn_on_led(i, 255, 0, 0);//num,r,g,b
          }
          M5.Lcd.println("Error Reading Card!");
          delay(1000);
          statemachine = RESET;
        }
        if(neopixel_feedback)
          turn_off_leds();

        break;
      }
      case SAVECARDDRAW:
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
        statemachine = SAVECARD;
        break;
      }
      case SAVECARD:
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
          {
            M5.Lcd.printf("File saved %s.mfd\n",tmp_uid);
            if(audio_feedback)
              happy_sound();
            if(neopixel_feedback)
            {
              for(int i=0;i<num_pixels;i++)
                turn_on_led(i, 0, 255, 0);//num,r,g,b
            }
          }
          else
          {
            M5.Lcd.println("File not saved");
            if(audio_feedback)
              sad_sound();
            if(neopixel_feedback)
            {
              for(int i=0;i<num_pixels;i++)
                turn_on_led(i, 255, 0, 0);//num,r,g,b
            }
          }
          delay(1000);  
          statemachine = RESET;
          if(neopixel_feedback)
            turn_off_leds();
        }
        else if(M5.BtnC.wasPressed())
        {
          statemachine = RESET;
        }
        break;
      }
      case RESET:
      {
        //reset
        reset_screen();
        mfrc522.PCD_Init();
        statemachine = DEFAULTLOOP;
      }
      default:
      {
        // Look for new cards, and select one if present
        if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
        {
          delay(50);
        }
        else
          statemachine = CARDFOUND;
        break;
      }
  }
  m5.update();

}

bool parse_card()
{
// In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
//    byte trailerBlock = 0;
    //MFRC522::StatusCode 
    byte status;
    //byte buffer[18];
    //byte size = sizeof(buffer);
    //int32_t value;

    byte valid_a_key[6];memset(valid_a_key,0x00,6);
    byte valid_b_key[6];memset(valid_b_key,0x00,6);

    bool try_prev_valid = false;

    for(byte sector=0;sector < 16;sector++)
    {
        bool key_a_found = false;
        bool key_b_found = false;

        for(int k=0;k<num_keys;k++)
        {
          //copy over key
          if(try_prev_valid)
          {
            if(display_serial)
              Serial.printf("try previous\n");
            memcpy(key.keyByte,valid_a_key,6);
          }
          else
            memcpy(key.keyByte,default_key[k],6);
          /**
          for (byte i = 0; i < 6; i++)
          {
            key.keyByte[i] = default_key[k][i];
          }
          /**/
          // Authenticate using key A
          if(display_serial)
          {
            Serial.printf("key:%d %02X%02X%02X%02X%02X%02X\n",k,key.keyByte[0],key.keyByte[1],key.keyByte[2],key.keyByte[3],key.keyByte[4],key.keyByte[5]);
            Serial.printf("sector:%d block:%d Authenticating using key A %d...",sector,sector_blocks[sector],k);
            Serial.println();
          }
          status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, sector_blocks[sector], &key, &(mfrc522.uid));
          if(display_serial)
            Serial.printf("status:%d\n",status);
          if (status == MFRC522::STATUS_OK)
          {
            //valid key a
            for (byte i = 0; i < 6; i++)
            {
              valid_a_key[i] = key.keyByte[i];
            }
            key_a_found = true;
            try_prev_valid = true;
            break;
          }
          else
          {
            if (status == MFRC522::STATUS_ERROR || status == MFRC522::STATUS_TIMEOUT)
            {
              //reset?
              mfrc522.PCD_Init();
            }
            if(try_prev_valid)
            {
              try_prev_valid = false;
              k=-1;
            }
            if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
            {delay(5);}
          }
        }//key a loop
/**
        mfrc522.PCD_StopCrypto1();
        if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
        {delay(5);}
        //key b loop
        for(int k=0;k<num_keys;k++)
        {
            //copy over key
            for (byte i = 0; i < 6; i++)
            {
              key.keyByte[i] = default_key[k][i];
            }
            // Authenticate using key B
            if(display_serial)
            {
              Serial.printf("key:%d %02X%02X%02X%02X%02X%02X\n",k,default_key[k][0],default_key[k][1],default_key[k][2],default_key[k][3],default_key[k][4],default_key[k][5]);
              Serial.printf("sector:%d block:%d Authenticating using key B %d...",sector,sector_blocks[sector],k);
              Serial.println();
            }
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, sector_blocks[sector], &key, &(mfrc522.uid));
            if(display_serial)
              Serial.printf("status:%d\n",status);
            if (status == MFRC522::STATUS_OK)
            {
              //valid key b
              for (byte i = 0; i < 6; i++)
              {
                valid_b_key[i] = key.keyByte[i];
              }
              key_b_found = true;
              break;
            }
            else
            {
              if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
              {delay(5);}
            }
        }//key b loop
**/

        if(key_a_found == false)
        {
          if(display_serial)
            Serial.println(F("Did not find key"));
          return false;
        }

        //sector complete
        //M5.Lcd.printf("%d ",sector);
        M5.Lcd.printf(".");
        if(neopixel_feedback)
          turn_on_led(sector, 0, 255, 0);//num,r,g,b

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
        **/
        /**
        if(display_serial)
          Serial.println(F("PCD_StopCrypto1"));
        mfrc522.PCD_StopCrypto1();
        if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
        {delay(5);}
        **/
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

bool check_magic()
{
  return mfrc522.MIFARE_OpenUidBackdoor(true);//MIFARE_CheckBackdoor();
}

bool clone_card(byte * uid, byte uidSize)
{
  //check to see if we have a magic card

  if(check_magic)
  {
    if(mfrc522.MIFARE_OpenUidBackdoor(true))
    {
      //change the uid
      if(mfrc522.MIFARE_SetUid(uid, uidSize, true))
      {
        //now actually write
/**
byte MFRC522::MIFARE_Write(     byte blockAddr, ///< MIFARE Classic: The block (0-0xff) number. MIFARE Ultralight: The page (2-15) to write to.
                                                        byte *buffer,   ///< The 16 bytes to write to the PICC
                                                        byte bufferSize ///< Buffer size, must be at least 16 bytes. Exactly 16 bytes are written.
                                                )
**/
      }
      else
        return false;
    }
    else
      return false; 
  }
  else
    return false;

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
