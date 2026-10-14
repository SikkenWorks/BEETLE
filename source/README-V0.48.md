# BEETLE V0.48 | Cursor & zes lane-statusblokjes

Deze versie bouwt verder op V0.47 en behoudt de originele geanimeerde startup, synthesizers, samples, Punch-In FX inclusief ALGORITHM-opnames, projectopslag en de bestaande bediening.

## Trackerweergave

- **Cursor:** de roze selectie sluit aan op de *werkelijke breedte* van de zichtbare `[waarde]`, ongeacht gekozen kolom (NOTE, INST, CHD, FX, of NOTE2 op de drum-lane). De vroegere 13 pixels hoge selectie overlapt andere regels: de nieuwe selectie is 9 pixels hoog en blijft volledig binnen de 10-pixel regelafstand. De witte blokhaken en roze tekst blijven behouden.
- **DAC READY / DAC WAIT verwijderd** van de trackerpagina, maar geen wijziging aan het daadwerkelijke DAC-opstartgedrag.
- **Zes 5×5 statusvierkantjes** onderaan links naast het roze insect, op native 240×240 coördinaten in twee kolommen en drie rijen:

  `1 2`
  `3 4`
  `5 6`

- Wit = geen events in die lane; roze = ten minste één noot, STOP, tweede drum-noot/akkoord of step-effect in het *huidige pattern*. De standaard instrumentkeuze en standaard stepwaarden tellen niet als data, zodat een nieuw leeg pattern daadwerkelijk zes witte blokjes heeft.
- De indicatoren lezen de actieve patterninhoud direct bij het tekenen; wissen, invoeren, laden en patternwissels worden op de eerstvolgende schermupdate zichtbaar. Geen wijziging aan sequencertiming of audio-ISR.

## Testresultaten

- Native JieLi FM1 **read-only**, **pilot** en **experimentele opslag/OTA** bouwen succesvol.
- V0.48 lane-test: leeg, 6 lanes, note, STOP, tweede noot/akkoord, FX, wissen, out-of-range: PASS.
- V0.48 geometriecontrole: de 9px kaderhoogte past binnen de 10px trackerregel en de 2×3-indicatoren passen op het scherm: PASS.
- V0.47 origineel bootscherm: beide 57.600 pixels matchen GIF in RGB565, PASS.
- V0.46 audio/Punch-In en V0.45 t/m V0.36 geselecteerde oudere hosttests: PASS. De volledige recursieve historische suite liep tegen een timeout aan en is dus niet als compleet beschouwd.
- OTA-simulatie met stock V15 en eerdere BEETLE-bronidentiteiten, inclusief firmware-integriteit, PASS.
- Firmware `.fwsc`: boot/OTA loader blijft gelijk aan de ETCH V1.37-referentie; de applicatie komt byte-identiek overeen met de gecompileerde app.

## Inhoud

- `source/firmware/fm1-native/`: volledige native C-code
- `source/tests/track/`: nieuwe tests en bestaande regressietests
- `source/tools/`: firmware packer, installer-embedder en eerdere tools
- `source/assets/loading-screen/`: oorspronkelijke boot-GIF en PNG's
- `source/web/BEETLE-V0.48-WebInstaller-EXPERIMENTAL.html`: webinstaller inclusief V0.48 firmware
- `BEETLE-V0.48-STORAGE-EXPERIMENTAL.fwsc`: exp. firmware
- `previous-release-v047/`: bewaarde V0.47 bestanden

## Flashwaarschuwing

Dit is **niet fysiek gevalideerd op een FM1**. De installer en de opslagfirmware zijn experimenteel. Aanpassen van firmware kan de FM1 onbruikbaar maken; bij initialisatie van de experimentele opslag kunnen bestaande ETCH/Felucca-gebruikersdata worden gewist. Flash niet zonder betrouwbare backup en herstelroute.
