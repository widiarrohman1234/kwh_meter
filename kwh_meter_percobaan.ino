#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>

String urlCreate = "http://192.168.140.127/kwh-meter-laravel/api/data";
//String urlCreate = "http://192.168.1.119/kwhmeter/api/create.php";
//String urlUpdate = "https://kwh.sidafa.id/api/update.php";

#include <PZEM004Tv30.h>
PZEM004Tv30 pzem(D3, D4); //D3 //D4

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); //jika gagal ganti alamat jadi 0x27

#include <ArduinoJson.h>

#define stop_kontak D5

const unsigned long jeda = 1000;
unsigned long zero  = 0;

#define buzzer D6
int count;

boolean flag = false;

void setup() {
  Serial.begin(115200);

  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  pinMode(stop_kontak, OUTPUT);
  pinMode(buzzer, OUTPUT);

  WiFiManager wm;
  bool res;
  res = wm.autoConnect("AutoConnectkWhMeter", "password"); // password protected ap
  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    Serial.println("successful connection...:)");
  }

  lcd.clear();
}

void loop() {
  //voltase (V)
  float voltage = pzem.voltage();
  if ( !isnan(voltage) ) {
    //    Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");
  } else {
    Serial.println("Error reading voltage");
  }

  //power (W)
  float power = pzem.power();
  if ( !isnan(power) ) {
    //    Serial.print("Power: "); Serial.print(power); Serial.println(" W");
  } else {
    Serial.println("Error reading power");
  }

  //current (A)
  float current = pzem.current();
  if ( !isnan(current) ) {
    //    Serial.print("Current: "); Serial.print(current); Serial.println(" A");
  } else {
    Serial.println("Error reading current");
  }

  //energy (kwh)
  float energy = pzem.energy();
  if ( !isnan(energy) ) {
    //    Serial.print("Energy: "); Serial.print(energy); Serial.println(" kWh");
  } else {
    Serial.println("Error reading energy");
  }

  //frequency (Hz)
  float freq = pzem.frequency();
  if ( !isnan(freq) ) {
    //    Serial.print("Frequency: "); Serial.print(frequency, 1); Serial.println(" Hz");
  } else {
    Serial.println("Error reading frequency");
  }

  //power factor (pf)
  float pf = pzem.pf();
  if ( !isnan(pf) ) {
    //    Serial.print("PF: "); Serial.print(pf); Serial.println(" PF");
  } else {
    Serial.println("Error reading power factor");
  }
  //  Serial.println("------------");

  lcd.setCursor(0, 0);
  //  lcd.print(String() + "Volt:" + voltage);
  //  lcd.setCursor(0, 1);
  lcd.print(String() + "Watt:" + power); //power

  if (millis() - zero >= jeda) {
    HTTPClient http;
    http.begin(urlCreate);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String httpRequestData = String() + "voltage=" + voltage + "&current=" + current +
                             "&power=" + power + "&energy=" + energy +
                             "&freq=" + freq + "&pf=" + pf + "&mac_address=" + WiFi.macAddress();

    Serial.println(httpRequestData);
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.print("Success POST data");
      Serial.print("HTTP ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);

      // ArduinoJSon
      //      StaticJsonDocument<64> doc;
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      String status = doc["status"];
      String relay = doc["relay"];
      float batas_kwh = doc["batas_kwh"].as<float>();
      float sisa_kwh = batas_kwh - energy;

      Serial.println("Status: " + status);
      Serial.println("Relay: " + relay);
      Serial.println("Batas KWH: " + String(batas_kwh));
      Serial.println("Sisa KWH: " + String(sisa_kwh));

      if (status == "TRUE") {

        //Stop Kontak
        if (relay == "ON") {
          digitalWrite(stop_kontak, HIGH);
        } else if (relay == "OFF") {
          digitalWrite(stop_kontak, LOW);
        }

        //Sisa KWH 0
        if (sisa_kwh <= 0) {
          lcd.setCursor(0, 1);
          lcd.print(String() + "KWH Habis: " + sisa_kwh + "     ");
          count++;
          if (count == 1) {
            digitalWrite(buzzer, HIGH);
          } if (count == 2) {
            digitalWrite(buzzer, LOW);
            count = 0;
          }
          Serial.println("count: " + count);
          // if (flag == false) {
          //   String path = url2 + "?relay=OFF";
          //   http.begin(path);
          //   int httpResponseCode = http.GET();
          //   if (httpResponseCode > 0) {
          //     Serial.print("HTTP ");
          //     Serial.println(httpResponseCode);
          //     String payload = http.getString();
          //     Serial.println();
          //     Serial.println(payload);
          //   }
          //   else {
          //     Serial.print("Error code: ");
          //     Serial.println(httpResponseCode);
          //     Serial.println(":-(");
          //   }
          //   flag = true;
          // }
        }else if (sisa_kwh > 0) {
          lcd.setCursor(0, 1);
          lcd.print(String() + "KWH Tersedia: " + sisa_kwh + "    ");
          digitalWrite(buzzer, LOW);
          flag = false;
        }

      Serial.println("-------------------- ");
      } else if (status == "FALSE") {
        Serial.println("Disconnected Database");
      }

    }
    else {
      Serial.print("Error code POST data: ");
      Serial.println(httpResponseCode);
      Serial.println(": -(");
    }

    zero = millis();
  }
}
