/* Railway Challenge 2024 - VGM (VESC Gateway Module)
 *  By Kiran Chinnam (kiran.chinnam@networkrail.co.uk)
 *  Date: 2024-May-01
 *  Version: 2024-05-B1
 */

// includes
#include <Arduino.h>
#include "STM32F446_RCC_init.h"
#include "STM32_CAN.h"
#include "pin_defs.h"
#include "constants.h"
#include "debug.h"
#include "timing.h"
#include "vesc_proto_defs.h"
#include "utils.h"

// Object declarations
STM32_CAN canIfV(PIN_CAN_V_PORT, PIN_CAN_V_PINS);
STM32_CAN canIfVesc(PIN_CAN_VESC_PORT, PIN_CAN_VESC_PINS);

// Variables
// Co-operative task switcher vars
unsigned long lastDue = 0;
unsigned long nextDue = 0;
unsigned long timeNow;
// Performance Counters
volatile int droppedFrameCtr = 0;
volatile int rxErrCtr = 0;
volatile int rxRc = 0;

// This variable is used to make the heartbeat LED work
int heartbeatCounter = 0;

// VESC Gateway Core
byte knownVescs[] = {KNOWN_VESC_IDS};
VescStatus statuses[KNOWN_VESC_COUNT];

// iter_ctr can be used to perform actions only on every nth run of the primary.
// e.g to run something every 50 ms, use an if statement like this:
// if (iter_ctr % 5 == 0){
//
// }
int iter_ctr = 0;

void secondary();
void primary();

void setup()
{
  // Belts and Braces - Zero out the statuses structs to initialise as zero.
  memset(&statuses, 0, sizeof(statuses));

  pinMode(PIN_HEARTBEAT_LED, OUTPUT);
  digitalWrite(PIN_HEARTBEAT_LED, HIGH);
  // Debugging delay to allow serial monitor to connect.
  delay(2000);

  digitalWrite(PIN_HEARTBEAT_LED, LOW);
  // Setup of pins
  Serial.begin(115200);

  analogReadResolution(12);
  pinMode(PIN_PRIMARY_TASK_TRACE, OUTPUT);
  pinMode(PIN_SECONDARY_TASK_TRACE, OUTPUT);

  // We are using the STM_32 CAN lib directly, so init code goes here
  //--------CAN-V Init---------------
  canIfV.begin(true);
  canIfV.setBaudRate(CAN_BAUDRATE);
  // We don't ever want to receive on the VGM with CAN-V, so set a filter that should never pass
  // The STM32F446 has dual CAN, so filter bank 0 to 13 is for CAN1, 14 to 27 is CAN 2.
  // We can modify this, but no need as we only use a handfull of filters.
  canIfV.setFilter(0, 0x7ff, 0x7ff);

  //--------Internal VESC CAN Init---------------
  // Since we're not transmitting, disable Auto Retransmit.
  canIfVesc.begin(false);
  // The VESC CAN interface shouldn't interfere with VESC operation,
  // so it shouldn't ever transmit. Set silent mode to ensure this.
  canIfVesc.enableSilentMode(true);
  canIfVesc.setBaudRate(VESC_CAN_BAUDRATE);
  // Refer to comment above, CAN2 should only use filterbank 14 to 27
  // according to default config of CAN lib.
  // The VESC uses bits 15 to 8 for packet type, and bit 7 to 0 for VESC ID.
  // This should be a mask of 0xff00.
  // We'll map the status messages so that filterbank = 20 + status number
  // e.g. status _4 is filter 24.

  canIfVesc.setMBFilterProcessing(CAN_BANK::MB20, VESC_STATUS_1 << 8, 0xFF00);
  canIfVesc.setMBFilterProcessing(CAN_BANK::MB21, VESC_STATUS_2 << 8, 0xFF00);
  canIfVesc.setMBFilterProcessing(CAN_BANK::MB22, VESC_STATUS_3 << 8, 0xFF00);
  canIfVesc.setMBFilterProcessing(CAN_BANK::MB23, VESC_STATUS_4 << 8, 0xFF00);
  canIfVesc.setMBFilterProcessing(CAN_BANK::MB24, VESC_STATUS_5 << 8, 0xFF00);
  canIfVesc.setMBFilterProcessing(CAN_BANK::MB25, VESC_STATUS_6 << 8, 0xFF00);

  Serial.printf("ready\n");
  lastDue = us();
  nextDue = us() + COOP_OS_MAINTASK_PERIOD;
  digitalWrite(PIN_HEARTBEAT_LED, HIGH);
}

// -------------DO NOT EDIT THE MAIN LOOP!!!----------------
// The main loop is here just to manage the running of a simple 'co-operative' task switcher.
// The real work happens in the primary and secondary functions. loop() ensures that the primary task
// is executed once every 10 ms, and tries to squeeze in as many executions of secondary in between as possible.
//
// While the nucleo has the capability to run a full RTOS, a simple co-operative model should work just fine
// Maybe a future improvement?
void loop()
{
  timeNow = us();
  // Second OR clause is to handle the case where micros() overflows
  if (timeNow < nextDue || (nextDue < lastDue && timeNow > lastDue))
  {
    digitalWrite(PIN_SECONDARY_TASK_TRACE, HIGH);
    secondary();
    digitalWrite(PIN_SECONDARY_TASK_TRACE, LOW);
  }
  else
  {
    lastDue = nextDue;
    nextDue += COOP_OS_MAINTASK_PERIOD;
    digitalWrite(PIN_PRIMARY_TASK_TRACE, HIGH);
    primary();
    digitalWrite(PIN_PRIMARY_TASK_TRACE, LOW);
  }
}

void primary()
{
  iter_ctr++;

  if(iter_ctr%100==0){
    
    for (int i = 0; i < 4; i++)
    {
      Serial.print(i);
      Serial.print(": ");
      Serial.print(statuses[i].voltageIn);
      Serial.print("V "); 
      Serial.print(statuses[i].tachoRel);
      Serial.print("n "); 
      Serial.print(statuses[i].rpm);
      Serial.print("-1/min "); 
      Serial.print(statuses[i].current);
      Serial.print("A "); 
      Serial.print(statuses[i].inputCurrent);
      Serial.print("A\n");
    }
    Serial.println();

  }

  // Blink the LED to show we're still alive
  heartbeatCounter++;
  heartbeatCounter %= 100;
  if (heartbeatCounter == 0 || heartbeatCounter == 20)
  {
    digitalWrite(PIN_HEARTBEAT_LED, HIGH);
  }
  if (heartbeatCounter == 10 || heartbeatCounter == 30)
  {
    digitalWrite(PIN_HEARTBEAT_LED, LOW);
  }
}

// This function should not need much/any modification.
void secondary()
{
  // Gobble and discard any messages that do get through
  //  on the CAN-V interface to keep the mailbox free.
  CAN_message_t dummy;
  canIfV.read(dummy);

  CAN_message_t vescRxMsg;
  if (canIfVesc.read(vescRxMsg))
  {
    // Received a message from a VESC
    // Extract the VESC ID and see if it is from a VESC that we know
    byte vId = vescRxMsg.id & 0xff;
    int vescIndex = -1;
    for (int i = 0; i < KNOWN_VESC_COUNT; i++)
    {
      if (knownVescs[i] == vId)
      {
        vescIndex = i;
        break;
      }
    }
    // Discard the packet if it doesn't match a pre-defined index.
    if (vescIndex < 0)
      return;
    int statusMessageNumber = 0;
    switch ((vescRxMsg.id >> 8) & 0xFF)
    {
    case VESC_STATUS_1:
      statusMessageNumber = 1;
      statuses[vescIndex].rpm = decodeBigEndianInt32(vescRxMsg.buf);
      statuses[vescIndex].current = decodeBigEndianInt16(&vescRxMsg.buf[4]) / 10.0f;
      statuses[vescIndex].dutyCycle = decodeBigEndianInt16(&vescRxMsg.buf[6]) / 1000.0f;
      break;
    case VESC_STATUS_2:
      statusMessageNumber = 2;
      statuses[vescIndex].ahConsumed = decodeBigEndianInt32(vescRxMsg.buf) / 10000.0f;
      statuses[vescIndex].ahCharged = decodeBigEndianInt32(&vescRxMsg.buf[4]) / 10000.0f;
      break;
    case VESC_STATUS_3:
      statusMessageNumber = 3;
      statuses[vescIndex].whConsumed = decodeBigEndianInt32(vescRxMsg.buf) / 10000.0f;
      statuses[vescIndex].whCharged = decodeBigEndianInt32(&vescRxMsg.buf[4]) / 10000.0f;
      break;
    case VESC_STATUS_4:
      statusMessageNumber = 4;
      statuses[vescIndex].mosfetTemp = decodeBigEndianInt16(vescRxMsg.buf) / 10.0f;
      statuses[vescIndex].motorTemp = decodeBigEndianInt16(&vescRxMsg.buf[2]) / 10.0f;
      statuses[vescIndex].inputCurrent = decodeBigEndianInt16(&vescRxMsg.buf[4]) / 10.0f;
      statuses[vescIndex].pidPosition = decodeBigEndianInt16(&vescRxMsg.buf[6]) / 50.0f;
      break;
    case VESC_STATUS_5:
      statusMessageNumber = 5;
      statuses[vescIndex].tachoRel = decodeBigEndianInt32(vescRxMsg.buf);
      statuses[vescIndex].voltageIn = decodeBigEndianInt16(&vescRxMsg.buf[4]) / 10.0f;
      break;
    case VESC_STATUS_6:
      statusMessageNumber = 6;
      statuses[vescIndex].adc1millivolts = decodeBigEndianInt16(vescRxMsg.buf);
      statuses[vescIndex].adc2millivolts = decodeBigEndianInt16(&vescRxMsg.buf[2]);
      statuses[vescIndex].adc3millivolts = decodeBigEndianInt16(&vescRxMsg.buf[4]);
      statuses[vescIndex].servoPosition = decodeBigEndianInt16(&vescRxMsg.buf[6]);
      break;
    default:
      // Unrecognised message somehow got through the filters - ignore it.
      return;
    }
    statuses[vescIndex].seenBefore = true;
    statuses[vescIndex].lastContactMillis = millis();

    CAN_message_t txMsg;
    // ID transform allows the STCM and ECM to continue using standard 11-bit IDs.
    txMsg.id = ((vescIndex+1)<<4) | statusMessageNumber;
    memcpy(txMsg.buf,vescRxMsg.buf,8);
    txMsg.len = vescRxMsg.len;

    //Re-tx the message onto CAN-V
    canIfV.write(txMsg);
  }
}
