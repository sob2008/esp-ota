// ============================================================
// TOVARNI (PROVISIONING) FIRMWARE - OBECNA SABLONA
// ============================================================
// Nahravat POUZE pres USB na nove/vracene kusy pred expedici, MISTO
// ostreho firmware. Jediny ucel: pripojit zarizeni k WiFi zakaznika/
// technika a hned poté si samo stahnout a nainstalovat nejnovejsi ostry
// firmware z GitHub Releases - pouziva presne stejny OTA klient
// (OtaManager/OtaState/OtaVersion/Sha256) jako ostry firmware, jen
// zkopirovany do teto slozky (Arduino kompiluje kazdy sketch/slozku
// zvlast, cross-folder #include neni mozny).
//
// Po uspesne instalaci se zarizeni samo restartuje a dal uz bezi jako
// ostry firmware - tento sketch se tim prepise a jiz nikdy nebezi znovu
// (dokud by nekdo rucne nenahral factory-sw pres USB podruhe, napr. pri
// vraceni/reklamaci).
//
// PRIZPUSOBTE PRO SVUJ PROJEKT:
// - Pokud ma vase zarizeni displej/LED, pridejte OtaManager::setStatusCallback()
//   (viz OtaManager.h) a zobrazujte prubeh - viz README.md v tomto repozitari,
//   sekce "factory-sw", pro konkretni priklad (meteostanice se 2 displeji).
// - Pokud vase ostre firmware potrebuje od zakaznika/technika dalsi udaj
//   pri prvnim nastaveni (napr. polohu, jazyk, jmeno zarizeni...), pridejte
//   dalsi WiFiManagerParameter do stejneho portalu (viz komentar u
//   "wm.addParameter" nize) a ulozte ho do LittleFS (vlastni modul, stejny
//   vzor jako OtaState - write-temp-then-rename).
// - AP_NAME nize zmente na nazev vhodny pro vas produkt.

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#include "OtaConfig.h"
#include "OtaState.h"
#include "OtaManager.h"

// TODO: prizpusobte nazev WiFi site, kterou zarizeni vytvori pro prvni
// nastaveni (zakaznik/technik se na ni pripoji telefonem/PC).
static const char* AP_NAME = "Provisioning_AP";

// Volitelny hook pro zobrazeni stavu behem stahovani/flashovani - vychozi
// implementace jen loguje na Serial. Pokud mate displej, prepiste na
// kresleni (viz komentar na zacatku souboru).
void otaStatusCallback(const String& line1, const String& line2) {
  Serial.print("[OTA-STATUS] ");
  Serial.print(line1);
  Serial.print(" ");
  Serial.println(line2);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("================================================");
  Serial.println("   TOVARNI (PROVISIONING) FIRMWARE");
  Serial.println("================================================");
  Serial.println("Ucel: pripoji zarizeni k WiFi a rovnou nainstaluje");
  Serial.println("aktualni ostry firmware z GitHub Releases.");
  Serial.print("Cilova platforma (FIRMWARE_TARGET): ");
  Serial.println(FIRMWARE_TARGET);
  Serial.println();

  // LittleFS + perzistentni OTA stav. Na zcela novem/prave smazanem
  // zarizeni (plny erase_flash pred zapisem factory-sw) je tohle vzdy
  // cisty start.
  OtaState::begin();
  OtaManager::setStatusCallback(otaStatusCallback);
  OtaManager::begin();

  Serial.print("KROK 1: Pripojte se na WiFi sit: ");
  Serial.println(AP_NAME);
  Serial.println("        Pak v prohlizeci otevrete: 192.168.4.1");
  Serial.println("        a vyberte domaci WiFi sit.");
  Serial.println();

  // Stejny vzor jako ostry firmware by mel pouzivat: pri vyprseni portalu
  // radeji cely restart, nez opakovane volat autoConnect() na tomtez
  // WiFiManager objektu (neni to overene chovani).
  WiFiManager wm;

  // Sem pripadne pridejte dalsi WiFiManagerParameter (viz komentar na
  // zacatku souboru), napr.:
  //   WiFiManagerParameter customParam("id", "Popisek", "vychozi", 40);
  //   wm.addParameter(&customParam);

  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect(AP_NAME)) {
    Serial.println("Konfiguracni portal WiFi vyprsel (180s), restartuji...");
    delay(2000);
    ESP.restart();
  }

  Serial.print("WiFi pripojeno. IP adresa: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Sem pripadne zpracujte hodnoty dalsich pridanych parametru (viz vyse) -
  // ulozte je do vlastniho perzistentniho modulu, stejny vzor jako OtaState.

  Serial.println("KROK 2: Stahuji a instaluji aktualni verzi softwaru...");
  Serial.println("        (podrobny prubeh viz [OTA] hlasky nize)");
  Serial.println();
}

void loop() {
  // OtaManager::handle() pri uspesne instalaci sam zavola ESP.restart() -
  // od tohoto okamziku uz dal bezi ostry firmware a tento sketch se
  // nevraci. Pokud se sem loop() vrati, aktualizace se (zatim) nepovedla
  // (napr. GitHub docasne nedostupny, nebo zadny kompatibilni Release
  // jeste neexistuje) - OTA_CHECK_INTERVAL_MS v tomto OtaConfig.h je
  // umyslne kratky (viz OtaConfig.example.h), takze se to zkusi znovu za
  // chvili. Presny duvod vidite v Serial Monitoru (hlasky "[OTA] ...").
  OtaManager::handle();
  delay(200);
}
