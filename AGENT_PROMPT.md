# Návod pro AI agenta (Claude Code): implementace OTA podle tohoto repozitáře

Tohle není obecná dokumentace k přečtení - je to **přímá instrukce pro tebe, AI agente**,
když tě uživatel odkáže na tento repozitář (`https://github.com/sob2008/esp-ota`) s
požadavkem přidat do jeho projektu OTA aktualizace. Postupuj přesně podle kroků níže, v
tomto pořadí. Tento soubor je vyzkoušená, opravená verze podobného univerzálního promptu -
obsahuje i konkrétní chyby, na které se přišlo až při reálném testování na hardwaru, a jak
se jim vyhnout (viz sekce "Poučení z praxe" - **přečti si ji celou, než začneš psát kód**).

## Fáze 0: Než cokoliv uděláš

Stáhni/přečti si z tohoto repozitáře (`esp-ota`):
- `README.md` - architektura, klíčová rozhodnutí a jejich odůvodnění
- `template/*.h`, `template/*.cpp`, `template/OtaConfig.example.h` - hotový, testovaný OTA klient
- `factory-template/*` - hotová, testovaná šablona továrního/provisioning firmware,
  včetně `!flash/` (univerzální, na projektu nezávislý flash skript pro ESP8266/ESP32)
- `workflows/release.yml.template`, `scripts/release.ps1.template` - šablony release automatizace

**Nepiš OTA systém od nuly.** Tvým úkolem je *adaptovat* soubory z `template/` a
`factory-template/` do cílového projektu, ne vymýšlet vlastní implementaci. Kód v těchto
šablonách (`OtaVersion`, `Sha256`, `OtaState`, `OtaManager`) je hotový a testovaný -
kopíruj ho beze změny, pokud k tomu není konkrétní důvod (viz Fáze 2).

## Fáze 1: Analýza cílového projektu (POVINNÁ, nic nepředpokládej)

Než napíšeš jakýkoliv kód, zjisti o cílovém projektu:

- Přesný typ čipu (ESP8266 / ESP32 / jiný) a framework (Arduino / ESP-IDF / PlatformIO).
- Aktuální systém buildování a hlavní firmware soubory.
- Existující OTA systém, pokud nějaký je - pokud je kvalitní, rozšiř ho, nevytvářej druhý
  paralelní systém.
- Skutečnou velikost flash a rozložení partition/flash-size nastavení (u ESP8266
  `eesz` board-option, u ESP32 `partitions.csv`) - **neodhaduj, ověř skutečnou
  kompilací** (viz Fáze 5).
- Dostupný persistentní úložiště (LittleFS/SPIFFS/NVS/EEPROM).
- WiFi architekturu, aktuální logování, watchdog.
- Kritické výstupy (relé, motory, topení, servo...) a operace, které OTA nesmí přerušit.

### Rozhodnutí o proveditelnosti

**Pokud cíl je ESP8266 (Arduino core):** postup v `template/`/`factory-template/` je
přímo použitelný - je to přesně ta platforma, na které byl vyvinutý a testovaný.

**Pokud cíl je ESP32:** architektura `OtaState`/`OtaManager` je koncepčně přenositelná,
ale **mechanika rollbacku je jinak** - ESP32 (Arduino-ESP32/ESP-IDF) má nativní A/B
partition a nativní rollback (`esp_ota_mark_app_valid_cancel_rollback()`), takže
`OtaState`'s vlastní `candidate.bin`/`last_good.bin` mechanika (navržená kompenzovat
ABSENCI nativního rollbacku na ESP8266) je pravděpodobně **zbytečná** - zjisti, jestli
dává smysl ji zjednodušit a použít nativní ESP32 rollback API místo ní. `OtaVersion` a
`Sha256` zůstávají beze změny (nezávisí na platformě). Prostuduj README.md sekci
"1. ESP8266 nemá nativní A/B rollback jako ESP32" před úpravou.

**Pokud cíl je jiná platforma** (jiný MCU, jiný framework): zhodnoť, jestli má
ekvivalent `Update`-stylu flashování z HTTP streamu a nějaký persistentní storage. Pokud
ne, **zastav se a řekni to uživateli** - nepokoušej se OTA vynutit na platformě, která
pro to nemá základní stavební kameny.

Pokud OTA na daném hardwaru/frameworku **nedává smysl nebo nejde bezpečně implementovat**,
řekni to uživateli přímo, s konkrétním důvodem, a nepokračuj do Fáze 2.

## Fáze 2: Implementace OTA klienta

1. Zkopíruj `template/OtaVersion.h/.cpp`, `template/Sha256.h/.cpp`, `template/OtaState.h/.cpp`,
   `template/OtaManager.h/.cpp` do složky cílového sketch/projektu.
2. Zkopíruj `template/OtaConfig.example.h` jako `OtaConfig.h`, vyplň `FIRMWARE_TARGET`,
   `GITHUB_OWNER`, `GITHUB_REPOSITORY` podle skutečného repozitáře uživatele.
3. Zapoj podle přesného pořadí v `README.md` (sekce "Rychlý start" - `OtaState::begin()` a
   `OtaManager::begin()` **před** připojením WiFi, `notifyApplicationHealthy()` až po
   prvním úspěšném cyklu hlavní funkce zařízení, `OtaManager::handle()` v `loop()`).
4. Pokud má cílové zařízení nebezpečné výstupy (relé, motory...), přidej před OTA krokem
   uvedení do bezpečného stavu - přizpůsob konkrétní funkci zařízení, nevymýšlej univerzální
   řešení naslepo.
5. Neměň zbytečně existující kód projektu. Preferuj malé izolované moduly a minimální
   zásahy do hlavního programu.

## Fáze 3: Tovární (provisioning) firmware - POVINNÁ SOUČÁST

Uživatel chce **stejný systém jako u referenčního projektu** - to znamená vytvořit i
druhý, samostatný sketch pro tovární přípravu nových/vrácených kusů, podle
`factory-template/`:

1. Vytvoř novou složku/sketch (např. `factory-<nazev-projektu>`).
2. Zkopíruj do ní `factory-template/factory-sw.ino` a znovu `OtaVersion/Sha256/OtaState/
   OtaManager` (stejné soubory jako ve Fázi 2 - Arduino kompiluje sketch složky zvlášť,
   cross-folder `#include` nejde).
3. Zkopíruj `factory-template/OtaConfig.example.h` jako `OtaConfig.h` do téhle složky -
   `FIRMWARE_TARGET`/`GITHUB_OWNER`/`GITHUB_REPOSITORY` musí být **identické** s ostrým
   firmware, `FIRMWARE_VERSION` zůstává `"0.0.0"` a `OTA_CHECK_INTERVAL_MS` krátký (viz
   komentáře v souboru - proč).
4. Přizpůsob `factory-sw.ino`: pokud má zařízení displej/LED, přidej
   `OtaManager::setStatusCallback()` s vykreslováním (vzor v referenčním projektu -
   `wather-station/factory-sw/factory-sw.ino`, dvě SH1106 OLED). Pokud ostré firmware při
   prvním nastavení potřebuje od uživatele další údaj (podobně jako referenční projekt
   řeší polohu/jazyk), přidej další `WiFiManagerParameter` do stejného portálu - vlastní
   modul pro perzistenci, stejný vzor jako `OtaState` (write-temp-then-rename).
5. Připomeň uživateli: tovární firmware nainstaluje **existující** GitHub Release - dokud
   repozitář žádný nemá, bude jen dokola zkoušet.
6. Zkopíruj `factory-template/!flash/` (celou složku vč. `flash.py`, `flash.sh`,
   `bin/`) vedle nového továrního sketch. Je to univerzální, na projektu nezávislý
   flash skript (ESP8266 i ESP32) - **needituj ho**, jen do `!flash/bin/` bude uživatel
   dávat zkompilovaný `.bin`. Řekni uživateli, jak se používá (`python flash.py` /
   `./flash.sh`, `--monitor` pro sériový monitor) - viz README.md v tomto repozitáři,
   sekce "Flashování".

## Fáze 4: Release automatizace

1. Zkopíruj `workflows/release.yml.template` jako `<projekt>/.github/workflows/release.yml`,
   uprav místa označená `<<...>>` (cesta ke sketch, FQBN desky, seznam knihoven).
2. Zkopíruj `scripts/release.ps1.template` jako `<projekt>/scripts/release.ps1` (běží
   beze změny, `-SketchDir` je parametr při spuštění).
3. **Nikdy nevytvářej GitHub Actions, který by se spouštěl automaticky při běžném push do
   `main`** - jen na ručně vytvořený a pushnutý tag `vX.Y.Z`. To je základní bezpečnostní
   vlastnost celého systému (uživatel má vždy plnou kontrolu nad tím, co se vydá).

## Fáze 5: Testování (bez hardwaru, co nejvíc)

1. `OtaVersion` a `Sha256` jsou navržené jako Arduino-nezávislé - zkopíruj/uprav
   `tests/test_ota_version.cpp` a `tests/test_sha256.cpp` (host-side `g++`, žádný ESP core
   potřeba) a spusť je.
2. Zbytek (`OtaState`, `OtaManager`, oba sketche) ověř **reálnou kompilací** proti
   skutečnému toolchainu cílového projektu (arduino-cli/PlatformIO/idf.py - podle toho, co
   projekt používá). Nespokojuj se s "mělo by to fungovat" bez zkušebního buildu.
3. Zkontroluj velikost výsledného firmware vůči zjištěné velikosti OTA oblasti (Fáze 1) -
   musí se s rezervou vejít.
4. Otestuj i YAML/PowerShell šablony (syntaktická validace, viz jak to řešil referenční
   projekt - `python -c "import yaml; yaml.safe_load(open(...))"` a
   `[System.Management.Automation.Language.Parser]::ParseFile(...)`).

## Fáze 6: Report uživateli

Na závěr shrň: přesný typ čipu/frameworku, zjištěnou velikost flash a OTA oblastí, jaké
soubory vznikly/se změnily, jak funguje rollback na téhle konkrétní platformě, jaké testy
proběhly a co nelze ověřit bez fyzického zařízení, a jaká omezení/rizika zůstávají
(zejména TLS - viz níže).

---

## Poučení z praxe (přečti si PŘED psaním kódu)

Tyhle věci se zjistily až při reálném nasazení a testování na hardwaru - nejsou
teoretické, jsou to skutečné bugy, které se staly a byly opravené:

1. **Redirect-following musí být zapnuté pro KAŽDÝ HTTP požadavek, který se dotýká
   asset URL z GitHub Release** - ne jen pro stažení `firmware.bin`. `browser_download_url`
   assetu (i `firmware.json`, i `firmware.bin.sha256`) je VŽDY 302 přesměrování na CDN.
   Bug: povolený redirect jen u stahování hlavního binárky způsobil, že se `firmware.json`
   a `firmware.bin.sha256` nikdy nestáhly (tiché selhání na kód 302), takže OTA vždy
   skončilo na "No checksum available". Řešení v `template/OtaManager.cpp`: jedna
   společná `httpBeginCommon()` funkce pro všechny HTTP požadavky, ne zvlášť pro každý.

2. **Persistentní "pending validation" stav se zapisuje PŘED finálním zápisem/
   potvrzením OTA (na ESP8266 `Update.end()`), ne po něm.** Jakmile platformní OTA
   mechanismus jednou úspěšně "potvrdí" novou verzi (na ESP8266 vrátí `Update.end()`
   `true`), je nový firmware nastaven k nabootování při dalším restartu bez ohledu na to,
   jestli k restartu skutečně došlo hned. Kdyby se stav zapsal až po tomto potvrzení,
   existuje okno (výpadek napájení přesně mezi potvrzením a zápisem stavu), kdy by
   zařízení nabootovalo nevalidovaný firmware, aniž by o tom sledování stavu vědělo -
   ztráta rollback ochrany. Viz `OtaManager::downloadAndApply()` (zápis stavu) a
   `OtaManager::begin()` v tomto repozitáři (detekce "stale" pending stavu porovnáním
   s compile-time `FIRMWARE_VERSION` skutečně běžícího firmware).

3. **ESP8266 nemá nativní rollback ani A/B partition switch jako ESP32** - `Update`/
   `eboot` je ping-pong kopírovací mechanismus, ne instantní přepnutí bootovací partition.
   Neopisuj ESP32 vzory (`esp_ota_*`) na ESP8266 bez ověření, že tam existují - většinou
   neexistují. Viz README.md, sekce 1.

4. **Nikdy nezabudovávej TLS kořenový certifikát (cert pinning) do firmware, pokud
   nemůžeš 100% ověřit, že je to skutečně správný certifikát daného serveru** - špatný
   certifikát OTA trvale a tiše rozbije. Pokud si nejsi jistý (např. síť, ze které
   certifikát stahuješ, může být zprostředkovaná/testovací), použij `setInsecure()` a
   kompenzuj to **povinnou** SHA-256 kontrolou - to je bezpečnější než hádaný certifikát.

5. **Neodhaduj velikost flash/OTA partition - ověř ji skutečnou kompilací** (u ESP8266
   `ESP.getFreeSketchSpace()`, případně přímo inspekcí `.map` souboru po kompilaci).
   `eesz` board-option (ESP8266) nebo `partitions.csv` (ESP32) urči skutečné rozložení,
   ne to, co "obvykle bývá".

6. **Testuj `PowerShell`/`bash` skripty a `.yml` soubory syntakticky, ne jen "vypadá to
   dobře"** - jednoduchá validace (parser/yaml.safe_load) odhalí překlepy a chyby
   v escapování dřív, než je najde uživatel při ostrém použití.

## Absolutní pravidla (nesmí se porušit)

- NIKDY nepřepisuj běžící firmware před bezpečným dokončením OTA.
- NIKDY nespouštěj firmware, který se nevejde do OTA oblasti, s neplatným checksumem,
  nebo pro jiný hardware target.
- NIKDY nemaž persistentní data (konfigurace, kalibrace) kvůli OTA, pokud to není
  výslovně nutné.
- NIKDY nedovol, aby GitHub byl nutný pro normální provoz zařízení - OTA je jen
  doplňková funkce.
- NIKDY nedovol nekonečnou OTA smyčku (opakované stahování a instalování té samé vadné
  verze) - viz `OtaState`'s `last_failed_version`.
- NIKDY nevytvářej GitHub Actions, který by se spouštěl automaticky na běžný push (jen na
  ručně vytvořený tag).
- NIKDY nedělej `git commit`/`git push` bez explicitního svolení uživatele - ani u OTA
  kódu, ani u release tagů.
- Pokud si nejsi jistý nějakou částí architektury cílového projektu, nejdřív ji
  analyzuj (Fáze 1) a použij řešení odpovídající skutečnému projektu - nehádej.
