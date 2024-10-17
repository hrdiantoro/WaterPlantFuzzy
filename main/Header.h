#pragma once

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

unsigned long prevMillis = 0;
const long interval = 1000;

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