//Programa: Teste modulo PN532 NFC RFID Arduino
//Autor: Victhor
//Baseado no programa exemplo da biblioteca PN532 

//Define a interface de comunicação
//diretiva do pre-processador
#if 0
#include <SPI.h>  
#include <PN532_SPI.h>  //Interface SPI
#include "PN532.h"
#include <SoftwareSerial.h>
PN532_SPI pn532spi(SPI, 10);  //modulo de interface com PN532
PN532 nfc(pn532spi);

#elif 1
#include <PN532_HSU.h> //Interface HSU
#include <PN532.h> 
#include <SoftwareSerial.h> //
SoftwareSerial SWSerial(10, 11);  //RX, TX
PN532_HSU pn532swhsu( SWSerial ); //modulo de interface com PN532
PN532 nfc(pn532hsu);

#else
#include <Wire.h>
#include <PN532_I2C.h>  //Interface I2C
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532i2c(Wire); //modulo de interface com PN532
PN532 nfc(pn532i2c);
#endif

void setup(void)
{ 
  
}