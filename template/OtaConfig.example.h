#pragma once
// ============================================================
// OTA KONFIGURACE - jedine misto pro nastaveni OTA systemu.
// ============================================================
// Zkopirujte jako "OtaConfig.h" do slozky sve sketch a vyplnte hodnoty
// nize (FIRMWARE_TARGET, GITHUB_OWNER, GITHUB_REPOSITORY). Zadne tajne
// udaje (GitHub token, hesla, klice) sem NEPATRI - repozitar musi byt
// verejny, systemu pouziva anonymni GitHub REST API.

// --- Verze a identita tohoto firmware ---

// Zvysujte pri kazde zmene, kterou chcete distribuovat pres OTA.
// Pouziva se Semantic Versioning (MAJOR.MINOR.PATCH), viz OtaVersion.h.
#define FIRMWARE_VERSION "1.0.0"

// Identifikuje HW/SW variantu tohoto firmware. OTA odmitne nainstalovat
// release, jehoz firmware.json obsahuje jiny "target" (viz README.md,
// sekce "Kompatibilita hardwaru"). Zvolte neco jednoznacneho, napr.
// "<cip>-<deska>-<nazev-projektu>".
#define FIRMWARE_TARGET "TODO-esp8266-nazev-projektu"

// --- GitHub repozitar s Releases ---
// TODO: vyplnte pred prvnim pouzitim.
#define GITHUB_OWNER "TODO_GITHUB_OWNER"
#define GITHUB_REPOSITORY "TODO_GITHUB_REPOSITORY"

// Nazvy ocekavanych assetu v GitHub Release.
// firmware.json je volitelny, ale doporuceny - pokud je pritomen, pouzije
// se pro kontrolu "target" a SHA-256 bez nutnosti samostatneho .sha256 souboru.
#define OTA_ASSET_FIRMWARE "firmware.bin"
#define OTA_ASSET_METADATA "firmware.json"
#define OTA_ASSET_CHECKSUM "firmware.bin.sha256"

// --- Chovani OTA ---

// Globalni vypinac - pri false OtaManager::handle() nic nedela.
#define OTA_ENABLED true

// Jak casto (ms) se kontroluje GitHub Releases na novou verzi.
// 30 minut je rozumny vychozi interval pro zarizeni bezici bez dohledu;
// pri tovarni/provisioning fazi (technik ceka u zarizeni) zvazte kratsi
// hodnotu, napr. 20 sekund - viz README.md, sekce "Varianty nasazeni".
#define OTA_CHECK_INTERVAL_MS (30UL * 60UL * 1000UL) // 30 minut

// Timeouty sitovych operaci (ms).
#define OTA_CONNECT_TIMEOUT_MS 10000UL
#define OTA_DOWNLOAD_TIMEOUT_MS 120000UL // cely stahovaci cyklus firmware.bin

// Vyzadovat platny SHA-256 checksum, jinak OTA zrusit (fail-closed).
// Vychozi true - viz README.md, sekce "TLS a integrita firmware": HTTPS
// spojeni pro OTA typicky pouziva BearSSL::WiFiClientSecure::setInsecure()
// (zadne cert pinning), takze tento checksum je skutecnou, ne jen
// volitelnou pojistkou integrity stazeneho firmware.
#define OTA_REQUIRE_CHECKSUM true

// Kolik po sobe jdoucich neuspesnych bootu noveho (jeste nevalidovaneho)
// firmware je povoleno, nez OtaManager provede automaticky rollback na
// /ota/last_good.bin.
#define OTA_MAX_BOOT_ATTEMPTS 3
