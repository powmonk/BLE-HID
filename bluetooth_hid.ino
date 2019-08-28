#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
#include <driver/adc.h>

BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

uint8_t buttons       = 0;
uint8_t button1       = 0;
uint8_t button2       = 0;
uint8_t button3       = 0;
bool connected        = false;
const int PushButton  = 12;
bool prevState = 1;

class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    connected = true;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(true);
  }

  void onDisconnect(BLEServer* pServer){
    connected = false;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(false);
  }
};

/*
 * This callback is connect with output report. In keyboard output report report special keys changes, like CAPSLOCK, NUMLOCK
 * We can add digital pins with LED to show status
 * bit 0 - NUM LOCK
 * bit 1 - CAPS LOCK
 * bit 2 - SCROLL LOCK
 */
 class MyOutputCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic* me){
    uint8_t* value = (uint8_t*)(me->getValue().c_str());
    ESP_LOGI(LOG_TAG, "special keys: %d", *value);
  }
};

void taskServer(void*){


    BLEDevice::init("CodeNameZombie");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyCallbacks());

    hid = new BLEHIDDevice(pServer);
    input = hid->inputReport(1); // <-- input REPORTID from report map
    output = hid->outputReport(1); // <-- output REPORTID from report map

    output->setCallbacks(new MyOutputCallbacks());

    std::string name = "Digital Anyone Inc.";
    hid->manufacturer()->setValue(name);

    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    hid->hidInfo(0x00,0x02);

  BLESecurity *pSecurity = new BLESecurity();
//  pSecurity->setKeySize();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    const uint8_t report[] = {
      USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
//      USAGE(1),           0x06,       // Keyboard
      USAGE(1),           0x07,       // Gamepad??
      COLLECTION(1),      0x01,       // Application
      REPORT_ID(1),       0x01,        //   Report ID (1)
      USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
      USAGE_MINIMUM(1),   0xE0,
      USAGE_MAXIMUM(1),   0xE7,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
      REPORT_COUNT(1),    0x08,
      HIDINPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
      REPORT_SIZE(1),     0x08,
      HIDINPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
      REPORT_SIZE(1),     0x08,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
      USAGE_MINIMUM(1),   0x00,
      USAGE_MAXIMUM(1),   0x65,
      HIDINPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
      REPORT_SIZE(1),     0x01,
      USAGE_PAGE(1),      0x08,       //   LEDs
      USAGE_MINIMUM(1),   0x01,       //   Num Lock
      USAGE_MAXIMUM(1),   0x05,       //   Kana
      HIDOUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
      REPORT_SIZE(1),     0x03,
      HIDOUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      END_COLLECTION(0)
    };

    hid->reportMap((uint8_t*)report, sizeof(report));
    hid->startServices();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_GAMEPAD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();
    hid->setBatteryLevel(7);

    ESP_LOGD(LOG_TAG, "Advertising started!");
    delay(portMAX_DELAY);
  
};

void setup() {
  Serial.begin(115200);
  //set the swithc pin as input
  pinMode(PushButton, INPUT_PULLUP);
  Serial.println("Starting BLE work!");

//  pinMode(12, INPUT_PULLDOWN);
//  attachInterrupt(digitalPinToInterrupt(12), clickNumLock, CHANGE);   // Num Lock
//  pinMode(14, INPUT_PULLDOWN);
//  attachInterrupt(digitalPinToInterrupt(13), clickCapsLock, CHANGE);   // Caps Lock
//  pinMode(32, INPUT_PULLDOWN);
//  attachInterrupt(digitalPinToInterrupt(32), clickScrollLock, CHANGE);   // Scroll Lock


  xTaskCreate(taskServer, "server", 20000, NULL, 5, NULL);
}

void loop() {
  delay(1);
  if(connected){
    int currState = digitalRead(PushButton);
     //Serial.println(Push_button_state);
     bool togglePlay = false;

    if(currState != prevState){
      if(currState == 1){
        //Serial.println("button released");
        togglePlay = true;
      }

      if(currState == 0){
        //Serial.println("button pressed");
        togglePlay = true;
      }
    }


    if(togglePlay){
  
        //delay(10);
        uint8_t keyToPress = 0x08;
    
 
  
        //Send the keypress
        uint8_t msg[] = {0x0,0x0,0x0,0x0, keyToPress,0x0,0x0,0x0,0x0,0x0};
        //uint8_t msg[] = {0X0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
        input->setValue(msg,sizeof(msg));
        input->notify();
  
        //Send null key to stop the kepress
        uint8_t msg1[] = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
        input->setValue(msg1,sizeof(msg1));
        input->notify();
          
    }

    prevState = currState;
    delay(10);

  }
}
