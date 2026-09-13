#pragma once
// ============================================================
// OTA KONFIGURACE PRO TOVARNI (PROVISIONING) FIRMWARE.
// ============================================================
// Zkopirujte jako "OtaConfig.h" do slozky factory-sw sketch.
//
// DULEZITE: FIRMWARE_TARGET, GITHUB_OWNER, GITHUB_REPOSITORY a
// OTA_ASSET_* MUSI byt IDENTICKE s OtaConfig.h ostreho firmware (viz
// ../template/OtaConfig.example.h) - jinak factory-sw nenajde/neoveri
// spravny Release. FIRMWARE_VERSION a OTA_CHECK_INTERVAL_MS jsou
// zamerne JINE, viz komentare nize.

// --- Identita "verze" tovarniho firmware ---

// Umyslne "0.0.0" - musi byt vzdy nizsi nez jakakoliv realne vydana
// verze, aby factory-sw pri prvnim pripojeni k WiFi okamzite nasel a
// nainstaloval nejnovejsi dostupny Release (viz OtaManager - porovnani
// verzi je vzdy "vysledek > 0", takze se OTA vzdy spusti).
#define FIRMWARE_VERSION "0.0.0"

// Musi presne odpovidat FIRMWARE_TARGET ostreho firmware - factory-sw
// bezi na identickem hardwaru a ma se zmenit prave na tento cil.
#define FIRMWARE_TARGET "TODO-esp8266-nazev-projektu"

// --- GitHub repozitar s Releases (musi odpovidat ostremu firmware) ---
#define GITHUB_OWNER "TODO_GITHUB_OWNER"
#define GITHUB_REPOSITORY "TODO_GITHUB_REPOSITORY"

// Nazvy ocekavanych assetu v GitHub Release - stejne jako v ostrem firmware.
#define OTA_ASSET_FIRMWARE "firmware.bin"
#define OTA_ASSET_METADATA "firmware.json"
#define OTA_ASSET_CHECKSUM "firmware.bin.sha256"

// --- Chovani OTA ---

#define OTA_ENABLED true

// U tovarniho firmware umyslne KRATSI interval nez v ostrem firmware
// (tam typicky 30 min) - technik/zakaznik ceka u zarizeni a sleduje
// Serial Monitor, chceme rychle opakovani pri docasnem vypadku WiFi/GitHubu,
// ne cekat 30 minut.
#define OTA_CHECK_INTERVAL_MS (20UL * 1000UL) // 20 sekund

#define OTA_CONNECT_TIMEOUT_MS 10000UL
#define OTA_DOWNLOAD_TIMEOUT_MS 120000UL

// Stejna bezpecnostni ocekavani jako ostry firmware - tovarni priprava
// neni duvod checksum vyzadovat mene prisne.
#define OTA_REQUIRE_CHECKSUM true

// Tovarni firmware se sam nikdy neboot uje opakovane jako "pending
// validation" (po uspesnem OTA rovnou restartuje do ostreho firmware),
// takze tato hodnota zde nema prakticky vyznam - ponechana pro
// konzistenci s OtaState.
#define OTA_MAX_BOOT_ATTEMPTS 3
