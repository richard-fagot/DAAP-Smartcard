// You might want to remove the support for the asynchronous cards from the SCLib, because this is not needed in
// this case and it will reduce the size of the sketch dramatically that there might be more space for your application
// code an a Arduino UNO etc.
// This can be done by modifying the SCLib.h (Comment out the ASYNCHRON_CARDS define .. See comment in SCLib.h)
#include "SL44x2.h"

// If you are using a Arduino Mega compatible board you need to change the SC_C2_CLK to 11 as the TIMER1A
// is used for asynchronous clock generation (1MHz with just plain arduino code is no fun ;-) )
// and the SC_C1_VCC can be changed to any other "free" digital pin.
#define SC_C2_RST              7
#define SC_C1_VCC              11
#define SC_C7_IO               10
#define SC_C2_CLK              9

// Default behavior of the signal connected to SC_SWITCH_CARD_PRESENT is
// that the signal is HIGH, when card is present and LOW otherwise.
#define SC_SWITCH_CARD_PRESENT 8

// If the signal on PIN SC_SWITCH_CARD_PRESENT has an inverted
// characteristic (LOW card present, HIGH otherwise) this can be signaled
// via the following define. The present signal will be inverted by SL44x2 object.
#define SC_SWITCH_CARD_PRESENT_INVERT false

// Pin used for authenticate with SL4442 smartcard (Most cards are sold with 0xFFFFFF)
#define SC_PIN 0xFFFFFF

SL44x2 sl44x2(SC_C7_IO, SC_C2_RST, SC_C1_VCC, SC_SWITCH_CARD_PRESENT, SC_C2_CLK, SC_SWITCH_CARD_PRESENT_INVERT);

//The setup function is called once at startup of the sketch
void setup()
{
  Serial.begin(9600);
  while (!Serial) {} // wait for serial port to connect. Needed for Leonardo only
}

// The loop function is called in an endless loop
void loop()
{
  uint8_t  data[SL44X2_DATA_SIZE];

  Serial.println("Waiting for Smartcard");

  // cardReady(true) will wait until card is inserted and recognized as supported.
  // It's necessary to wait for the card to be recognized via the cardRaeady() function
  // as this will take care of all initialization of the sl44x2 object
  // As the cardReady() function is a blocking call, which might not be apropriate in all
  // cases. You might want to read other user input etc. there is also the cardInserted() function, which just
  // gives you the information if the card was physically inserted into the slot. After that you can call 
  // the cardReady() to start the recognition process.
  if (sl44x2.cardReady()) {
    // You don't need to be authenticated to read the complete data
    // of the smartcard
    uint16_t i = sl44x2.readMainMemory(0, data, SL44X2_DATA_SIZE);

#if defined(SC_DEBUG)
    if (i > 0) {
      sl44x2.dumpHEX(data, i);
    }
#endif

    // Read the protection bits indicating the protection status of the
    // first 32 bytes of the smart card
    Serial.print("Protection Bits : 0x");
    Serial.println(sl44x2.readProtectionMemory(), HEX);

    // Only available if card supports PSC (SLE4442/SLE4441/SLE4440)
#if defined(SL44X2_SUPPORT_PSC)
    // Read the security memory of the SL44x2 card (Only supported with
    // SL4442 and not with SL4432 (Will return 0xFFFFFFFF in that case)
    uint32_t secMem = sl44x2.readSecurityMemory();

    // The last 3 bits of the security memory give the number of possible
    // authentications. Every bit stands for one authentication. If all bits
    // are cleared, the cards is not changeable anymore (No way to get back
    // from this point, because there is no "MASTER PIN" etc.). That
    // means you have 3 tries to authenticate to the card, before it becomes
    // unwritable (Reading is always possible, even without authentication)
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

    
    // Uncomment the next lines only , if you've set the correct PIN for the
    // card you are using in the SC_PIN define above
    // I've tested this feature quite some time and it worked all the time
    // but after 3 unsuccessful authentications the card is not writable anymore
    // See note above.
    // If you are using a SL4432, there is no need to authenticate, as the card is not
    // supporting authentication, you will be able to modify all card data, if it's not
    // protected by the protection bits (data bytes 0 - 31) or if the index is 32 .. 255
    if(sl44x2.authenticate(SC_PIN)) {
      boolean result = false;
      Serial.println("Authenticated ...");

      // Update Main Memory (Unprotected Area 32 .. 255)
      uint8_t info[] = "2436300Constance";
      int cpt = 0;
      for(; info[cpt] != '\0' ; cpt++) {
        data[cpt] = info[cpt];
      }
      
      //data[0] = 'C';
      //data[1] = 'o';
      result = sl44x2.updateMainMemory(0x60, data, i-1);

      if (result) {
        Serial.println("Update worked ...");
      } else {
        Serial.println("Unable to update ...");
      }
uint16_t i = sl44x2.readMainMemory(0, data, SL44X2_DATA_SIZE);

#if defined(SC_DEBUG)
    if (i > 0) {
      sl44x2.dumpHEX(data, i);
    }
#endif
//      //
//      // You might want to change the PSC of the SL4442 (To set a new PIN)
//      // As most cards are sold with 0xFFFFFF
//      // No need to give the old pin, as you are already authenticated
//      //result = sl44x2.changePSC(0x123456);
//
//      // Update Main Memory (Protected Area 00 .. 31)
//      data[0] = 0x55;
//      data[1] = 0xAA;
//      result = sl44x2.updateMainMemory(30, data, 2);
//
//      // Protect byte against further modification
//      // This process is irreversible, so use with care ;-.)
//      //result = sl44x2.protectByte(30);
//      //result = result && sl44x2.protectByte(31);
//
//      //if (result) {
//      //  Serial.println("Set Protection on bytes 30 and 31 ...");
//      //} else {
//      //  Serial.println("Unable to set protection on bytes 30 and 31 ...");
//      //}
    }
#endif
  } else {
    Serial.println("Unable to identify card ... Please remove");
  }

  // Will wait until Card is removed and re-inits
  // sl44x2 object for next communication session
  sl44x2.cardRemoved();
}
