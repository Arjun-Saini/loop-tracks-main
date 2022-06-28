/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/sainihome/Documents/GitHub/loop-tracks/master/src/master.ino"
#include "HttpClient.h"
#include "JsonParserGeneratorRK.h"
#include "math.h"
#include "dotstar.h"

float getDistance(float trainLat, float trainLon);
void setup();
void loop();
void randomizeAddress();
hal_i2c_config_t acquireWireBuffer();
#line 6 "/Users/sainihome/Documents/GitHub/loop-tracks/master/src/master.ino"
class Checkpoint{
  public:
  
    float lat;
    float lon;

    Checkpoint(float la, float lo){
      lat = la;
      lon = lo;
    }

    float getDistance(float trainLat, float trainLon){
      return sqrt(pow((trainLat - lat), 2) + pow((trainLon - lon), 2));
    }
};

constexpr size_t I2C_BUFFER_SIZE = 512;

const int slaveCountExpected = 1;
int addressArr[slaveCountExpected];
int sequenceArr[slaveCountExpected];
int slaveCount, bleCount;

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

http_header_t headers[] = {
  {"Accept", "/*/"},
  {NULL, NULL}
};

http_request_t request;
http_response_t response;

Checkpoint redLineCheckpoints[] = {Checkpoint(41.853028, -87.63109), Checkpoint(41.9041, -87.628921), Checkpoint(41.903888, -87.639506), Checkpoint(41.913732, -87.652380), Checkpoint(41.924615, -87.716979)};

HttpClient http;
JsonParserStatic<10000, 1000> parser;

void setup() {
  Serial.begin(9600);
  delay(2000);

  BLE.on();

  BLE.addCharacteristic(txCharacteristic);
  BLE.addCharacteristic(rxCharacteristic);

  BleAdvertisingData data;
  data.appendServiceUUID(serviceUuid);
  BLE.advertise(&data);

  acquireWireBuffer();
  Wire.begin();
  randomizeAddress();

  request.hostname = "lapi.transitchicago.com";
  request.port = 80;
  request.path = "/api/1.0/ttpositions.aspx?key=00ff09063caa46748434d5fa321d048f&rt=red&outputType=JSON";
}

void loop() {
  http.get(request, response, headers);

  parser.clear();
	parser.addString(response.body);
  if (!parser.parse()) {
		Serial.println("parsing failed");
		return;
	}
  
  //loop through each train, loop breaks when all trains have been parsed
  int count = 0;
  while(true){
    JsonReference train = parser.getReference().key("ctatt").key("route").index(0).key("train").index(count);
    String nextStation = train.key("nextStaNm").valueString();
    String trainDir = train.key("trDr").valueString();
    float lat = train.key("lat").valueString().toFloat();
    float lon = train.key("lon").valueString().toFloat();
    int pos;

    if(nextStation.length() <= 1){
      break;
    }

    Serial.printf("Train %i: ", count);
    int arrSize = sizeof(redLineCheckpoints) / sizeof(redLineCheckpoints[0]);
    float checkpointDistances[arrSize];
    for(int i = 0; i < arrSize; i++){
      checkpointDistances[i] = redLineCheckpoints[i].getDistance(lat, lon);
    }
    float* closestCheckpoint = std::min_element(checkpointDistances, checkpointDistances + arrSize);
    Serial.println(closestCheckpoint - checkpointDistances);

    count++;
  }

  // for(int i = 0; i < arraySize(redLineOutput); i++){
  //   if(redLineOutput[i] == 1){
  //     strip.setPixelColor(i - 1, red);
  //     strip.setPixelColor(i, red);
  //     strip.setPixelColor(i + 1, brightRed);
  //   }else if(redLineOutput[i] == 5){
  //     strip.setPixelColor(i - 1, brightRed);
  //     strip.setPixelColor(i, red);
  //     strip.setPixelColor(i + 1, red);
  //   }
  //   redLineOutput[i] = 0;
  // }

  //strip.show();
  delay(5000);
}

//clears up conflicts with multiple i2c slaves having the same address
void randomizeAddress(){
  while(slaveCount != slaveCountExpected){
    slaveCount = 0;
    for(int i = 8; i <= 119; i++){
      Serial.println("\nrequest code 1, address: " + String(i));
      Wire.beginTransmission(i);
      Wire.write('1');
      Wire.endTransmission();

      Wire.requestFrom(i, 24);
      if(Wire.available() > 0){
        Serial.println("transmission recieved from: " + String(i));

        slaveCount++;

        String inputBuffer = "";
        char c;
        for(int j = 0; j < 24; j++){
          c = Wire.read();
          inputBuffer += c;
        }
        Wire.beginTransmission(i);
        Wire.write(inputBuffer);
        Serial.println("device id: " + inputBuffer);
        Wire.endTransmission();
        Serial.println("transmission sent to: " + String(i));

        Wire.beginTransmission(i);
        Wire.write('2');
        Wire.endTransmission();

        Serial.println("request code 2, address: " + String(i));
        Wire.requestFrom(i, 4);
        inputBuffer = "";
        for(int j = 0; j < 4; j++){
          inputBuffer += (char)Wire.read();
        }
        
        Serial.println("conflict verification: " + inputBuffer);
      }
    }
  }

  Serial.println("\nConnected to: ");

  int count = 0;
  for(int i = 8; i<= 119; i++){
    Wire.beginTransmission(i);
    Wire.write('1');
    Wire.endTransmission();

    Wire.requestFrom(i, 24);
    if(Wire.available() > 0){
      Serial.print(i);
      Serial.print(", ");

      addressArr[count] = i;
      count++;
    }
  }
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context){
  String inputBuffer = "";

  if(bleCount <= slaveCountExpected){
    int input;

    for(int i = 0; i < len - 1; i++){
      inputBuffer += (char)data[i];
      input = atoi(inputBuffer);
    }

    if(bleCount < slaveCountExpected){
      txCharacteristic.setValue("\nEnter the position of the device with the blue LED as an integer (first device is at 1, second is at 2, etc): ");
    }

    Wire.beginTransmission(addressArr[bleCount]);
    Wire.write('3');
    Wire.endTransmission();

    if(bleCount > 0){
      sequenceArr[input - 1] = addressArr[bleCount - 1];
      
      Wire.beginTransmission(addressArr[bleCount - 1]);
      Wire.write('4');
      Wire.endTransmission();
    }
  }

  if(bleCount == slaveCountExpected){
    delay(1000);
    Serial.println("\nSequence: ");
    for(int i = 0; i < slaveCountExpected; i++){
      Serial.print(sequenceArr[i]);
      Serial.print(", ");

      Wire.beginTransmission(sequenceArr[i]);
      Wire.write('3');
      Wire.endTransmission();
      delay(2000);
      Wire.beginTransmission(sequenceArr[i]);
      Wire.write('4');
      Wire.endTransmission();
    }
    //BLE.disconnect();
    //BLE.off();
  }

  bleCount++;
}

hal_i2c_config_t acquireWireBuffer() {
    hal_i2c_config_t config = {
        .size = sizeof(hal_i2c_config_t),
        .version = HAL_I2C_CONFIG_VERSION_1,
        .rx_buffer = new (std::nothrow) uint8_t[I2C_BUFFER_SIZE],
        .rx_buffer_size = I2C_BUFFER_SIZE,
        .tx_buffer = new (std::nothrow) uint8_t[I2C_BUFFER_SIZE],
        .tx_buffer_size = I2C_BUFFER_SIZE
    };
    return config;
}