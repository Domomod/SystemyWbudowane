#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 8 // Pin 9 do resetowania RC522
#define SS_PIN 9 // Pin 10 dla SS (SDA) RC522
#define echoPin 5
#define trigPin 6
#define alarmPin 7
#define ledPin 10

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup(){
  Serial.begin(9600);
  Serial.println("N:Otworzono port szeregowy.");
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(alarmPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(alarmPin, LOW);
  SPI.begin(); //inicjacja magistrali SPI
  Serial.println("N:Inicjalizacja magistrali SPI");
  
  mfrc522.PCD_Init(); // inicjacja RC522
  Serial.println("N:Zainicjalizowano czytnik RFID");
}

int zmierzOdleglosc(){
  long czas, dystans;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  czas = pulseIn(echoPin, HIGH);
  dystans = czas/ 58;

  return dystans;
}

float measure(){
  int _max = -9000;
  for (int i =0; i<6; i++){
    int odczyt = zmierzOdleglosc();
    _max = max(_max, odczyt);
    delay(10); 
  }
  return _max;
}

enum EVENT{
  NONE,
  CARD_OR_PIN,

};

enum DOOR_STATE
{
  OPENED,
  CLOSED
};

enum ALARM_STATE{
  ENABLED,
  DISABLED,
  TRIGGERED,
  ENABLED_WAIT
};
  

DOOR_STATE door_state = CLOSED;
ALARM_STATE alarm_state = DISABLED;


int check_RFID(){
  // Look for new cards
  static boolean same_card = false;
  
  if(mfrc522.PICC_IsNewCardPresent()){
  succesfull_read:
     if(!same_card)
    {
      mfrc522.PICC_ReadCardSerial();
  
      String content= "";
      byte letter;
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
         content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
         content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      content.toUpperCase();

      if (content.substring(1) == "6B 72 F0 0D") //change here the UID of the card/cards that you want to give access
      {
        same_card = true;
        Serial.println("N:Wprowadzono poprawna karte");
        return CARD_OR_PIN;
      }
    }
    
  }
  else if(!mfrc522.PICC_IsNewCardPresent())
  { // Dwa następujące po sobie fałsze oznaczają brak karty przy czytniku.
    same_card = false;
  }
  else
  { //Pierwszy odczyt był fałszem ale drugi okazał się prawdą, karte nadal należy odczytać.
    goto succesfull_read;
  }
  return NONE;
}

int check_serial(){
  // Look for new cards
  if (Serial.available() > 0) {
    if(Serial.read() == 'p'){
       return CARD_OR_PIN;
    }
  }
  return NONE;
}


void check_Ultrasonic(){
  float m = measure();
  if(m > 15.0 && m < 1000.0)
  {
    if(door_state == CLOSED){Serial.println("W:Drzwi otwarto");}
    door_state = OPENED;
  }
  else
  {
    if(door_state == OPENED){Serial.println("W:Drzwi zamknieto");}
    door_state = CLOSED;
  }
}

String pin = "";
int timer = 0;

void reactToEvent(int event)
{
  if(alarm_state == ENABLED)
  {
    if(event == CARD_OR_PIN) { alarm_state = DISABLED; Serial.println("W:Rozbrojono alarm");}
    else if(door_state == OPENED){ alarm_state = TRIGGERED; Serial.println("W:Uruchomiono alarm");}
  }
  else if(alarm_state == DISABLED)
  {
    if(event == CARD_OR_PIN && door_state == CLOSED) { alarm_state = ENABLED; Serial.println("W:Uzbrojono alarm");}
    if(event == CARD_OR_PIN && door_state == OPENED) { alarm_state = ENABLED_WAIT; Serial.println("W:Uzbrojono alarm");}
  }
  else if(alarm_state == TRIGGERED)
  {
    if(event == CARD_OR_PIN) { alarm_state = DISABLED; Serial.println("W:Wylaczono alarm");}
  }
  else if(alarm_state == ENABLED_WAIT)
  {
    if(door_state == CLOSED){ alarm_state = ENABLED; }
    else if(event == CARD_OR_PIN) { alarm_state = DISABLED; Serial.println("W:Rozbrojono alarm");}
  }


  digitalWrite(alarmPin, alarm_state == TRIGGERED);
  digitalWrite(ledPin, alarm_state == DISABLED);
}

void loop()
{
  while (true)
  {
    check_Ultrasonic();
    reactToEvent((EVENT)check_RFID());
    reactToEvent((EVENT)check_serial());
  }
}
