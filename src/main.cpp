/*
  RadioLib SX127x Ping-Pong Example

  This example is intended to run on two SX126x radios,
  and send packets between the two.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// uncomment the following only on one
// of the nodes to initiate the pings
// #define INITIATING_NODE

// SX1276 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// NRST pin:  9
// DIO1 pin:  3
// SX1276 radio = new Module(10, 2, 9, 3);
SX1276 radio = nullptr; // SX1276

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
  // we sent or received  packet, set the flag
  operationDone = true;
}

void setupLoRa()
{
  radio = new Module(18, 26, 14, 33); // TTGO  CS, DI0, RST, BUSY

  Serial.print("[SX1276] Initializing ...  ");
  // carrier frequency:           868.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            9
  // coding rate:                 7
  // sync word:                   0x12 (private network)
  // output power:                14 dBm
  // current limit:               60 mA
  // preamble length:             8 symbols
  // TCXO voltage:                1.6 V (set to 0 to not use TCXO)
  // regulator:                   DC-DC (set to true to use LDO)
  // CRC:                         enabled
  int state = radio.begin(
      868.0, // float freq              = Fréquence correcte pour l'Europe
      62.5,  // float bw                = Bande passante réduite
      12,    // uint8_t sf              = Facteur d'étalement maximal
      8,     // uint8_t cr              = Taux de codage plus robuste
      0x12,  // uint8_t syncWord        = SyncWord par défaut
      14,    // int8_t power            = Puissance maximale autorisée
      12,    // uint16_t preambleLength = Longueur de préambule augmentée
      1      // uint8_t gain            // NB this param is different on SX126x => float tcxoVoltage
             // false                  // bool useRegulatorLDO    // NB this param exists on SX126x only
  );

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      yield();
  }
}

int transmitMsg()
{
  static uint16_t cnt = 0;
  String msg =  ", cnt: " + String(++cnt) + ", millis: " +  String(millis()) + ", HelloFrom: " + PROJECT_NAME + ", CompilationTime: " + __TIME__;
  int state = radio.startTransmit(msg);
  return (state);
}

void setup()
{
  Serial.begin(BAUD_RATE);
  for (size_t i = 0; i < 10; i++)
  {
    Serial.println(i);
    delay(1000);
  }

  // initialize SX1276 with default settings
  Serial.print(F("[SX1276] Initializing ... "));
  setupLoRa();

  // set the function that will be called
  // when new packet is received
  radio.setDio0Action(setFlag, RISING);

#if defined(INITIATING_NODE)
  // send the first packet on this node
  Serial.print(F("[SX1276] Sending first packet ... "));
  transmissionState = transmitMsg();
  transmitFlag = true;
#else
  // start listening for LoRa packets on this node
  Serial.print(F("[SX1276] Starting to listen ... "));
  int16_t state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
    {
      delay(10);
    }
  }
#endif
}

void loop()
{
  // check if the previous operation finished
  if (operationDone)
  {
    // reset flag
    operationDone = false;

    if (transmitFlag)
    {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE)
      {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));
      }
      else
      {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;
    }
    else
    {
      // the previous operation was reception
      // print data and send another packet
      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE)
      {
        // packet was successfully received
        Serial.println(F("[SX1276] Received packet!"));

        // print data of the packet
        Serial.print(F("[SX1276] Data:\t\t"));
        Serial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[SX1276] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[SX1276] SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));
      }

      // wait a second before transmitting again
      delay(1000);

      // send another one
      Serial.print(F("[SX1276] Sending another packet ... "));
      transmissionState = transmitMsg();
      transmitFlag = true;
    }
  }
}
