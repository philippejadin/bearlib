/*
Stockage RFID :

Sector >  Block >  Bytes
16 secteurs de 4 blocs chacun (secteurs 0..15) > contenant 16 octets
Nous allons utiliser le secteur 1 bloc 4 5 6
et si nécessaire secteur 2 block 8 9 10

N'utiliser que ceux qui n'ont rien ci-dessus (ceux où tout est à 0)

block 4 = locale
Valeurs :
0 = fr
1 = nl
2 = en
3 = de
*/


#include <SPI.h>
#include <MFRC522.h> // à installer
#include <avr/wdt.h> // inclue dans arduino
#include <FadeLed.h> // à installer

#define RST_PIN         9           // pin reset du lecteur rfid
#define SS_PIN          10          // pins slave select du lecteur rfid
#define LED_PIN         3           // led on pin 3

#define RELAY_1         4
#define RELAY_2         5
#define RELAY_3         6
#define RELAY_4         7

#define MOSFET_1        5
#define MOSFET_2        6




const byte LOCALE_FR = 0;
const byte LOCALE_NL = 1;
const byte LOCALE_EN = 2;
const byte LOCALE_DE = 3;

int LED_LOW = 5; // led semi allumée
int LED_MEDIUM = 20; // led medium power (100 = réel maximum)
int LED_HIGH = 80; // led full power (100 = réel maximum)


MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
FadeLed led(LED_PIN); //Fading status LED on pin 3


void bear_init()
{
  Serial.begin(9600);                                           // Initialize serial communications with the PC
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                              // Init MFRC522 card
  Serial.println(MODULE_NAME);    //affiche le nom du module en debug série
  led.setTime(1000, true);

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  // Cette clé est utilisée pour s'authentifier avec la carte mifare. Mais nous n'allons pas utiliser cette fonctionalité (ou plutôt : nous allons garder la clé d'origine pour ne pas compliqer)
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  wdt_enable(WDTO_8S); // active le watchdog pour rebooter l'arduino si pas de réponse après 8 secondes
}



boolean bear_has_card()
{
  return (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial());
}




byte bear_get_locale()
{
  // on prépare des variables qui vont recevoir toutes les infos, dont un tableau, data, qui recevra les données du block sélectionné

  byte data[72];
  byte block = 4; // numéro du block, que l'on interroge, ici la locale
  byte len = sizeof(data);


  // on s'authentifie avec l'uid de la carte trouvé ci-dessus ainsi qu'avec la clé initialisée ci-dessus aussi
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }



  // là on lit les données
  len = sizeof(data);
  status = mfrc522.MIFARE_Read(block, data, &len);

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  // Cloturer au plus vite la lecture de la carte
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return data[0];
}


byte bear_set_locale(int locale)
{
  // on prépare des variables qui vont recevoir toutes les infos, dont un tableau, data, qui recevra les données du block sélectionné

  byte data[18];
  byte block = 4; // numéro du block, que l'on interroge, ici la locale
  byte len = sizeof(data);
  boolean result = true;

  data[0] = locale;

  // on s'authentifie avec l'uid de la carte trouvé ci-dessus ainsi qu'avec la clé initialisée ci-dessus aussi
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));

    delay(100);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return false;

    result = false;
  }


  // écriture locale
  status = mfrc522.MIFARE_Write(block, &data[16],  16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));

    delay(100);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return false;
    
    result = false;
  }

  // Cloturer au plus vite la lecture de la carte
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return result;
}

void bear_led_standby()
{
  FadeLed::update(); // gestion du fondu des leds, à appeller en boucle

  if (led.done())
  {
    if (led.get() == LED_LOW)
    {
      led.set(LED_MEDIUM);
    } else
    {
      led.set(LED_LOW);
    }
  }
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
