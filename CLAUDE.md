@C:/Workprojects/AmigaGAMES/PORTING_PLAN.md
@C:/Workprojects/AmigaGAMES/Warp-RTG-reguly-dla-portow.md
@C:/Workprojects/AmigaGAMES/AmiAgent/AMIAGENT.md

# Plan portu Command & Conquer (Tiberian Dawn + Red Alert): Amiga z kartą Warp (68060 + RTG)

> Dokument roboczy. Dane o sprzęcie pochodzą z dokumentów **„Warp RTG Developer notes”** i **„Karty Warp — architektura i firmware 3.0”** (firmware 3.0, październik 2026). Pomiary VRAM: Warp, 68060 @ 108 MHz, ekran 640×480×8. Punkty oznaczone **(do sprawdzenia)** wymagają weryfikacji w kodzie. Cele fps to szacunki do potwierdzenia w fazie 1.

## 1. Cel i zakres

**Cel:** natywny port **Vanilla Conquer** (silnik C&C pierwszej generacji) na AmigaOS 3.x dla kart **Warp z MC68060**: grafika RTG w FPGA karty (nie Zorro), Fast RAM w DDR3 z cache L2, wyjście HDMI. Gra ma działać płynnie w 640×400, z filmami FMV i pełną rozgrywką.

**Konfiguracje docelowe:**

| Karta | Komputer | Priorytet |
|---|---|---|
| Warp1260 / Warp1260X | Amiga 1200 | główny |
| Warp7060 | Amiga 3000/4000 | główny |
| Warp560 | Amiga 500 | test zgodności |
| Warp1240 (68040) | Amiga 1200 | opcjonalnie (bez strojenia) |

**Wymagania programowe:** AmigaOS 3.2, Picasso96, `68060.library`, pakiet firmware Warp 3.0, AHI z `warp.audio`. Do gry sieciowej: stos TCP/IP (`bsdsocket.library`) na `warpNET.device`.

**Kolejność gier:** najpierw Tiberian Dawn, potem Red Alert (ten sam silnik, większe wymagania).

**Poza zakresem:** AGA bez RTG, Vampire/PiStorm (mają port Artura Jarosika), Tiberian Sun i nowsze.

## 2. Punkt wyjścia

| Źródło | Co daje | Uwagi |
|---|---|---|
| `github.com/TheAssemblyArmada/Vanilla-Conquer` | Przenośny silnik TD + RA, GPL-3.0-or-later | **Baza projektu.** Upstream używa SDL2 i OpenAL |
| `github.com/electronicarts/CnC_Tiberian_Dawn`, `CnC_Red_Alert` | Oryginalny kod (EA, luty 2025, GPLv3) | Referencja przy błędach i różnicach zachowania |
| Port Artura Jarosika (arczi) TD i RA | Działający port 68k dla Vampire V4 / PiStorm / Emu68 | **(do sprawdzenia)** czy źródła są publiczne. Jeśli tak, to najcenniejsze źródło poprawek endianness i backendu Amiga |
| Port SDL2 Duncana Bowringa | AmigaOS 3.x, cel 68040 + RTG (A4000) | W trakcie prac. Kontakt, żeby nie dublować pracy |
| Dane gry | TD i RA wydane przez EA jako freeware | Nie dołączać do paczki, tylko instrukcja, skąd pobrać |

**Przed startem:** kontakt z obydwoma autorami. Najlepszy scenariusz: wspólny backend AmigaOS 3.x z profilami sprzętowymi (Vampire / Warp / inne RTG), a nie kilka niezależnych forków.

## 3. Sprzęt Warp: co z tego wynika dla portu

### 3.1 VRAM: zasady z notatek Warp RTG

VRAM nie jest cache'owany. Zapisy idą przez bufor zapisu i są szybkie, odczyty same w sobie też. Kosztowne jest **przełączenie z zapisu na odczyt: ok. 0,6 µs za każdym razem**.

Zmierzone przepustowości (MB/s):

| Operacja | VRAM | Fast RAM |
|---|---|---|
| zapis bajtów | 15,3 | 51,2 |
| zapis long | 61,5 | 75,7 |
| zapis MOVE16 | 77,4 | 52,9 |
| odczyt-modyfikacja-zapis każdego longa | **6,0** | 58,8 |
| kopia z Fast RAM, long | 35,6 | 40,6 |
| **kopia z Fast RAM, MOVE16** | **60,5** | 44,8 |
| `WriteChunkyPixels` z Fast RAM | 35,0 | – |
| kopia VRAM→VRAM procesorem, long | 5,5 | – |
| **`BltBitMap` w obrębie ekranu (blitter Warp)** | **514** | – |

Zasady dla portu:

1. **Klatka renderowana w Fast RAM**, do VRAM tylko kopiowanie gotowych pikseli raz na klatkę.
2. **Żadnej pętli, która czyta i pisze VRAM.** Sprite'y SHP z przezroczystością, cienie, remap kolorów, mgła/shroud tylko w buforze Fast RAM.
3. **Kopiowanie przez `MOVE16`** (bufor i wiersze ekranu wyrównane do 16 bajtów, `AllocVec` gwarantuje tylko 8, więc wyrównać ręcznie). Fallback: `CopyMemQuick` / long. Gotowa procedura `CopyRows16` jest w notatkach Warp RTG, rozdz. 5.
4. **Przesunięcia w obrębie VRAM** przez `BltBitMap` / `ScrollRaster` (blitter Warp na ekranach 8/16-bit).
5. **Ekran CLUT 8-bit** (C&C jest 8-bitowe z paletą): bez konwersji, fade'y i efekty palety przez `LoadRGB32`.
6. **Blokada bitmapy krótko:** `p96LockBitMap()` → kopia → `p96UnlockBitMap()`.

### 3.2 Jak to pasuje do architektury C&C

- **(do sprawdzenia)** Oryginalny silnik rysuje na stronę ukrytą (`HidPage`) i kopiuje na widoczną (`SeenPage`). To dokładnie model zalecany dla Warpa. Pilnować, żeby backend nie zrobił z `SeenPage` zablokowanej bitmapy, do której cokolwiek rysuje się bezpośrednio.
- **(do sprawdzenia)** Silnik ma własny system odświeżania tylko zmienionych komórek mapy. Wykorzystać go do dirty rectangles: kopiować do VRAM tylko zmienione obszary.
- **Kursor myszy:** w wersji DOS kursor rysowany programowo z zapamiętaniem tła, czyli z czytaniem ekranu. Na Warpie to RMW na VRAM. Zastąpić **sprzętowym kursorem** (sprite FPGA przez P96).
- Pasek boczny (sidebar) i radar: rysować w buforze Fast RAM razem z resztą, bez osobnych zapisów do VRAM.

### 3.3 Budżet klatki przy kopiowaniu MOVE16 (~60 MB/s)

| Tryb (8-bit CLUT) | Bajty na klatkę | Pełna kopia | Udział w klatce 60 fps (16,7 ms) |
|---|---|---|---|
| 320×200 | 63 KB | ~1 ms | ~6% |
| 640×400 | 250 KB | ~4,2 ms | ~25% |
| 640×480 | 300 KB | ~5 ms | ~30% |

C&C ma limit szybkości gry (logika w stałym tempie), więc pełna kopia 640×400 mieści się z zapasem. Dirty rectangles zwalniają czas CPU dla logiki przy dużych bitwach, zwłaszcza w Red Alert.

**(do sprawdzenia)** Czy P96 na Warpie udostępnia niskie tryby (320×200 / 320×240) skalowane sprzętowo przez FPGA do HDMI. Jeśli tak, tryb zgodności 320×200 i filmy VQA nie wymagają skalowania programowego.

### 3.4 Fast RAM: DDR3 + L2

- Cache 68060 (8 KB danych) → L2 na karcie: **96 KB, 6-way, linie 128 B, LRU, write-back** → DDR3.
- Czas dostępu: trafienie w cache 060 ok. 20 ns, w L2 ok. 100 ns, chybienie do DDR3 **600–800 ns**.
- Dostęp sekwencyjny jest szybki, **losowy po megabajtach jest wolny**.

Wnioski:
- Archiwa MIX wczytane w całości do pamięci (zero doczytywania z dysku w trakcie misji), ale gorące dane trzymać zwarto: tablica komórek mapy, lista obiektów, dane pathfindingu.
- Sprite'y SHP zdekodowane z wyprzedzeniem do formatu gotowego do sekwencyjnego blitu.
- Tablice LUT (remap stron, cienie) są małe w C&C. Unikać dużych tablic, które wypychają L2.
- **Liczniki L2 w WarpDiag / WarpTool** do pomiaru skuteczności cache w konkretnej scenie przed i po zmianie.

### 3.5 Dźwięk: `warp.audio` i MHI

- AHI przez `warp.audio`: tryby miksowania **44,1 / 22,05 / 11,025 kHz**, karta interpoluje do 44,1 kHz, więc niższa częstotliwość odciąża 68k bez utraty jakości wyjścia.
- Własny kanał dźwięku w DPRAM: ładowanie z dysku i sieć nie przerywają dźwięku.
- Efekty AUD (Westwood ADPCM) zdekodowane do PCM przy ładowaniu misji.
- Muzyka, opcja domyślna: AUD → PCM w pamięci (dużo Fast RAM).
- Muzyka, opcja dodatkowa: jeśli użytkownik ma ścieżki w MP3, odtwarzanie przez **MHI** (dekoder MP3 w STM32, zero obciążenia 68k).
- OpenAL z upstreamu zastąpić backendem AHI.

### 3.6 Pozostałe usługi karty

- Klawiatura, mysz i pady USB są widoczne jako natywne urządzenia Amigi (input.device). Skróty z PC (grupy jednostek Ctrl+1–9) muszą działać.
- Dyski SD/USB działają asynchronicznie (firmware 3.0), co skraca ładowanie.
- Sieć: `warpNET.device` (SANA-II) przez WiFi (ESP32) albo Ethernet USB. **Ethernet USB ma pełną prędkość tylko na Warp1260X i Warp7060**, na pozostałych kartach lepsze jest WiFi. Dla gry RTS liczy się opóźnienie, a nie przepustowość.
- **STM32 nie jest procesorem aplikacyjnym:** nie planujemy przenoszenia kodu gry na ARM. Korzystamy tylko z usług (AHI, MHI, sieć, dyski).

## 4. Endianness (największe ryzyko poprawności)

Oryginalny kod pisano na x86 (little-endian), a 68060 jest big-endian. Pliki danych C&C są little-endian.

- Audyt wszystkich miejsc, gdzie struktury są czytane z plików wprost do pamięci: MIX, SHP, mapy INI/BIN, palety, AUD, VQA.
- Zapisy gry: jeden, opisany porządek bajtów (najlepiej little-endian dla zgodności z PC).
- Sieć: pakiety zawsze w jednym porządku bajtów (zgodność z PC, jeśli gra z PC ma działać).
- Bitfieldy i unie: kolejność bitów zależy od kompilatora, sprawdzić każde użycie.
- **(do sprawdzenia)** ile rozwiązał już upstream Vanilla Conquer i port arcziego.
- Testy na hoście: porównanie zdekodowanych zasobów (SHP, palety, mapy) na x86 i 68k (emulator), bajt w bajt.

## 5. Optymalizacja pod 68060

- Kompilacja z `-m68060`, żeby uniknąć instrukcji emulowanych w `68060.library`:
  - 64-bitowe `MULU.L`/`MULS.L`/`DIVU.L`/`DIVS.L`,
  - FPU transcendentalne (`FSIN`, `FCOS`, `FATAN`, `FLOG*` itp.).
- C&C liczy głównie na liczbach całkowitych, ale przejrzeć disassembler pod kątem powyższych instrukcji.
- Gorące miejsca: blity SHP z remapem i cieniami, shroud, pathfinding, sortowanie warstw, logika przy dużej liczbie jednostek.
- Pętle rysujące pod superskalarność 060; kluczowe blity (shadow, remap, transparent) rozważyć w asemblerze.
- Wyrównanie struktur do 4 bajtów, uwaga na `#pragma pack` z kodu x86. Bufory obrazu wyrównane do 16 bajtów.
- Profilowanie: timer E-Clock, WarpDashboard (obciążenie 68k), liczniki L2.

## 6. Filmy FMV (VQA)

- Dekoder VQA w C z gorącymi pętlami pod 060, wyjście do bufora 8-bit w Fast RAM, kopia `MOVE16` do VRAM.
- Skalowanie 2× do 640×400 w Fast RAM albo sprzętowo przez FPGA (sekcja 3.3, do sprawdzenia).
- Dźwięk filmu przez AHI, synchronizacja do zegara audio (tempo miksowania wyznacza karta, więc jest stabilne).
- Opcja wyłączenia filmów.

## 7. Fazy i kryteria ukończenia

| Faza | Zakres | Kryterium ukończenia |
|---|---|---|
| **F0: środowisko** | Toolchain (bebbo amiga-gcc z C++, NDK 3.2, SDK P96, AHI), Vanilla Conquer na hoście z danymi freeware, Warp z firmware 3.0, **WarpStressTest przy docelowym zegarze** | Vanilla Conquer działa na PC; pusty program 68k otwiera ekran P96 na Warpie; zegar stabilny |
| **F1: kontakt i pomiar** | Rozmowa z arczim i Bowringiem, test ich portów na Warpie, RTGBench, wariant SDL2 jako punkt odniesienia | Decyzja: na czym budujemy + tabela pomiarów |
| **F2: kompilacja i endianness** | Build 68k, audyt i poprawki endianness, testy zasobów bajt w bajt | Menu i mapa TD wyświetlają się poprawnie |
| **F3: backend Warp/P96** | Bufor w Fast RAM, `CopyRows16` z fallbackiem, sprzętowy kursor, `warp.audio`, input, timer E-Clock | Pełna misja TD do przejścia; brak RMW na VRAM |
| **F4: optymalizacja 060** | Profil, flagi, dirty rectangles z systemu komórek mapy, układ danych pod L2, asembler w blitach, preload MIX | Docelowy fps (sekcja 8) w 640×400 |
| **F5: FMV** | Dekoder VQA, skalowanie, synchronizacja audio | Odprawy płynne |
| **F6: Red Alert** | Ten sam backend dla RA | Pełna misja RA do przejścia |
| **F7: sieć (opcjonalnie)** | UDP przez `bsdsocket.library` na `warpNET.device` | Gra 2 Amig z Warpem przez WiFi |
| **F8: testy** | Warp1260, 1260X, 7060, 560; dla porównania jedna karta Zorro | Lista znanych problemów, poprawki krytyczne |
| **F9: wydanie** | Paczka LHA, README (firmware 3.0, P96, ustawienia AHI, skąd dane freeware), źródła publicznie | Wydanie zgodne z GPLv3, bez danych gry |

## 8. Metryki docelowe (do potwierdzenia w F1)

- Warp 68060 ~100 MHz, Tiberian Dawn 640×400: **stałe tempo gry bez spowolnień** przy dużej bitwie (≥ 30 fps renderu).
- Red Alert 640×400: ≥ 30 fps przy dużej bitwie.
- Kopia pełnej klatki 640×400: ≤ 4,5 ms (MOVE16).
- Filmy VQA bez gubienia klatek.
- Rozgrywka identyczna z PC (AI, balans, misje, zapisy).

## 9. Ryzyka

| Ryzyko | Skutek | Mitygacja |
|---|---|---|
| Ukryte błędy endianness | uszkodzone grafiki, desynchronizacja, crashe | testy zasobów bajt w bajt, kod arcziego jako referencja |
| Kursor / sidebar / radar rysowane wprost do VRAM | odświeżanie w tempie Chip RAM | sprzętowy kursor, wszystko przez bufor Fast RAM |
| Emulowane instrukcje 060 | spadki fps | `-m68060`, audyt disassemblera |
| Losowy dostęp poza L2 przy wielu jednostkach | wolna logika | zwarte struktury, pomiar licznikami L2 |
| Niestabilny zegar CPU | błędne pomiary, losowe błędy | WarpStressTest przed każdą serią pomiarów |
| Dublowanie pracy z istniejącymi portami | strata czasu | kontakt w F1, wspólny backend |

## 10. Licencje

- Kod: GPLv3 (Vanilla Conquer, kod EA). Źródła publiczne przy każdym wydaniu.
- Kod z portów arcziego / Bowringa: tylko za zgodą i zgodnie z ich licencją, z zachowaniem autorstwa.
- Dane gry: freeware EA, ale **nie dołączać do paczki**. W README podać, skąd je pobrać i gdzie wgrać.

## 11. Checklista startowa

- [ ] Kontakt z Arturem Jarosikiem (dostępność źródeł portu 68k)
- [ ] Kontakt z Duncanem Bowringiem (stan wersji SDL2, plan wspólnego backendu)
- [ ] Warp z firmware 3.0, zegar zweryfikowany WarpStressTest
- [ ] Fork Vanilla Conquer, build na hoście z danymi freeware
- [ ] Toolchain 68k z C++ i build testowy
- [ ] Lista miejsc czytających struktury z plików (endianness)
- [ ] Sprawdzenie modelu `HidPage`/`SeenPage` i systemu odświeżania komórek w Vanilla Conquer
- [ ] RTGBench na docelowej karcie
- [ ] Test istniejących portów na Warpie
