#include <Fuzzy.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

Fuzzy *fuzzy = new Fuzzy();
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define rainSensorPin A0
#define soilMoisturePin A1
#define EN_A 6
#define IN_1 2
#define IN_2 3

float rainSensorValue;
float soilMoistureValue;

int pumpSpeed;

// Fungsi untuk mengontrol pompa
void controlPump(int speed) {
  analogWrite(EN_A, speed);  // Mengatur kecepatan pompa
  if (speed > 0) {
    digitalWrite(IN_1, HIGH);
    digitalWrite(IN_2, LOW);
  } else {
    digitalWrite(IN_1, LOW);
    digitalWrite(IN_2, LOW);  // Mematikan pompa
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(EN_A, OUTPUT);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(" SISTEM KENDALI");
  lcd.setCursor(5, 1);
  lcd.print("CERDAS");
  delay(3000);
  lcd.clear();

  // Input Rain Sensor
  FuzzyInput *rainSensor = new FuzzyInput(1);
  FuzzySet *noRain = new FuzzySet(0, 0, 20, 50);         // Tidak ada hujan
  FuzzySet *lightRain = new FuzzySet(30, 50, 60, 70);    // Gerimis
  FuzzySet *heavyRain = new FuzzySet(60, 80, 100, 100);  // Hujan lebat
  rainSensor->addFuzzySet(noRain);
  rainSensor->addFuzzySet(lightRain);
  rainSensor->addFuzzySet(heavyRain);
  fuzzy->addFuzzyInput(rainSensor);

  // Input Soil Moisture Sensor
  FuzzyInput *soilMoisture = new FuzzyInput(2);
  FuzzySet *dry = new FuzzySet(0, 0, 10, 25);      // Kering
  FuzzySet *moist = new FuzzySet(20, 30, 40, 60);  // Lembab
  FuzzySet *wet = new FuzzySet(70, 80, 100, 100);  // Basah
  soilMoisture->addFuzzySet(dry);
  soilMoisture->addFuzzySet(moist);
  soilMoisture->addFuzzySet(wet);
  fuzzy->addFuzzyInput(soilMoisture);

  // Output Pompa
  FuzzyOutput *pump = new FuzzyOutput(1);
  FuzzySet *off = new FuzzySet(0, 0, 10, 30);       // Mati
  FuzzySet *low = new FuzzySet(10, 30, 40, 60);     // Low
  FuzzySet *medium = new FuzzySet(40, 60, 70, 90);  // Medium
  FuzzySet *high = new FuzzySet(70, 90, 100, 100);  // High
  pump->addFuzzySet(off);
  pump->addFuzzySet(low);
  pump->addFuzzySet(medium);
  pump->addFuzzySet(high);
  fuzzy->addFuzzyOutput(pump);

  // Fuzzy Rules 1
  FuzzyRuleAntecedent *ifNoRainAndDry = new FuzzyRuleAntecedent();
  ifNoRainAndDry->joinWithAND(noRain, dry);
  FuzzyRuleConsequent *thenPumpHigh = new FuzzyRuleConsequent();
  thenPumpHigh->addOutput(high);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, ifNoRainAndDry, thenPumpHigh);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // Fuzzy Rules 2
  FuzzyRuleAntecedent *ifLightRainAndDry = new FuzzyRuleAntecedent();
  ifLightRainAndDry->joinWithAND(lightRain, dry);
  FuzzyRuleConsequent *thenPumpMedium = new FuzzyRuleConsequent();
  thenPumpMedium->addOutput(medium);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, ifLightRainAndDry, thenPumpMedium);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // Fuzzy Rules 3
  FuzzyRuleAntecedent *ifHeavyRainAndMoist = new FuzzyRuleAntecedent();
  ifHeavyRainAndMoist->joinWithAND(heavyRain, moist);
  FuzzyRuleConsequent *thenPumpOff = new FuzzyRuleConsequent();
  thenPumpOff->addOutput(off);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, ifHeavyRainAndMoist, thenPumpOff);
  fuzzy->addFuzzyRule(fuzzyRule3);

  // Fuzzy Rules 4
  FuzzyRuleAntecedent *ifHeavyRainAndWet = new FuzzyRuleAntecedent();
  ifHeavyRainAndWet->joinWithAND(heavyRain, wet);
  FuzzyRuleConsequent *thenPumpOffForHeavyRainAndWet = new FuzzyRuleConsequent();
  thenPumpOffForHeavyRainAndWet->addOutput(off);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, ifHeavyRainAndWet, thenPumpOffForHeavyRainAndWet);
  fuzzy->addFuzzyRule(fuzzyRule4);
}

void loop() {
  rainSensorValue = (1023 - analogRead(rainSensorPin)) / 10.24;      // Dibuat agar menjadi prosentase
  soilMoistureValue = (1023 - analogRead(soilMoisturePin)) / 10.24;  // Dibuat agar menjadi prosentase

  // Validasi pembacaan sensor
  if (rainSensorValue < 0 || rainSensorValue > 100) {
    rainSensorValue = 0; // Set default jika ada kesalahan pembacaan
  }
  if (soilMoistureValue < 0 || soilMoistureValue > 100) {
    soilMoistureValue = 0; // Set default jika ada kesalahan pembacaan
  }

  fuzzy->setInput(1, rainSensorValue);
  fuzzy->setInput(2, soilMoistureValue);

  fuzzy->fuzzify();

  pumpSpeed = fuzzy->defuzzify(1);

  int pwmValue = map(pumpSpeed, 0, 100, 0, 255);

  // Kontrol pompa berdasarkan kecepatan
  controlPump(pwmValue);

  // Menampilkan nilai pada LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moisture: ");
  lcd.print(soilMoistureValue);
  lcd.setCursor(0, 1);
  lcd.print("Rain: ");
  lcd.print(rainSensorValue);

  // Serial print sensor dan status pompa
  Serial.print("Rain Sensor: ");
  Serial.print(rainSensorValue);
  Serial.print("% - Status: ");
  if (rainSensorValue <= 50) {
    Serial.print("No Rain");
  } else if (rainSensorValue > 50 && rainSensorValue <= 70) {
    Serial.print("Light Rain");
  } else {
    Serial.print("Heavy Rain");
  }

  Serial.print(" | Soil Moisture: ");
  Serial.print(soilMoistureValue);
  Serial.print("% - Status: ");
  if (soilMoistureValue <= 25) {
    Serial.print("Dry");
  } else if (soilMoistureValue > 25 && soilMoistureValue <= 60) {
    Serial.print("Moist");
  } else {
    Serial.print("Wet");
  }

  Serial.print(" | Pump Speed: ");
  Serial.print(pumpSpeed);
  Serial.print(" - Status: ");
  if (pumpSpeed == 0) {
    Serial.println("Off");
  } else if (pumpSpeed > 0 && pumpSpeed <= 60) {
    Serial.println("Low");
  } else if (pumpSpeed > 60 && pumpSpeed <= 90) {
    Serial.println("Medium");
  } else {
    Serial.println("High");
  }

  delay(1000);
}
