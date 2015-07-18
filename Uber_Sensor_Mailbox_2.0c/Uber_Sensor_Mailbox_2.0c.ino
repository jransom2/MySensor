#include <SPI.h>
#include <MySensor.h>
// for battery checking
#include <Vcc.h>

// for temp sensor
#include <DHT.h>

const float VccMin   = 0.0;           // Minimum expected Vcc level, in Volts.
const float VccMax   = 3.71;           // Maximum expected Vcc level, in Volts.
const float VccCorrection = 3.71 / 3.57; // Measured Vcc by multimeter divided by reported Vcc

Vcc vcc(VccCorrection);

MySensor gw;

#define CHILD_ID_TEMP 4
#define TEMP_SENSOR_ANALOG_PIN A0
MyMessage msgTEMP(CHILD_ID_TEMP, V_TEMP);

#define CHILD_ID_HUM 5
#define HUMIDITY_SENSOR_ANALOG_PIN A0
MyMessage msgHUM(CHILD_ID_HUM, V_HUM);

#define CHILD_ID_DOOR 7
#define DIGITAL_INPUT_SENSOR_DOOR 2   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT DIGITAL_INPUT_SENSOR_DOOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
MyMessage msgDOOR(CHILD_ID_DOOR, V_LIGHT);


#define CHILD_ID_BAT 9
MyMessage msgBAT(CHILD_ID_BAT, V_VOLTAGE);

unsigned long SLEEP_TIME = 1800000; // Sleep time between reads (in milliseconds)

//Door Variables
boolean tripped_DOOR;
int open_DOOR = 1;
int closed_DOOR = 0;


// DHT-11 TEMP sensor ===============================================
DHT dht;
float lastTemp;
float lastHum;

// timings
unsigned long REBOOT_time;


void setup()
{
  // Initialize library and add callback for incoming messages
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("UBER Mailbox Sensor", "2.0");

  present_SENSORS();
  present_PINMODES();

  dht.setup(HUMIDITY_SENSOR_ANALOG_PIN);

  refresh_SENSORS(); //run to inialize all sensors to gateway!

  read_DOOR();
  read_DHT11();
  read_VOLTS();
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void loop()
{
  unsigned long time_passed = 0;
  read_DHT11();
  read_VOLTS();
  read_DOOR();
  time_passed = millis() - REBOOT_time;
  if (time_passed > 86400000) //how often to examine the sensor analog value
  {
    REBOOT_time = millis();	//update time when sensor value last read
    resetFunc();  //call reset
  }
  // Sleep until interrupt comes in on motion sensor. Send update every 30 minutes.
  Serial.println("***** GOING TO SLEEP *****************");
  gw.sleep(INTERRUPT, CHANGE, SLEEP_TIME);
}


void present_SENSORS() {
  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_DOOR, S_LIGHT);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_BAT, S_POWER);
}


void read_DHT11()
{
  Serial.println("***** BEGIN DHT-11 *****************");
  delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    gw.send(msgTEMP.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }

  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
    lastHum = humidity;
    gw.send(msgHUM.set(humidity, 1));
    Serial.print("H: ");
    Serial.println(humidity);
  }

  Serial.println("***** END DHT-11 *****************");
}


void read_VOLTS() {
  Serial.println("***** BEGIN BATTERY *****************");

  float v = vcc.Read_Volts();
  Serial.print("VCC = ");
  Serial.print(v);
  Serial.println(" Volts");

  float p = vcc.Read_Perc(VccMin, VccMax);
  Serial.print("VCC = ");
  Serial.print(p);
  Serial.println(" %");
  gw.send(msgBAT.set(v, 1));

  Serial.println("***** END BATTERY *****************");

}


void present_PINMODES() {

  pinMode(TEMP_SENSOR_ANALOG_PIN, INPUT);
  pinMode(HUMIDITY_SENSOR_ANALOG_PIN, INPUT);
  pinMode(DIGITAL_INPUT_SENSOR_DOOR, INPUT);

}

void read_DOOR() {

  int open_DOOR = 1;
  int closed_DOOR = 0;

  tripped_DOOR = digitalRead(DIGITAL_INPUT_SENSOR_DOOR);
  delay(300);
  Serial.println(tripped_DOOR);
  if (tripped_DOOR == LOW) {
    Serial.println("***** BEGIN DOOR *****************");
    //Find out what triggered
    gw.send(msgDOOR.set(open_DOOR)); // Send tripped value to gw
  }
  if (tripped_DOOR == HIGH) {
    Serial.println("***** BEGIN DOOR *****************");
    //Find out what triggered
    gw.send(msgDOOR.set(closed_DOOR)); // Send tripped value to gw
  }
  Serial.println("***** END DOOR *****************");
}


void refresh_SENSORS() {
  gw.send(msgTEMP.set(0)); // Send temp to register child
  gw.send(msgDOOR.set(1)); // Send tripped value to gw
  gw.send(msgBAT.set(0));
  gw.send(msgHUM.set(0));
}
