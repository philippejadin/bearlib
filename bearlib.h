/*
Stockage RFID :

Sector >  Block >  Bytes
16 secteurs de 4 blocs chacun (secteurs 0..15) > contenant 16 octets
Nous allons utiliser le secteur 1 bloc 4 5 6
et si nécessaire secteur 2 block 8 9 10

N'utiliser que ceux qui n'ont rien ci-dessus (ceux où tout est à 0)

block 4 = locale
Valeurs :
1 = fr
2 = nl
3 = en
4 = de

block 5, position 0 =  nourriture

*/


#include <SPI.h>
#include <MFRC522.h> // à installer
#include <avr/wdt.h> // inclue dans arduino

#define RST_PIN         9           // pin reset du lecteur rfid
#define SS_PIN          10          // pins slave select du lecteur rfid
#define LED_PIN         3           // led on pin 3

#define RELAY_1         4
#define RELAY_2         5
#define RELAY_3         6
#define RELAY_4         7

#define MOSFET_1        5
#define MOSFET_2        6




const byte LOCALE_FR = 1;
const byte LOCALE_NL = 2;
const byte LOCALE_EN = 3;
const byte LOCALE_DE = 4;

int LED_LOW = 10; // led semi allumée
int LED_MEDIUM = 20; // led medium power (100 = réel maximum)
int LED_HIGH = 80; // led full power (100 = réel maximum)


MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;



void bear_init()
{
  Serial.begin(9600);                                           // Initialize serial communications with the PC
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                              // Init MFRC522 card
  Serial.println(MODULE_NAME);    //affiche le nom du module en debug série


  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  // Cette clé est utilisée pour s'authentifier avec la carte mifare. Mais nous n'allons pas utiliser cette fonctionalité (ou plutôt : nous allons garder la clé d'origine pour ne pas compliqer)
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  wdt_enable(WDTO_8S); // active le watchdog pour rebooter l'arduino si pas de réponse après 8 secondes

  analogWrite(LED_PIN, LED_LOW);
}



boolean bear_has_card()
{
  return (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial());
}


byte bear_read(byte block, byte position)
{
  byte buffer[18];
  byte len = 18;

  // auth
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // read buffer
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  return buffer[position];
}


byte bear_read_block(byte block, byte *buffer)
{

  byte len = 18;

  // auth
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // read buffer
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  return true;
}


byte bear_write(byte block, byte position, byte value)
{
  byte buffer[18];
  byte len = 18;


  // auth
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }


  // read initial block values
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // alter buffer values
  buffer[position] = value;

  // Write block with new values
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }


  return true;
}

/*
Erase a whole block on the bear_has_card
*/
byte bear_erase_block(byte block)
{
  byte buffer[18];
  byte len = 18;


  // auth
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }


  // read initial block values
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // alter buffer values, set everything to 0
  for (byte i = 0; i < 17; i++) buffer[i] = 0;

  // Write block with new values
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }


  return true;
}


bear_stop()
{
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}


byte bear_get_locale()
{
  int locale = bear_read(4,0);

  if (locale > 4)
  {
    return false;
  }

  if (locale < 1) // send english if nothing found
  {
    return false;
  }
  
  return locale;
}


byte bear_set_locale(int locale)
{
  return bear_write(4,0, locale);
}

// TODO EFFACER LES DONNEES DE LA CARTE
void  bear_erase()
{
  bear_erase_block(4); // efface la nourriture
  bear_write(5, 0, 20); // efface la nourriture
  bear_erase_block(6); // efface les ressemblances

}

void bear_led_standby()
{
  analogWrite(LED_PIN, LED_LOW);
}

void bear_led_blink()
{
  wdt_reset();
  analogWrite(LED_PIN, LED_LOW);
  delay(100);
  analogWrite(LED_PIN, LED_HIGH);
  delay(100);
  analogWrite(LED_PIN, LED_LOW);
  delay(100);
  analogWrite(LED_PIN, LED_HIGH);
  delay(100);
  analogWrite(LED_PIN, LED_LOW);
  delay(100);
  analogWrite(LED_PIN, LED_HIGH);
  delay(100);
  analogWrite(LED_PIN, LED_LOW);
  wdt_reset();
}


void bear_led_blink_error()
{
  wdt_reset();
  analogWrite(LED_PIN, LED_LOW);
  delay(50);
  analogWrite(LED_PIN, LED_HIGH);
  delay(50);
  analogWrite(LED_PIN, LED_LOW);
  delay(50);
  analogWrite(LED_PIN, LED_HIGH);
  delay(50);
  analogWrite(LED_PIN, LED_LOW);
  delay(50);
  analogWrite(LED_PIN, LED_HIGH);
  delay(50);
  analogWrite(LED_PIN, LED_LOW);
  wdt_reset();
}


// fait un
bear_delay(long delay)
{
  long start = millis();

  while (millis() < (start + delay))
  {
    wdt_reset();
  }
}


unsigned long bear_playing_time = 0;

/*
Attends un point d'exclamation de la raspberry pour signaler que la lecture du fichier est terminée
Renvoi true si la vidéo est terminée, false si elle joue toujours
on peut définir un temps maximum (maxdelay) au delà duquel elle renvoi d'office false.
*/
byte bear_is_playing(long maxdelay)
{

  wdt_reset();


  if (bear_playing_time == 0)
  {
    bear_playing_time = millis();
  }

  if (millis() > (bear_playing_time + maxdelay))
  {
    bear_playing_time = 0;
    return false;
  }

  if (Serial.available() > 0)
  {
    // read the incoming byte:
    byte incomingByte = Serial.read();
    if (incomingByte == "!")
    {
      bear_playing_time = 0;
      return false;
    }
  }
  return true;
}
