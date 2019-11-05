/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <bluefruit.h>

#define SerialDebug   Serial
#define SerialCom     Serial1

// ASCII output goes to this console UART port which might be different from
// the debug port.
#define combegin(...)     SerialCom.begin(__VA_ARGS__)
#define comprint(...)     SerialCom.print(__VA_ARGS__)
#define comprintln(...)   SerialCom.println(__VA_ARGS__)
#define comread(...)      SerialCom.read(__VA_ARGS__)
#define comreadBytes(...) SerialCom.readBytes(__VA_ARGS__)
#define comwrite(...)     SerialCom.write(__VA_ARGS__)
#define comavailable(...) SerialCom.available(__VA_ARGS__)

#define DEBUG_ON  0
#if DEBUG_ON
#define dbbegin(...)      SerialDebug.begin(__VA_ARGS__)
#define dbprint(...)      SerialDebug.print(__VA_ARGS__)
#define dbprintln(...)    SerialDebug.println(__VA_ARGS__)
#else
#define dbbegin(...)
#define dbprint(...)
#define dbprintln(...)
#endif

BLEDis bledis;
BLEHidAdafruit blehid;

void setup()
{
  combegin(8*115200);
  dbbegin(115200);
  while ( !SerialDebug && millis() < 2000) delay(10);   // for nrf52840 with native usb

  dbprintln("Bluefruit52 HID Mouse Example");
  dbprintln("--------------------------------\n");

  dbprintln();
  dbprintln("Go to your phone's Bluetooth settings to pair your device");
  dbprintln("then open an application that accepts mouse input");

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Bluefruit52");

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();

  /* Start BLE HID
   * Note: Apple requires BLE device must have min connection interval >= 20m
   * ( The smaller the connection interval the faster we could send data).
   * However for HID and MIDI device, Apple could accept min connection interval
   * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
   * connection interval to 11.25  ms and 15 ms respectively for best performance.
   */
  blehid.begin();

  /* Set connection interval (min, max) to your perferred value.
   * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
   * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms
   */
  /* Bluefruit.setConnInterval(9, 12); */
  // HID Device can have a min connection interval of 9*1.25 = 11.25 ms
  Bluefruit.Periph.setConnInterval(9, 16); // min = 9*1.25=11.25 ms, max = 16*1.25=20ms

  // Set up and start advertising
  startAdv();
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);

  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   *
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

const uint8_t STX =0x02;
const uint8_t ETX =0x03;
enum state_t {WAITSTX, WAITTYPELEN, WAITDATA, WAITETX};
state_t message_state = WAITSTX;
enum msgType_t {KEYBOARD, MOUSE, JOYSTICK, MIDI};
msgType_t msgType;
uint8_t msgLen, msgCount;
uint8_t message[32];

void loop()
{
  // <STX><message type/len><message data...><ETX>
  // message len bits 4..0 = number of bytes of message data
  // message type bits 7..5 = message type
  //   0 = keyboard, 1 = mouse, 2 = joystick, 3 = MIDI
  // Examples:
  // 0,8 = keyboard HID report
  // 1,4 = mouse HID report
  //
  int bytesAvail = comavailable();
  while (bytesAvail > 0) {
    hid_mouse_report_t mouse_report;
    uint8_t inByte = comread();
    bytesAvail--;
    dbprintln(); dbprint("inByte="); dbprintln(inByte);
    switch (message_state) {
      case WAITSTX:
        dbprintln("WAITSTX");
        if (inByte == STX) {
          message_state = WAITTYPELEN;
        }
        break;
      case WAITTYPELEN:
        dbprintln("WAITTYPELEN");
        msgCount = 0;
        msgLen = inByte & 0x1F;
        msgType = (msgType_t)((inByte & 0xE0) >> 5);
        dbprint("msgType="); dbprint(msgType);
        dbprint(" msgLen="); dbprintln(msgLen);
        if (msgLen == 0) {
          message_state = WAITSTX;
        }
        else {
          message_state = WAITDATA;
        }
        break;
      case WAITDATA:
        dbprint("WAITDATA bytesAvail="); dbprint(bytesAvail);
        dbprint(" msgCount="); dbprintln(msgCount);
        message[msgCount++] = inByte;
        if (msgCount >= msgLen) {
          message_state = WAITETX;
        }
        else {
          if (bytesAvail > 1) {
            size_t bytesIn = comreadBytes(&message[msgCount], min(msgLen-msgCount, bytesAvail));
            msgCount += bytesIn;
            bytesAvail -= bytesIn;
            if (msgCount >= msgLen) {
              message_state = WAITETX;
            }
          }
        }
        break;
      case WAITETX:
        dbprintln("WAITETX");
        if (inByte == ETX) {
          // Good message received so process it.
          switch(msgType) {
            case KEYBOARD:
              blehid.keyboardReport(message[0], &message[2]);
              break;
            case MOUSE:
              dbprint(F("Mouse HID report: "));
              for (uint8_t i = 0; i < msgLen; i++) {
                dbprint(message[i], HEX); dbprint(' ');
              }
              dbprintln();
              memset(&mouse_report, 0, sizeof(mouse_report));
              memcpy(&mouse_report, message, msgLen);
              blehid.mouseReport(&mouse_report);
              break;
            case JOYSTICK:
              break;
            case MIDI:
              break;
            default:
              break;
          }
        }
        message_state = WAITSTX;
        break;
      default:
        dbprint("Invalid message state=");
        dbprintln(message_state);
        break;
    }
  }

  // Request CPU to enter low-power mode until an event/interrupt occurs
  waitForEvent();
}
