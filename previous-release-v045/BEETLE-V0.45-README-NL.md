# BEETLE V0.45 — PROJECT OVERWRITE / SUSTAIN FX / LOUDNESS

## 1. SAVE / OVERWRITE

- Selecteer een bestaand projectslot en druk SAVE; bevestig `OVERWRITE?` met SELECT.
- Alleen dat ene slot wordt vervangen; de andere elf slots blijven byte-voor-byte ongewijzigd.
- De journaalopslag gebruikt twee banken per slot. Een power-loss tijdens opslaan laat de laatst bevestigde kopie bestaan.
- Een fout bij het herkennen van al geïnitialiseerde flashopslag leidt **niet** tot een verzoek om alles te wissen. Er verschijnt een niet-destructieve opslagfout.
- Bij een **volledig nieuwe, lege** opslagruimte kan de aparte eerste initialisatie worden gestart. De bevestiging is vijf seconden SELECT+SAVE; **dit wist alle twaalf slots**, plus eventuele vreemde gebruikersdata in het gereserveerde gebied. Dit gebeurt **niet automatisch** bij de eerste boot.

## 2. Per-step FX sustain

SYNTH CH1–CH5: een effect begint op zijn step en blijft actief over lege steps. Het stopt of wordt vervangen bij `[STP]`, een nieuwe noot, of een nieuwe FX-step. Een nieuwe FX-step kan tijdens het sustain van dezelfde noot worden geplaatst. Ook PITCH verandert een klinkende noot zonder opnieuw aanslaan. Elke lane heeft zijn eigen aanhoudende effect-tijd; deze wordt niet bij ieder step-interval gereset. Stop en project LOAD verwijderen achtergebleven automatisering.

## 3. Loopvolume

V0.44 schaalde de synthmix op basis van het aantal ooit toegewezen stemmen. Eén nieuwe noot op step 0 gebruikte in de tweede loop een extra stemslot en klonk daardoor permanent zachter. V0.45 telt uitsluitend daadwerkelijk hoorbare, niet vrijgegeven stemmen voor headroom. Een C-audiotest bevestigt dezelfde RMS-energie over 14 loop-herhalingen (tolerantie 15%).

## 4. Compatibiliteit en veiligheid

- V8-projectformaat blijft ongewijzigd; oudere formaatfixtures blijven getest.
- ROM-drumkit en Punch-In-engine uit V0.44 behouden.
- De originele beschermde ETCH V1.37-header en OTA-loader blijven byte-identiek.
- Stock-V15-herkenning in de webinstaller blijft behouden en is alleen gesimuleerd getest.
- Projectopslag en OTA zijn nog niet hardware-gevalideerd. Flashen/initialisatie zonder volledige back-up en onafhankelijke recovery is niet aangeraden.

**Tests:** `bash source/tests/track/run_v045.sh` (uit distributiemap) of `bash tests/track/run_v045.sh` (vanuit `source`).
