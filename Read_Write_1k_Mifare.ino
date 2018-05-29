/*
   Initial Author: ryand1011 (https://github.com/ryand1011)
   
   Reads data written by a program such as "rfid_write_personal_data.ino"

   See: https://github.com/miguelbalboa/rfid/tree/master/examples/rfid_write_personal_data

   Uses MIFARE RFID card using RFID-RC522 reader
   Uses MFRC522 - Library
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15

   SPI SS is pin 4 in our case
   Sometimes during the read/write process, you need to press 
   another key to continue the code after you enter a number.
   Press the enter key, then on the next read you will get
   an invalid response. This bug can be ignored and try again
   or reload the board.

   If modifying this code, never write to the last block of
   each sector. This holds the key and overwriting it will
   brick the chip. 

   Always hold the chip against the reader during use
*/

#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = 9;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 4;     // Configurable, see typical pin layout above

boolean enableHex = false;
byte block;
byte len;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

//*****************************************************************************************//
void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card

  Serial.println(F("Read or write personal data on a MIFARE PICC"));    //shows in serial that it is ready to read
  Serial.println(F("Keep the nfc tag to the reader and chose an option:"));
}

//*****************************************************************************************//
void loop() {

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  MFRC522::StatusCode status;

  //-------------------------------------------

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("\r\nA)Read\r\nB)Write\r\nH)Toggle hex block data view\r\n\nChoose an option:"));
  while (!Serial.available()) ; //Wait for user to type a character
  char command = Serial.read();

  //read function
  if (command == 'a' || command == 'A')
  {
    Serial.println();

    //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

    if (enableHex)mfrc522.PICC_DumpToSerial(&(mfrc522.uid));     //see all blocks in hex
    
    Serial.print(F("How many sectors do you want to read(1-9): "));
    while (!Serial.available()) ; //Wait for user to type a character
    int sectorRead = Serial.parseInt();

    Serial.println(sectorRead);

    Serial.print(F("Starting at sector(1-9): "));
    while (!Serial.available()) ; //Wait for user to type a character
    int j = Serial.parseInt();

    Serial.println(j);  
    Serial.print(F("#")); //print this first for android studio
    j-=1;
    sectorRead+=j;
    
    for (j; j < sectorRead; j++)
    {
      block = 1 + (j * 4);
      len = 18;
      byte buffer1[len];

      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }

      status = mfrc522.MIFARE_Read(block, buffer1, &len);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }

      //PRINT
      for (uint8_t i = 0; i < 16; i++)
      {
        if (buffer1[i] != 32)
        {
          Serial.write(buffer1[i]);
        }
      }
      Serial.print(" ");
    }
    
    Serial.println(F("~")); //end of string in android
    Serial.println();
  }

  //write function
  else if (command == 'b' || command == 'B')
  {
    Serial.print(F("Card UID:"));    //Dump UID
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.print(F(" PICC type: "));   // Dump PICC type
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    Serial.print(F("How many sectors do you want to write(1-9): "));
    while (!Serial.available()) ; //Wait for user to type a character
    int sector = Serial.parseInt();
    
    Serial.println(sector);

    Serial.print(F("Starting at sector(1-9): "));
    while (!Serial.available()) ; //Wait for user to type a character
    int j = Serial.parseInt();

    Serial.println(j);

    byte buffer[17 * sector];
    MFRC522::StatusCode status;

    j-=1;
    sector+=j;
    Serial.setTimeout(20000L) ;     // wait until 20 seconds for input from serial

    for (j; j < sector; j++)
    {
      Serial.print(F("Enter Sector "));
      Serial.print(j + 1);
      Serial.println(F(", ending with *"));

      len = Serial.readBytesUntil('*', (char *) buffer, 30) ; // read family name from serial
      for (byte i = len; i < 30; i++) buffer[i] = ' ';     // pad with spaces

      block = 1 + (j * 4);
      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      else Serial.println(F("PCD_Authenticate() success: "));

      // Write block
      status = mfrc522.MIFARE_Write(block, buffer, 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      else Serial.println(F("MIFARE_Write() success: "));

      block = 2 + (j * 4);
      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }

      // Write block
      status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      else Serial.println(F("MIFARE_Write() success: "));

      Serial.println(" ");
    }
  }
  else if (command == 'h' || command == 'H')
  {
    if (!enableHex)
    {
      Serial.println("Hex enabled on next read\r\n");
      enableHex = true;
    }
    else
    {
      Serial.println("Hex Disabled\r\n");
      enableHex = false;
    }
  }
  else
    Serial.println(F("Invalid\r\n"));

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  Serial.flush();

  delay(3000);
  Serial.println(F("Read or write personal data on a MIFARE PICC"));
  Serial.println(F("Keep the nfc tag to the reader and chose an option:"));
}
//*****************************************************************************************//
