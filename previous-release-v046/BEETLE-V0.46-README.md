# BEETLE V0.46 · DRUM GAIN, PUNCH-IN ALGORITHM & SYNTH MIX FIX

Dit is de volledige BEETLE V0.46-source voor de M-VAVE FM1, gebaseerd op V0.45. Alle bestaande DSP-modules, pagina's, patterns, slots, installerroutes en eerdere tools blijven aanwezig.

## Wat is aangepast?

1. **Drumgain in de audiobron:** de ROM-PCM-drum-engine is op intern DSP-niveau versterkt (+4,86 dB vóór de mixer). De CH DRUM-mixerfader staat standaard terug op **80**, niet 100. De effectieve nieuwe fabrieksstand is ongeveer **+2,9 dB** ten opzichte van V0.45 bij zijn standaardvolume 100. Bij zes gelijktijdige drums voorkomt een zachte begrenzer dat de drumsubmix clipt. Elke individuele drum behoudt zijn eigen GAIN/EQ-parameters.
2. **Punch-In neemt ALGORITHM op:** wanneer REC actief is en de sequencer speelt, schrijft een draai aan ALGORITHM tijdens het vasthouden van REPEAT, REVERSE, FILTER, TAPE STOP of STUTTER nu een **parameter-event** in hetzelfde pattern. Ook de waarde op het moment van indrukken wordt vastgelegd. Bij afspelen worden deze parameters per step teruggezet zoals opgenomen. OCT UP, OCT DOWN en MUTE hebben nog steeds geen ALGORITHM-parameter. De fysieke beweging wordt **per sequencerstep (16-step resolutie)** opgeslagen, niet sample-accurate. Snelle meerdere draaien voor dezelfde FX in dezelfde step worden gecombineerd.
3. **Synth-volume aan het einde:** de polyfone mix bewaart de headroom van een overlappende muzikale frase tot alle voices zijn uitgestorven. Loslaten van één voice verlaagt de deler niet plotseling, waardoor de overblijvende noot niet onverwacht **+6 dB** luider wordt. Nieuwe note-on/loops blijven stabiel.
4. **Projecten:** projectformaat **V9** voegt een compacte ALGORITHM-byte aan opgenomen Punch-In-events toe, zonder extra RAM in de 48-pattern bank. Oudere projectversies blijven leesbaar. Bij import van een V8-project met de V0.45-fabriekswaarde drumfader 100 wordt deze naar 80 gemigreerd. In V9 worden handmatige mixerinstellingen exact bewaard.

## Tests / gebruik

- Host regressions: `cd source && bash tests/track/run_v046.sh`.
- Webinstaller OTA-mocks: `cd source && node tests/ota/test_stock_install_v046.cjs`.
- Native read-only FM1: `JIELI_TOOLCHAIN=/pad/naar/toolchain python source/firmware/fm1-native/build.py --hardware-test`.
- Native experimental storage/OTA: dezelfde opdracht met `--beetle-storage-lab --updater`.
- WAV previews in `audio/` zijn met de nieuwe V0.46-drum-engine gerenderd.
- Oude V0.45-releasebestanden en oorspronkelijke samples zijn bewaard onder `previous-release-v045/`.

**VEILIGHEID:** De `.fwsc` en de ingebouwde webinstaller zijn uitsluitend **EXPERIMENTELE** builds. Het pakket is statisch geverifieerd en de OTA-route is softwarematig gesimuleerd, maar **niet op een fysieke FM1 getest**. Initialisatie van het BEETLE-opslaggebied kan eerdere ETCH/Felucca-gegevens wissen. Een onafhankelijk bewezen hardware-recoveryroute ontbreekt. Flashen zonder volledige back-up en betrouwbare recovery wordt afgeraden. De read-only testbuild schrijft geen projecten of firmware.

---

## Historische ontwikkelnotities (V0.45 en eerder)

