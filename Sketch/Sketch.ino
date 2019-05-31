#include "SL44x2.h"

#define SC_C2_RST              7
#define SC_C1_VCC              11
#define SC_C7_IO               10
#define SC_C2_CLK              9

#define SC_SWITCH_CARD_PRESENT 8

#define SC_SWITCH_CARD_PRESENT_INVERT false

#define SC_PIN 0xFFFFFF

SL44x2 sl44x2(SC_C7_IO, SC_C2_RST, SC_C1_VCC, SC_SWITCH_CARD_PRESENT, SC_C2_CLK, SC_SWITCH_CARD_PRESENT_INVERT);

char name[20+1]; // Stocke le nom de l'enfant
char code[4+1];  // Stocke le code secret de l'enfant
char amount[3+1]; // Stocke le montant de l'argent de poche


char ack[2]; // Utilisé pour stocker la réponse de l'utilisateur à la confirmation de l'écriture


enum state {
  INSERT_CARD,
  WAIT_CARD,
  INIT_CARD,
  NAME,
  WAIT_NAME,
  CODE,
  WAIT_CODE,
  MONEY,
  WAIT_MONEY,
  ACK, 
  WAIT_ACK,
  WRITE_DATA,
  REMOVE_CARD,
  WAIT_CARD_REMOVE,
  IDLE
};

state STATE = NAME;
state nextState;
boolean hasFinished = false;

void setup()
{
  Serial.begin(9600);
  while (!Serial) {} // wait for serial port to connect. Needed for Leonardo only
                     // https://www.arduino.cc/en/Serial/IfSerial
}

void loop()
{
  switch(STATE) {
    case INSERT_CARD:
      Serial.println(F("Insérer la carte"));
      nextState = WAIT_CARD;
      break;

    case WAIT_CARD:
      if(sl44x2.cardInserted()) {
        STATE = INIT_CARD;
      }
      break;

    case INIT_CARD:
      if(sl44x2.cardReady()) {
        displayCardContent();
        STATE = NAME;
      } else {
        Serial.println(F("Carte non reconnue."));
        STATE = REMOVE_CARD;
      }
      break;

    case NAME:
    	Serial.print(F("Saisir le nom (20 car. max.) : "));
    	STATE = WAIT_NAME;
    	nextState = CODE;
    	hasFinished = false;
    	break;

    case WAIT_NAME:
    	waitUserEntry(name);
    	if(hasFinished) {
          Serial.println(name);
          STATE = nextState;
    	}
    	break;

    case CODE:
    	Serial.print(F("Saisir le code à 4 chiffres : "));
    	STATE = WAIT_CODE;
    	nextState = MONEY;
    	hasFinished = false;
    	break;

    case WAIT_CODE:
    	waitUserEntry(code);
      if(hasFinished) {
        Serial.println(code);
        STATE = nextState;
    	}
    	break;

    case MONEY:
    	Serial.print(F("Saisir le montant en centimes sur 3 car. : "));
    	STATE = WAIT_MONEY;
    	nextState = ACK;
    	hasFinished = false;
    	break;

    case WAIT_MONEY:
    	waitUserEntry(amount);
        if(hasFinished) {
          Serial.println(amount);
          STATE = nextState;
    	}
    	break;

    case ACK:
    	Serial.println(F("Les données suivantes vont être écrites : "));
    	Serial.print(F(" - Nom     : ")); Serial.println(name);
    	Serial.print(F(" - Code    : ")); Serial.println(code);
    	Serial.print(F(" - Montant : ")); Serial.println(amount);
    	Serial.println(F("Confirmer (O/N) : "));
    	STATE = WAIT_ACK;
    	break;

    case WAIT_ACK:
    	waitUserEntry(ack);
    	switch(ack[0]) {
        case 'O':
        case 'o':
        	Serial.println(F("Ecriture des données dans la carte."));
          STATE = WRITE_DATA;
        	break;
        case 'N':
        case 'n':
        	STATE = NAME;
        	break;
        default:
        	STATE = ACK;
        	break;
    	}
    	break;

    case WRITE_DATA:
      setNewData();
      STATE = REMOVE_CARD;
      break;

    case REMOVE_CARD:
      Serial.println(F("Retirer la carte."));
      STATE = WAIT_CARD_REMOVE;
      break;

    case WAIT_CARD_REMOVE:
      if(!sl44x2.cardInserted()) {
        Serial.println(F("Carte retirée"));
        sl44x2.cardRemoved();
        STATE = INSERT_CARD;
      }
      break;

    default:break;
  }
}

void waitUserEntry(char *buff) {
  
  while(Serial.available() <= 0){delay(10);}
  int index = 0;
  char inChar;
  while(Serial.available() > 0) {
    inChar = Serial.read();
    buff[index] = inChar;
    index++;
    buff[index] = '\0';
    delay(10); // nécessaire pour laisser le temps au Serial de faire ses traitements
  }
  
  hasFinished = true;
}

void setNewData() {

  if(sl44x2.authenticate(SC_PIN)) {
    boolean result = false;
    Serial.println(F("Authentifié ..."));

    // Update Main Memory (Unprotected Area 32 .. 255)
    uint8_t infos[4+3+20 + 1];
    strcpy(infos, code);
    strcat(infos, amount);
    strcat(infos, name);
      
    result = sl44x2.updateMainMemory(0x60, infos, 4+3+strlen(name) + 1); 
                                                      
    if (result) {
      Serial.println(F("Données mises à jour ..."));
    } else {
      Serial.println(F("Problème pendant la mise à jour ..."));
    }
  }  
}


void displayCardContent() {
  uint8_t  buff[SL44X2_DATA_SIZE];
  uint16_t i = sl44x2.readMainMemory(0, buff, SL44X2_DATA_SIZE);

  if (i > 0) {
    displayContent(buff, i);
  }

  Serial.print(F("Protection Bits : 0x"));
  Serial.println(sl44x2.readProtectionMemory(), HEX);

  uint32_t secMem = sl44x2.readSecurityMemory();

  uint8_t failedAuthentications = 0;
  for(uint8_t i=0; i < 3;i++) {
    if ((secMem && (1<<i)) == 0) {
      failedAuthentications++;
    }
  }

  Serial.print(F("Security Memory : 0x"));
  Serial.println(secMem, HEX);

  Serial.print(F("Failed authentications : "));
  Serial.println(failedAuthentications);
}

/**
 * C'est une copie de la fonction SL44x2::dumpHex().
 */
void displayContent(uint8_t* values, uint16_t size) {
  if (values != NULL && size > 0) {
    char ascii[17];
    for(uint16_t row=0; row<(size + 15)/16; row++) {
      // Print Adress
      if (row==0)
        Serial.print(F("0"));
      Serial.print(row * 16, HEX);
      Serial.print(F("|"));

      // Prefill ascii
      for(int i=0; i<16; i++)
        ascii[i] = '.';
      ascii[16] = (char)0x00;
      // colums
      for(uint16_t pos=row*16; pos<(row + 1) * 16; pos++ ) {
        if(pos < size) {
          if(values[pos] < 0x10)
            Serial.print(F("0"));
          Serial.print(values[pos], HEX);
          if(isPrintable(values[pos]))
            ascii[pos - row*16] = (char)values[pos];
        } else {
          Serial.print(F("  "));
        }
        Serial.print(F(" "));
      }
      Serial.print(F("'"));
      Serial.print(ascii);
      Serial.println(F("'"));
    }
  }
}