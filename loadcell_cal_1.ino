#include <HX711_ADC.h>
#include <EEPROM.h>

// === Pin HX711 ===
const int HX711_dout = 18;
const int HX711_sck  = 17;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// EEPROM untuk kalibrasi
const int calVal_eepromAddress = 0;

// === Fitur Tare Dengan Tombol ===
const int tareButtonPin = 14;               // Pin tombol tare
unsigned long lastTareTime = 0;             // Untuk debouncing
const unsigned long debounceDelay = 500;    // Delay untuk mencegah bouncing

unsigned long lastPrintTime = 0;            // Waktu terakhir cetak berat
const unsigned long printInterval = 5000;   // Interval cetak 5 detik (5000 ms)

float latestWeightKg = 0.0;                 // Menyimpan berat terbaru

void setup() {
  Serial.begin(9600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  pinMode(tareButtonPin, INPUT_PULLUP);  // Menggunakan internal pull-up resistor

  LoadCell.begin();
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, cek koneksi HX711 ke mikrokontroler!");
    while (1);
  }

  // === Gunakan nilai kalibrasi default = 1.0 (tanpa regresi) ===
  float calFactor = 1.0;
  LoadCell.setCalFactor(calFactor);

  Serial.print("Faktor kalibrasi: ");
  Serial.println(calFactor, 6);

  Serial.println("Startup selesai. Pengukuran dimulai...");

  while (!LoadCell.update());
}

void loop() {
  static boolean newDataReady = false;

  if (LoadCell.update()) newDataReady = true;

  // === Tare dengan tombol ===
  if (digitalRead(tareButtonPin) == LOW) {
    if (millis() - lastTareTime > debounceDelay) {
      Serial.println("Tombol ditekan: melakukan TARE...");
      LoadCell.tareNoDelay();
      lastTareTime = millis();
    }
  }

  if (newDataReady) {
    // Ambil data langsung dari HX711
    float weightKg = LoadCell.getData();

    // Jika berat negatif, set ke 0
    if (weightKg < 0) weightKg = 0;

    latestWeightKg = weightKg;
    newDataReady = false;
  }

  // Cetak berat setiap 5 detik
  if (millis() - lastPrintTime >= printInterval) {
    Serial.print("Berat (update 5 detik): ");
    Serial.print(latestWeightKg, 3);
    Serial.println(" kg");
    lastPrintTime = millis();
  }

  // Status tare
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare selesai");
  }
}
