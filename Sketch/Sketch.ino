#include "SL44x2.h"

#define SC_C2_RST              7
#define SC_C1_VCC              11
#define SC_C7_IO               10
#define SC_C2_CLK              9

#define SC_SWITCH_CARD_PRESENT 8

#define SC_SWITCH_CARD_PRESENT_INVERT false

#define SC_PIN 0xFFFFFF

SL44x2 sl44x2(SC_C7_IO, SC_C2_RST, SC_C1_VCC, SC_SWITCH_CARD_PRESENT, SC_C2_CLK, SC_SWITCH_CARD_PRESENT_INVERT);

char name[20+1];
char code[4+1];
char amount[3+1];
char ack[2];

char inChar;
int index = 0;

enum state {
  NAME,
  WAIT_NAME,
  CODE,
  WAIT_CODE,
  MONEY,
  WAIT_MONEY,
  ACK, 
  WAIT_ACK,
  IDLE
};

state STATE = NAME;
state nextState;
boolean hasFinished = false;

void setup()
{
  Serial.begin(9600);
  while (!Serial) {} // wait for serial port to connect. Needed for Leonardo only
}

// The loop function is called in an endless loop
void loop()
{
  uint8_t  data[SL44X2_DATA_SIZE];

  sl44x2.cardRemoved();

  switch(STATE) {
    case NAME:
    	Serial.print("Saisir le nom : ");
    	index = 0;
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
    	Serial.print("Saisir le code : ");
    	STATE = WAIT_CODE;
    	index = 0;
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
    	Serial.print("Saisir le montant en centimes : ");
    	index = 0;
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
    	Serial.println("Les données suivantes vont être écrites : ");
    	Serial.print(" - Nom     : "); Serial.println(name);
    	Serial.print(" - Code    : "); Serial.println(code);
    	Serial.print(" - Montant : "); Serial.println(amount);
    	Serial.println("Confirmer (O/N) : ");
    	STATE = WAIT_ACK;
    	index = 0;
    	break;
    case WAIT_ACK:
    	waitUserEntry(ack);
    	switch(ack[0]) {
          case 'O':
          case 'o':
          	Serial.println("Ecriture");
          	Serial.println("Insérer carte");
          	break;
          case 'N':
          	STATE = NAME;
          	break;
          default:
          	STATE = ACK;
          	break;
    	}
    	break;
    default:break;
  }
}

void waitUserEntry(char *buff) {
  
  while(Serial.available() <= 0){delay(10);}
  
  while(Serial.available() > 0) {
    inChar = Serial.read();
    buff[index] = inChar;
    index++;
    buff[index] = '\0';
    delay(10);
  }
  
  hasFinished = true;
}

void setNewData(uint8_t  buff[SL44X2_DATA_SIZE]) {

  if(sl44x2.authenticate(SC_PIN)) {
    boolean result = false;
    Serial.println("Authenticated ...");

    // Update Main Memory (Unprotected Area 32 .. 255)

    uint8_t info[] = "2436300Constance";
    int cpt = 0;
    for(; info[cpt] != '\0' ; cpt++) {
      buff[cpt] = info[cpt];
    }
      
    //data[0] = 'C';
    //data[1] = 'o';                                  
    result = sl44x2.updateMainMemory(0x60, buff, cpt); 
                                                      
    if (result) {
      Serial.println("Update worked ...");
    } else {
      Serial.println("Unable to update ...");
    }
  }  
}


void displayCardContent(uint8_t  buff[SL44X2_DATA_SIZE]) {
  uint16_t i = sl44x2.readMainMemory(0, buff, SL44X2_DATA_SIZE);

  if (i > 0) {
    displayContent(buff, i);
  }

  Serial.print("Protection Bits : 0x");
  Serial.println(sl44x2.readProtectionMemory(), HEX);

  uint32_t secMem = sl44x2.readSecurityMemory();

  uint8_t failedAuthentications = 0;
  for(uint8_t i=0; i < 3;i++) {
    if ((secMem && (1<<i)) == 0) {
      failedAuthentications++;
    }
  }

  Serial.print("Security Memory : 0x");
  Serial.println(secMem, HEX);

  Serial.print("Failed authentications : ");
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
        Serial.print("0");
      Serial.print(row * 16, HEX);
      Serial.print("|");

      // Prefill ascii
      for(int i=0; i<16; i++)
        ascii[i] = '.';
      ascii[16] = (char)0x00;
      // colums
      for(uint16_t pos=row*16; pos<(row + 1) * 16; pos++ ) {
        if(pos < size) {
          if(values[pos] < 0x10)
            Serial.print("0");
          Serial.print(values[pos], HEX);
          if(isPrintable(values[pos]))
            ascii[pos - row*16] = (char)values[pos];
        } else {
          Serial.print("  ");
        }
        Serial.print(" ");
      }
      Serial.print("'");
      Serial.print(ascii);
      Serial.println("'");
    }
  }
}