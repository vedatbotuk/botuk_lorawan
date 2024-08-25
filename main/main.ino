/* Heltec Automation LoRaWAN communication example
 *
 * Function:
 * 1. Upload node data to the server using the standard LoRaWAN protocol.
 *  
 * Description:
 * 1. Communicate using LoRaWAN protocol.
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * */

#include "LoRaWan_APP.h"
#include <AM2302-Sensor.h>
#include "../credentials.h"

// TEMPERATURE
constexpr unsigned int SENSOR_PIN {7U};
AM2302::AM2302_Sensor am2302{SENSOR_PIN};

/* OTAA para*/
uint8_t devEui[] = {
  DEV_EUI_0, DEV_EUI_1, DEV_EUI_2, DEV_EUI_3,
  DEV_EUI_4, DEV_EUI_5, DEV_EUI_6, DEV_EUI_7
};

uint8_t appEui[] = {
  APP_EUI_0, APP_EUI_1, APP_EUI_2, APP_EUI_3,
  APP_EUI_4, APP_EUI_5, APP_EUI_6, APP_EUI_7
};

uint8_t appKey[] = {
  APP_KEY_0, APP_KEY_1, APP_KEY_2, APP_KEY_3,
  APP_KEY_4, APP_KEY_5, APP_KEY_6, APP_KEY_7,
  APP_KEY_8, APP_KEY_9, APP_KEY_10, APP_KEY_11,
  APP_KEY_12, APP_KEY_13, APP_KEY_14, APP_KEY_15
};

/* ABP para*/
uint8_t nwkSKey[] = {
  NWK_SKEY_0, NWK_SKEY_1, NWK_SKEY_2, NWK_SKEY_3,
  NWK_SKEY_4, NWK_SKEY_5, NWK_SKEY_6, NWK_SKEY_7,
  NWK_SKEY_8, NWK_SKEY_9, NWK_SKEY_10, NWK_SKEY_11,
  NWK_SKEY_12, NWK_SKEY_13, NWK_SKEY_14, NWK_SKEY_15
};

uint8_t appSKey[] = {
  APP_SKEY_0, APP_SKEY_1, APP_SKEY_2, APP_SKEY_3,
  APP_SKEY_4, APP_SKEY_5, APP_SKEY_6, APP_SKEY_7,
  APP_SKEY_8, APP_SKEY_9, APP_SKEY_10, APP_SKEY_11,
  APP_SKEY_12, APP_SKEY_13, APP_SKEY_14, APP_SKEY_15
};

uint32_t devAddr = DEV_ADDR;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 60000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
    /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
    float temperature = (float)(am2302.get_Temperature());
    float humidity = (float)(am2302.get_Humidity());
    unsigned char *puc;

    puc = (unsigned char *)(&temperature);
    appDataSize = 8;
    appData[0] = puc[0];
    appData[1] = puc[1];
    appData[2] = puc[2];
    appData[3] = puc[3];

    puc = (unsigned char *)(&humidity);
    appData[4] = puc[0];
    appData[5] = puc[1];
    appData[6] = puc[2];
    appData[7] = puc[3];

    Serial.print("T=");
    Serial.print(temperature);
    Serial.print("C, RH=");
    Serial.print(humidity);
    Serial.print("%,");
}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ 


void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      //both set join DR and DR when ADR off 
      LoRaWAN.setDefaultDR(3);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}
