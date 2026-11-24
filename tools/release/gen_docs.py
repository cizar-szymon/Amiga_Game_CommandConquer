#!/usr/bin/env python3
"""Writes the texts of the Amiga releases (docs/release/amiga) of both games from one template, so that the two
packages say the same things. Run from the project drawer:  python tools/release/gen_docs.py
Texts for players describe the present behaviour only (no history of fixes), see PORTING_PLAN.md."""
import os
import sys

VERSION = open(os.path.join(os.path.dirname(__file__), '..', '..', 'VERSION')).read().strip()
OUT = os.path.join(os.path.dirname(__file__), '..', '..', 'docs', 'release', 'amiga')

GAMES = {
    'TD': {
        'dir': 'TiberianDawn',
        'title': 'COMMAND & CONQUER: TIBERIAN DAWN',
        'short': 'Tiberian Dawn',
        'data_pl': ('Pliki *.MIX z Command & Conquer (Gold), darmowych danych Tiberian Dawn wydanych przez\n'
                    'Electronic Arts, na przyklad:\n'
                    '  CONQUER.MIX GENERAL.MIX SOUNDS.MIX SPEECH.MIX SCORES.MIX UPDATE.MIX UPDATEC.MIX\n'
                    '  TEMPERAT.MIX WINTER.MIX DESERT.MIX TRANSIT.MIX LOCAL.MIX CCLOCAL.MIX DESEICNH.MIX\n'
                    '  TEMPICNH.MIX WINTICNH.MIX\n'
                    'Z plikiem MOVIES.MIX gra pokazuje filmy (bez niego dziala bez filmow).'),
        'data_en': ('The *.MIX files of Command & Conquer (Gold), the freeware Tiberian Dawn data released by\n'
                    'Electronic Arts, for example:\n'
                    '  CONQUER.MIX GENERAL.MIX SOUNDS.MIX SPEECH.MIX SCORES.MIX UPDATE.MIX UPDATEC.MIX\n'
                    '  TEMPERAT.MIX WINTER.MIX DESERT.MIX TRANSIT.MIX LOCAL.MIX CCLOCAL.MIX DESEICNH.MIX\n'
                    '  TEMPICNH.MIX WINTICNH.MIX\n'
                    'With MOVIES.MIX the game plays its movies (without it the game runs without movies).'),
        'pl_game': ('Kampanie GDI i Nod, ekran 640x400, 256 kolorow, gra sie mysza jak na PC.\n'
                    '  Panel boczny otwiera zakladka "Sidebar" w prawym gornym rogu.'),
        'en_game': ('GDI and Nod campaigns, 640x400 screen with 256 colours, played with the mouse as on the PC.\n'
                    '  The sidebar is opened by the "Sidebar" tab at the top right.'),
        'log_tag': 'cnc',
        'tag': 'CnCTD',
    },
    'RA': {
        'dir': 'RedAlert',
        'title': 'COMMAND & CONQUER: RED ALERT',
        'short': 'Red Alert',
        'data_pl': ('Pliki z Command & Conquer: Red Alert (darmowe dane wydane przez Electronic Arts, z plyty\n'
                    'Allied lub Soviet):\n'
                    '  MAIN.MIX      (z glownego katalogu plyty)\n'
                    '  REDALERT.MIX  (z katalogu INSTALL plyty)\n'
                    'MAIN.MIX zawiera tez filmy.'),
        'data_en': ('The files of Command & Conquer: Red Alert (the freeware data released by Electronic Arts,\n'
                    'from the Allied or the Soviet disc):\n'
                    '  MAIN.MIX      (from the main folder of the disc)\n'
                    '  REDALERT.MIX  (from the INSTALL folder of the disc)\n'
                    'MAIN.MIX contains the movies too.'),
        'pl_game': ('Kampanie Aliantow i Sowietow, ekran 640x400, 256 kolorow, gra sie mysza jak na PC.\n'
                    '  Panel boczny otwiera zakladka "Sidebar" w prawym gornym rogu.'),
        'en_game': ('Allied and Soviet campaigns, 640x400 screen with 256 colours, played with the mouse as on the PC.\n'
                    '  The sidebar is opened by the "Sidebar" tab at the top right.'),
        'log_tag': 'cnc',
        'tag': 'CnCRA',
    },
}

PL = '''{title} na Amige (karty Warp)                      wydanie {v}
==========================================================================

Port silnika GPL "Vanilla Conquer" (otwarte zrodla gry firmy Electronic Arts)
na AmigaOS 3.x z wlasnym, natywnym ukladem wyswietlania.

SPRZET
  Przeznaczony dla Amigi 500 / 1200 / 3000 / 4000 z karta Warp (Warp 560,
  1240, 1260, 1260 Extreme, 7060), na takich kartach przenoszony i testowany:
  szybkosc osiaga dzieki szybkiej grafice (RTG) Warpa. Inne Amigi z
  68040/68060 i Picasso96 uruchomia gre, ale wolniej.
  Testowano na: A1200 + Warp 1260 Extreme, A4000 + Warp 7060 (68060),
  AmigaOS 3.2.

WYMAGANIA
  AmigaOS 3.2, Picasso96, 68060.library (lub 040), dane gry (patrz
  data/ReadMe.txt). Dzwiek: wlasny uklad audio Amigi (Paula); AHI jako opcja.
  Zgloszenia bledow z gry wymagaja stosu TCP/IP (Roadshow, Miami, Genesis).

INSTALACJA
  UnZip {dir}-{v}.zip -d Work:Games
  Pliki gry wlozyc do {dir}/data (darmowe dane Electronic Arts, NIE
  dolaczone):
{data}
  Uruchomic {dir} z ikony albo z Shella.

GRA
  {game}
  Filmy sa odtwarzane z dzwiekiem (jesli dane zawieraja filmy).
  Zapisy gry i ustawienia: szuflada "user" w szufladzie gry.
  Ctrl-C zamyka gre.

USTAWIENIA (Tool Types ikony, usun nawiasy, zeby wlaczyc)
  SOUND=PAULA          dzwiek przez wlasny uklad audio Amigi (domyslnie)
  SOUND=AHI            dzwiek przez AHI (na karcie Warp przez HDMI)
  SOUND=OFF            bez dzwieku
  MOVIES=OFF           filmy pomijane
  REPORTSERVER=host:port   serwer zgloszen bledow (domyslnie serwer gier)
  Dzwiek filmow idzie ta sama droga co dzwiek gry.

ZGLASZANIE BLEDOW (F10)
  Klawisz F10 w trakcie gry zapisuje stan gry (w szufladzie "user": bugNNN.txt,
  bugNNN-screen.bmp, bugNNN.sav) i - jesli dziala stos TCP/IP - otwiera okno
  zgloszenia: adres e-mail (nieobowiazkowy, zapamietany) i pole na opis
  problemu (dlugi tekst przechodzi sam do nastepnej linii; Enter w polu e-mail
  przechodzi do opisu, Enter w opisie wysyla; Esc zamyka). Przycisk SEND
  wysyla zgloszenie na serwer gier: e-mail i opis,
  numer wydania, opis Amigi (procesor, Kickstart, pamiec, karta Warp), obraz
  ekranu i zapis gry. Numer zgloszenia pojawia sie po wyslaniu. Bez stosu
  TCP/IP F10 zapisuje tylko pliki - mozna je przeslac autorowi recznie.
  Uwaga: F10 nie jest juz klawiszem zakladki nr 2 (zakladki: F9, F11, F12).

LICENCJA
  GNU GPL v3 (doc/COPYING). Dane gry naleza do Electronic Arts.
'''

EN = '''{title} for the Amiga (Warp cards)                 release {v}
==========================================================================

Port of the GPL engine "Vanilla Conquer" (the open-source game of Electronic
Arts) to AmigaOS 3.x with its own native back end.

HARDWARE
  Made for Amiga 500 / 1200 / 3000 / 4000 with a Warp card (Warp 560, 1240,
  1260, 1260 Extreme, 7060), ported and tested on such cards: it reaches
  its speed through the fast graphics (RTG) of the Warp. Other Amigas with a
  68040/68060 and Picasso96 RTG run it, but slower.
  Tested on: A1200 + Warp 1260 Extreme, A4000 + Warp 7060 (68060),
  AmigaOS 3.2.

REQUIREMENTS
  AmigaOS 3.2, Picasso96, 68060.library (or 040), the game data (see
  data/ReadMe.txt). Sound: the Amiga's own audio chip (Paula); AHI is an
  option. Bug reports from the game need a TCP/IP stack (Roadshow, Miami,
  Genesis).

INSTALLATION
  UnZip {dir}-{v}.zip -d Work:Games
  Put the game files into {dir}/data (the freeware data from Electronic Arts,
  NOT included):
{data}
  Start {dir} from its icon or from a Shell.

THE GAME
  {game}
  Movies are played with sound (when the data contain the movies).
  Saved games and settings: the drawer "user" in the game's drawer.
  Ctrl-C closes the game.

SETTINGS (Tool Types of the icon, remove the brackets to switch on)
  SOUND=PAULA          sound through the Amiga's own audio chip (default)
  SOUND=AHI            sound through AHI (on a Warp card through HDMI)
  SOUND=OFF            no sound
  MOVIES=OFF           movies are skipped
  REPORTSERVER=host:port   server of the bug reports (default: the game server)
  The sound of the movies takes the same way as the sound of the game.

BUG REPORTS (F10)
  The F10 key during the game writes the state of the game (in the drawer
  "user": bugNNN.txt, bugNNN-screen.bmp, bugNNN.sav) and - when a TCP/IP stack
  is running - opens the report window: an e-mail address (optional,
  remembered) and a box for the description of the problem (long text goes to
  the next line by itself; Return in the e-mail field goes to the description,
  Return in the description sends; Esc closes). SEND sends the report to the
  game server: e-mail and description, release number, description of the Amiga
  (CPU, Kickstart, memory, Warp card), the picture of the screen and the
  saved game. The number of the report appears after sending. Without a
  TCP/IP stack F10 only writes the files - they can be sent to the author by
  hand.
  Note: F10 is no longer the key of bookmark 2 (bookmarks: F9, F11, F12).

LICENCE
  GNU GPL v3 (doc/COPYING). The game data belongs to Electronic Arts.
'''

REL_PL = '''{title} na Amige - historia wydan
====================================================================

Jak zglosic blad: klawisz F10 w grze (okno zgloszenia, wymaga stosu TCP/IP) albo
numer wydania (Shell: Version {dir}), opis Amigi i karty oraz co sie robilo.

{v}  (24.11.2026)
  - Okno zgloszenia bledu (F10): jedno pole na opis problemu; dlugi tekst
    przechodzi sam do nastepnej linii. Enter w polu opisu wysyla zgloszenie,
    Esc je anuluje. Okno otwiera sie od razu; stan gry jest zapisywany po
    kliknieciu SEND (komunikat "SAVING THE GAME").
  - Naliczanie kredytow ma jeden wyrazny tik na klatke gry.

0.3  (24.11.2026)
  - Straznik zawieszen zapisuje RAM:cnc-hang.txt dopiero wtedy, gdy gra nie
    odpowiada przez 10 sekund (dluzsze wczytywanie misji go nie uruchamia).

0.2  (24.11.2026)
  - {short}: {game_one_pl}, ekran 640x400, 256 kolorow, sterowanie mysza.
  - Filmy z dzwiekiem (z danymi zawierajacymi filmy); Tool Type MOVIES=OFF je pomija.
  - Dzwiek i muzyka przez wlasny uklad audio Amigi (Paula, domyslnie) albo AHI;
    Tool Types SOUND=PAULA / AHI / OFF. Dzwiek filmow idzie ta sama droga.
  - Zgloszenia bledow klawiszem F10: okno w stylu menu gry, wysylka na serwer gier
    (e-mail, opis, stan gry, obraz ekranu); bez stosu TCP/IP tylko zapis plikow.
  - Start z ikony albo z Shella; Ctrl-C zamyka gre.
'''

REL_EN = '''{title} for the Amiga - history of releases
====================================================================

How to report a bug: the F10 key in the game (report window, needs a TCP/IP
stack) or give the release number (Shell: Version {dir}), your Amiga and card,
and what you were doing.

{v}  (24.11.2026)
  - The bug report window (F10): one box for the description; long text
    goes to the next line by itself. Enter in the box sends the report,
    Esc cancels it. The window opens at once; the state of the game is
    saved after SEND (message "SAVING THE GAME").
  - The credits counter gives one clear tick for every game frame.

0.3  (24.11.2026)
  - The hang guard writes RAM:cnc-hang.txt only when the game does not respond
    for 10 seconds (a long mission load does not start it).

0.2  (24.11.2026)
  - {short}: {game_one_en}, 640x400, 256 colours, mouse control.
  - Movies with sound (with data that contain the movies); Tool Type MOVIES=OFF
    skips them.
  - Sound and music through the Amiga's own audio chip (Paula, default) or AHI;
    Tool Types SOUND=PAULA / AHI / OFF. The sound of the movies takes the same way.
  - Bug reports with the F10 key: a window in the style of the game's menus, sent
    to the game server (e-mail, description, game state, picture of the screen); without
    a TCP/IP stack only the files are written.
  - Start from the icon or from a Shell; Ctrl-C closes the game.
'''

INSTALL = '''INSTALLATION / INSTALACJA - {title} {v}
================================================================

EN
 1. UnZip {dir}-{v}.zip -d Work:Games
 2. Copy the game files into Work:Games/{dir}/data (freeware data from
    Electronic Arts, not included - see data/ReadMe.txt).
 3. Double-click {dir}.

PL
 1. UnZip {dir}-{v}.zip -d Work:Games
 2. Skopiuj pliki gry do Work:Games/{dir}/data (darmowe dane Electronic Arts,
    niedolaczone - patrz data/ReadMe.txt).
 3. Kliknij dwukrotnie {dir}.
'''

README_DATA = '''Put here the files of the game (see below). They are not part of this package.

{data_en}

Tutaj wlozyc pliki gry (ponizej). Nie sa czescia tej paczki.

{data_pl}
'''


def write(name, text):
    with open(os.path.join(OUT, name), 'w', encoding='ascii', newline='\n') as f:
        f.write(text)


for key, g in GAMES.items():
    d = g['dir']
    game_one_pl = {'TD': 'kampanie GDI i Nod', 'RA': 'kampanie Aliantow i Sowietow'}[key]
    game_one_en = {'TD': 'GDI and Nod campaigns', 'RA': 'Allied and Soviet campaigns'}[key]
    common = dict(title=g['title'], dir=d, v=VERSION, short=g['short'], game_one_pl=game_one_pl, game_one_en=game_one_en)
    write(d + '.txt', PL.format(game=g['pl_game'], data=''.join('    ' + l + '\n' for l in g['data_pl'].split('\n')).rstrip('\n'), **common))
    write(d + '_en.txt', EN.format(game=g['en_game'], data=''.join('    ' + l + '\n' for l in g['data_en'].split('\n')).rstrip('\n'), **common))
    write(d + '_release.txt', REL_PL.format(**common))
    write(d + '_release_en.txt', REL_EN.format(**common))
    write(d + '_Install.txt', INSTALL.format(**common))
    write(d + '_data_ReadMe.txt', README_DATA.format(data_en=g['data_en'], data_pl=g['data_pl']))
print('docs for', ', '.join(g['dir'] for g in GAMES.values()), 'release', VERSION)
