# ota-esp

Znovupoužitelný OTA (over-the-air) systém pro ESP8266/Arduino, aktualizující firmware
z **GitHub Releases**. Vytažen a zobecněn z projektu `wather-station` (meteostanice), kde
běží v produkci a byl několikrát revidován a opravován na základě reálného testování na
hardwaru. Tady je popsaný tak, aby šel implementovat i do jiných ESP projektů.

Tento balíček **není knihovna ke stažení** - je to sada zdrojových souborů k ručnímu
zkopírování do vaší sketch složky + dokumentace k tomu, jak je zapojit a proč jsou
navržené tak, jak jsou.

**Používáte Claude Code (nebo jiného AI agenta)?** Stačí mu dát odkaz na tento repozitář
a ať postupuje podle [`AGENT_PROMPT.md`](AGENT_PROMPT.md) - ten ho navede, aby nejdřív
ověřil, jestli OTA na vašem konkrétním hardwaru/frameworku dává smysl, a pak implementoval
kompletní systém včetně továrního/provisioning firmware (`factory-template/`), přesně jak
je popsáno níže.

## Co to dělá

Zařízení si samo, na pozadí, kontroluje GitHub Releases vašeho repozitáře. Když najde
novější kompatibilní verzi, bezpečně ji stáhne, ověří (velikost, cíl/target, SHA-256
checksum) a nainstaluje. Pokud se nová verze po restartu ukáže jako nefunkční
(crash/boot-loop), automaticky se vrátí na poslední funkční verzi. GitHub se používá
**jen jako úložiště releasů** - žádný automatický build ani release nevzniká sám, dokud
to sami nespustíte pushnutím gitového tagu.

**Zásadní vlastnost:** OTA nikdy nesmí rozbít funkční zařízení. Každé rozhodnutí v tomto
systému (pořadí operací, kdy se co maže/zapisuje, co se stane při chybě) je odvozeno z
téhle jedné věty - viz "Klíčová rozhodnutí" níže.

## Rychlý start - integrace do nového projektu

1. Zkopírujte do složky své sketch (vedle `.ino`):
   ```
   template/OtaVersion.h    template/OtaVersion.cpp
   template/Sha256.h        template/Sha256.cpp
   template/OtaState.h      template/OtaState.cpp
   template/OtaManager.h    template/OtaManager.cpp
   ```
   (Arduino kompiluje všechny `.h`/`.cpp` v sketch složce automaticky spolu s `.ino` -
   nejde o samostatnou knihovnu, žádná instalace přes Library Manager není potřeba.)

2. Zkopírujte `template/OtaConfig.example.h` jako `OtaConfig.h` do stejné složky a
   vyplňte `FIRMWARE_TARGET`, `GITHUB_OWNER`, `GITHUB_REPOSITORY` (komentáře v souboru
   vysvětlují každou hodnotu).

3. V `.ino` přidejte (přesné pořadí je důležité, viz "Klíčová rozhodnutí"):

   ```cpp
   #include "OtaConfig.h"
   #include "OtaState.h"
   #include "OtaManager.h"

   void setup() {
     // Pripojit LittleFS + nacist perzistentni stav - PRED pripojenim WiFi,
     // aby pripadny rollback nezavisel na siti.
     OtaState::begin();

     // Volitelne - zobrazit stav stahovani na displeji/LED apod.
     OtaManager::setStatusCallback([](const String& l1, const String& l2) {
       Serial.println(l1 + " " + l2);
     });

     // Rozhodne o pripadnem rollbacku z minuleho OTA cyklu.
     OtaManager::begin();

     // ... tady pripojte WiFi, inicializujte hardware ...

     // ... tady provedte alespon jeden cyklus hlavni funkce zarizeni ...

     // Potvrdit, ze firmware funguje (jinak po par neuspesnych bootech
     // dojde k automatickemu rollbacku).
     OtaManager::notifyApplicationHealthy();
   }

   void loop() {
     // Neblokujici mimo aktivni kontrolu (viz OTA_CHECK_INTERVAL_MS).
     OtaManager::handle();

     // ... zbytek vaseho loop() ...
   }
   ```

4. Vyžaduje knihovny `ArduinoJson` (v7 API) a ESP8266 core s `LittleFS`/`ESP8266HTTPClient`/
   `WiFiClientSecureBearSSL`/`Updater` (součást ESP8266 Arduino core, nic navíc neinstalujte).

5. Zkopírujte `workflows/release.yml.template` a `scripts/release.ps1.template` (viz jejich
   vlastní komentáře pro úpravy) - řeší automatický build+publikaci Release po pushnutí tagu.

6. Ověřte reálnou kompilací: `arduino-cli compile --fqbn esp8266:esp8266:d1_mini --warnings all <sketch-slozka>`.

## Tovární (provisioning) firmware - `factory-template/`

Druhá polovina systému: samostatný sketch, který se nahraje **místo** ostrého firmware na
nové/vrácené kusy před expedicí. Připojí zařízení k WiFi zákazníka/technika a hned poté si
samo stáhne a nainstaluje nejnovější release - používá stejný OTA klient jako ostrý
firmware, jen s `FIRMWARE_VERSION = "0.0.0"` (vždy nižší než reálný release, takže OTA se
spustí okamžitě) a kratším kontrolním intervalem (technik čeká u zařízení).

1. Vytvořte novou složku/sketch (např. `factory-<projekt>`), zkopírujte do ní
   `factory-template/factory-sw.ino` a znovu `OtaVersion/Sha256/OtaState/OtaManager`
   (stejné soubory jako výše - Arduino kompiluje sketch složky zvlášť).
2. Zkopírujte `factory-template/OtaConfig.example.h` jako `OtaConfig.h` - `FIRMWARE_TARGET`/
   `GITHUB_OWNER`/`GITHUB_REPOSITORY` musí být **identické** s ostrým firmware.
3. Přizpůsobte `AP_NAME` a případně přidejte zobrazování stavu na displeji/LED
   (`OtaManager::setStatusCallback`) nebo další pole do WiFi portálu, pokud vaše ostré
   firmware potřebuje od zákazníka další údaj při prvním nastavení (komentáře v
   `factory-sw.ino` ukazují kam).
4. Po úspěšné instalaci se zařízení samo restartuje a běží dál jako ostrý firmware -
   tovární sketch se tím přepíše a už nikdy neběží znovu (dokud by se přes USB nenahrál
   podruhé, např. při reklamaci).

### Flashování - `factory-template/!flash/`

Univerzální flash skript (ESP8266 i ESP32, žádné projektové závislosti) - zkopírujte
celou složku `!flash/` vedle továrního sketch. Zkompilovaný `.bin` (`arduino-cli compile
--export-binaries` nebo Arduino IDE `Sketch -> Export compiled Binary`) se dá do
`!flash/bin/` a spustí se skript ze složky `!flash`:

- Windows: `python flash.py`
- Linux/macOS: `./flash.sh`

Skript sám zkontroluje/nabídne doinstalovat `esptool`/`pyserial` (Windows) nebo
`python3`/`pip`/`esptool`/oprávnění k sériovému portu (Linux/macOS), najde USB
zařízení a typ čipu (ESP8266/ESP32 - podle toho zvolí správné adresy pro `write_flash`),
smaže flash a nahraje firmware. Pokud je ve `bin/` víc `.bin` souborů (např. tovární i
ostrý firmware najednou), nabídne výběr, který nahrát. Samostatná volba `--monitor`
otevře sériový monitor (115200 baud), `--info` zobrazí informace o čipu, `--erase` jen
smaže flash.

## Architektura

| Modul | Zavislosti | Ucel |
|---|---|---|
| `OtaVersion.h/.cpp` | zadne (Arduino-nezavisly) | Parsuje `vMAJOR.MINOR.PATCH` a porovnava verze **numericky** (`1.10.0 > 1.9.0`), ne lexikograficky. |
| `Sha256.h/.cpp` | zadne (Arduino-nezavisly) | Samostatna implementace SHA-256 (FIPS 180-4), streamovaci API - hashuje firmware po castech behem stahovani. |
| `OtaState.h/.cpp` | LittleFS, ArduinoJson | Perzistentni stav na `/ota/`: `state.json` (pending-validation flag, pocitadlo pokusu o boot, posledni neuspesna verze), `candidate.bin`, `last_good.bin`. |
| `OtaManager.h/.cpp` | vse vyse + HTTPClient, WiFiClientSecureBearSSL | Orchestrace: kontrola GitHub Releases, stahovani, overeni, zapis, rollback rozhodnuti. |
| `OtaConfig.h` | - | Vsechny konstanty na jednom miste. Zadne secrets. |

`OtaVersion` a `Sha256` jsou napsane bez zavislosti na `Arduino.h` **umyslne** - dají se
zkompilovat a otestovat běžným `g++` na počítači (viz `tests/`), bez nutnosti mít ESP8266
core nebo hardware. `OtaState`/`OtaManager` na LittleFS/HTTPClient závisí a jde je ověřit
jen reálnou kompilací proti ESP8266 core (arduino-cli/Arduino IDE) - ne spuštěním na hostu.

## Formát GitHub Release

```
Tag:  v1.2.0        (musi byt parsovatelny jako MAJOR.MINOR.PATCH, volitelne s "v")

Assety:
  firmware.bin          - povinny, zkompilovany .bin
  firmware.json         - doporuceny (viz nize)
  firmware.bin.sha256   - volitelny fallback, pokud firmware.json chybi
```

`firmware.json` (doporučeno - umožňuje kontrolu cíle/target a SHA-256 bez nutnosti
samostatného `.sha256` souboru):

```json
{
  "version": "1.2.0",
  "target": "esp8266-d1mini-nazev-projektu",
  "firmware_size": 512430,
  "sha256": "1a2b3c...64 hex znaku..."
}
```

`firmware.bin.sha256` (pokud `firmware.json` chybí) obsahuje buď samotný 64znakový hex
SHA-256, nebo formát `sha256sum` (`hash  firmware.bin`).

## Klíčová rozhodnutí (a proč)

### 1. ESP8266 nemá nativní A/B rollback jako ESP32

**Tohle je nejdůležitější věc k pochopení před jakoukoliv úpravou `Update.begin()`/
`Update.end()` volání.** ESP32 (ESP-IDF) má skutečné, nezávislé A/B partition s okamžitým
přepnutím bootovací partition (`esp_ota_set_boot_partition`) a nativní rollback
(`esp_ota_mark_app_invalid_rollback_and_reboot`). ESP8266 Arduino core **nic takového
nemá** - `Update`/`eboot` je "ping-pong" mechanismus:

- Nový firmware se zapíše do volné (aktuálně nepoužívané) aplikační oblasti flash -
  `ESP.getFreeSketchSpace()` je přesně ta druhá oblast, adresově spočítaná tak, aby se
  nikdy nepřekrývala s běžícím firmware (ověřeno přímo ve zdrojácích ESP8266 core,
  `Updater.cpp`/`Esp.cpp` - `Update.begin(ESP.getFreeSketchSpace(), U_FLASH)` je
  bezpečná horní mez, ne odhad).
- Zápis nového firmware tedy **nikdy nepřepisuje** to, co právě běží - výpadek napájení
  během stahování je bezpečný.
- Teprve při dalším restartu malý bootloader (`eboot`) tuto oblast zkopíruje na místo
  aktuálně běžícího firmware. Tahle kopie trvá ~1s a je **jediné reziduální riziko**, které
  nelze z úrovně sketch odstranit (šlo by jen náhradou `eboot` samotného).
- Protože nativní rollback neexistuje, je implementován na aplikační úrovni v `OtaState`:
  před dokončením OTA se stažený firmware zároveň uloží do `/ota/candidate.bin`. Po
  úspěšném boot-ověření (viz níže) se povýší na `/ota/last_good.bin`. Rollback = přeflashovat
  `last_good.bin` stejnou cestou jako běžné OTA (jen zdroj bajtů je LittleFS, ne síť).

**Pokud portujete na ESP32**: tahle celá `OtaState`/candidate/last_good mechanika je
pravděpodobně zbytečná - ESP-IDF/Arduino-ESP32 nabízí nativní rollback
(`esp_ota_mark_app_valid_cancel_rollback()` po `Update.begin()`/`Update.end()`), který
dělá totéž jednodušeji a bez potřeby druhé kopie firmware na flash. Přepočítejte i
velikostní úvahy - ESP32 partition tabulka je explicitní (`partitions.csv`), ne
odvozená z `eesz` board-option jako u ESP8266.

### 2. TLS `setInsecure()` + POVINNÝ SHA-256 checksum

HTTPS spojení (GitHub API i stažení `firmware.bin`) používá
`BearSSL::WiFiClientSecure::setInsecure()` - **žádné ověřování TLS certifikátu**. Tohle
je vědomé rozhodnutí, ne přehlédnutí: reálné TLS pinning vyžaduje zabudovat správný
kořenový certifikát do firmware, a špatný/neaktuální certifikát by OTA trvale a tiše
rozbil - horší důsledek než dokumentované riziko `setInsecure()`. GitHub navíc používá
**různé certifikační autority** pro `api.github.com` a CDN, na které se přesměrovává
stažení assetu (`objects.githubusercontent.com`) - je potřeba připnout obě, ne jednu.

Jako kompenzaci je **SHA-256 kontrola integrity povinná** (`OTA_REQUIRE_CHECKSUM = true`)
- bez platného checksumu ve `firmware.json` nebo `firmware.bin.sha256` se OTA vždy zruší.
To je hlavní ochrana proti poškozenému/změněnému obsahu při přenosu.

Pokud chcete přidat skutečné TLS pinning: `WiFiClientSecureBearSSL.h` podporuje
`setTrustAnchors()` s vlastním `BearSSL::X509List` - potřebujete kořenové certifikáty
z **důvěryhodného** zdroje (ne z prostředí, kde nemůžete ověřit, že HTTPS spojení není
odposlouchávané/podvržené).

### 3. Kontrola velikosti - dvojitá, nikdy jen podle Content-Length

Před stažením se porovná deklarovaná velikost (z `firmware.json`, nebo velikosti GitHub
asset záznamu) s `ESP.getFreeSketchSpace()`. Pokud je firmware větší, OTA se zruší **bez
jakéhokoli zápisu do flash**. Protože `Content-Length` HTTP hlavička nemusí být dostupná
nebo spolehlivá, se navíc při streamovaném zápisu kontroluje návratový kód
`Update.write()` - jakmile by zápis překročil skutečně volné místo, přeruší se okamžitě.
Firmware nikdy nespoléhá jen na jeden z těchto dvou mechanismů.

### 4. Ochrana proti nekonečné OTA smyčce

Pokud verze `X.Y.Z` po instalaci opakovaně nedoběhne k `notifyApplicationHealthy()` (víc
než `OTA_MAX_BOOT_ATTEMPTS`-krát po sobě, výchozí 3), provede se rollback **a** verze
`X.Y.Z` se zapíše jako "last_failed_version" do `state.json`. Při dalších pravidelných
kontrolách se **stejná** verze znovu nezkouší, dokud nevyjde novější release. Bez tohoto
by zařízení mohlo dokola stahovat a instalovat tu samou vadnou verzi navěky.

**Důležité omezení:** `last_good.bin` existuje až od **druhé** úspěšné OTA aktualizace -
úplně první OTA (z firmware nahraného přes USB/výrobu) nemá co zálohovat. Pokud by první
OTA byla vadná, automatický rollback není možný - nutný ruční zásah (USB reflash).

### 5. Detekce "stale" pending-validation stavu (race condition při výpadku napájení)

Stav `pending_validation=true` se do `state.json` zapisuje **před** `Update.end()`, ne
po něm - jakmile `Update.end()` jednou vrátí `true`, `eboot` je "nabitý" na swap při
příštím restartu, i kdyby `ESP.restart()` nikdy nebyl zavolán kvůli výpadku napájení.
Kdyby se pending stav zapsal až po `Update.end()`, existovalo by okno, kdy `eboot` při
dalším bootu nový firmware nahraje, ale sledování stavu o tom neví a rollback ochrana by
se neaktivovala.

Navíc se `pending_validation` porovnává s verzí **skutečně běžícího** firmware
(compile-time `FIRMWARE_VERSION`) - pokud dojde k výpadku napájení přesně mezi zápisem
stavu a provedením `eboot` swapu, další boot to sám rozpozná (verze v `state.json`
neodpovídá tomu, co skutečně běží) a stav bezpečně vyčistí, místo falešné validace nebo
falešného rollbacku.

## Release workflow (GitHub Actions + skript)

`workflows/release.yml.template` (zkopírovat jako `<projekt>/.github/workflows/release.yml`)
se spouští **výhradně ručním pushnutím tagu** `vX.Y.Z` - žádný automatický build při běžném
push do `main`. Ověří, že tag odpovídá `FIRMWARE_VERSION` v `OtaConfig.h` (při neshodě
selže bez publikace), zkompiluje firmware, spočítá SHA-256, vygeneruje `firmware.json` a
vytvoří GitHub Release se všemi třemi assety.

`scripts/release.ps1.template` (zkopírovat jako `<projekt>/scripts/release.ps1`)
zjednodušuje ruční kroky na jeden příkaz: bump verze v `OtaConfig.h` → commit → tag → (po
potvrzení) push, čímž se spustí výše uvedený workflow.

## Testování

- `tests/test_sha256.cpp`, `tests/test_ota_version.cpp` - host-side testy (běžný `g++`,
  žádný ESP8266 core potřeba):
  ```
  g++ -std=c++17 -Wall -Wextra -o test_sha256.exe tests/test_sha256.cpp template/Sha256.cpp
  g++ -std=c++17 -Wall -Wextra -o test_ota_version.exe tests/test_ota_version.cpp template/OtaVersion.cpp
  ```
- Zbytek (`OtaState`, `OtaManager`) ověřujte reálnou kompilací proti ESP8266 core:
  `arduino-cli compile --fqbn esp8266:esp8266:d1_mini --warnings all <sketch-slozka>`.
- **Co nelze ověřit bez fyzického zařízení**: skutečný WiFi/HTTPS provoz, reálný `eboot`
  swap při restartu, chování watchdogu při dlouhém stahování, celý rollback cyklus
  (vyžaduje záměrně vadný Release a sledování přes několik rebootů).

## Konfigurace (`OtaConfig.h`)

Vše na jednom místě, žádné secrets:

- `FIRMWARE_VERSION` - Semantic Versioning, zvyšovat při každé distribuované změně.
- `FIRMWARE_TARGET` - identifikuje HW/SW variantu; OTA odmítne firmware s jiným targetem
  ve `firmware.json` (chrání před nahráním firmware určeného pro jiný hardware).
- `GITHUB_OWNER`/`GITHUB_REPOSITORY` - veřejný repozitář, anonymní GitHub REST API
  (limit 60 požadavků/hod na IP - při výchozím 30min intervalu ~2/hod).
- `OTA_CHECK_INTERVAL_MS` - výchozí 30 minut pro běžný provoz; pro tovární/provisioning
  fázi (technik čeká u zařízení) zvažte mnohem kratší hodnotu (např. 20 s) v samostatné
  konfiguraci pro tu variantu firmware.
- `OTA_REQUIRE_CHECKSUM`, `OTA_MAX_BOOT_ATTEMPTS`, timeouty - viz komentáře v souboru.

## Chování v okrajových situacích

| Situace | Chování |
|---|---|
| Není internet/WiFi | OTA kontrola se přeskočí, zařízení pokračuje normálně |
| GitHub nedostupný | `[OTA] GitHub unavailable`, pokračuje normálně |
| Release neexistuje / bez `firmware.bin` | přeskočí se, pokračuje normálně |
| Release má stejnou/starší verzi | `[OTA] No update available` - nikdy automatický downgrade |
| Release již dříve selhal | `[OTA] Firmware previously failed`, čeká na novější verzi |
| Firmware pro jiný target / špatný checksum / příliš velký | zrušeno, žádný zápis/swap |
| Výpadek napájení během stahování | bezpečné - zapisuje se do volné (neaktivní) oblasti |
| Výpadek napájení během `eboot` swapu | jediné neodstranitelné reziduální riziko (~1s okno) |
| Nový firmware opakovaně nenabootuje | automatický rollback po `OTA_MAX_BOOT_ATTEMPTS` pokusech |

## Odkud to pochází

Tento systém vznikl a byl ověřen (včetně živého testu na hardwaru, který odhalil a opravil
reálný bug s HTTP 302 přesměrováním při stahování metadat) v projektu meteostanice
`wather-station`. Tam najdete i plnou historii rozhodnutí a širší kontext (např. jak se
tenhle OTA klient kombinuje s tovární přípravou zařízení, konfigurací polohy apod.) -
zde je jen ta obecná, přenositelná část.
