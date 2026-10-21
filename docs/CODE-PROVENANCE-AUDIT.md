# BEETLE 1.01 RC1: provenance- en licentiecontrole

## Reikwijdte

Dit is een bronboom- en buildketencontrole van de daadwerkelijke 1.01 RC1-package, geen juridische non-infringement-garantie of volledige onafhankelijke similarity-audit met externe upstream-repositories.

## Gevonden

- De native buildscript `source/firmware/fm1-native/build.py` compileert expliciet **27** lokale C- en assemblybestanden. Er staat **geen** reSID- of Nes_Snd_Emu-bibliotheek op de compileer- of linklijst.
- Buiten gegenereerde buildmappen bevat deze bronboom **geen `.cpp`, `.cc`, `.cxx`, `.hpp`, `.a` of `.so`** bron- of bibliotheekbestanden (controle op extensie en locatie).
- De SID-geïnspireerde oscillators staan in `source/firmware/fm1-native/beetle_sid_lite.h` als eigen fixed-point C-routines (`beetle_sid_wave`, `beetle_sid_sample`), met code en klankkarakter dat niet als originele/cycle-exacte reSID wordt gepresenteerd.
- De NES-geïnspireerde oscillators staan in `source/firmware/fm1-native/beetle_nes_apu.h` als kleine C-functies (`beetle_nes_pulse`, `beetle_nes_triangle`, `beetle_nes_noise`) met hardware-gebaseerde tabellen en modelmatige golfvormen. Het is geen complete Nes_Snd_Emu-APU.
- Credits voor **Dag Lem**, **Shay Green (blargg)** en **James Athey** blijven behouden, evenals `CREDITS-AND-LICENSES.md` en de referentielicenties; geen auteurschap of herkomst wordt afgeschreven.
- De daadwerkelijke BEETLE-source bevat wel oorspronkelijke **Felucca/ETCH-hardware/boot/HAL-lijn** onder eigen upstream-notices. Die dependency blijft afzonderlijk vermeld.

## Conclusie en grens

**Geen upstream reSID of Nes_Snd_Emu C++-bron, gelinkte upstream-library of oorspronkelijke upstream-buildmodule aangetroffen of gebruikt in de huidige native buildketen.** De beschikbare C-implementaties zijn zelfstandig vormgegeven en verwijzen voor de technische concepten naar de genoemde upstream-projecten. Dit ondersteunt dat de originele libraries niet worden meegeleverd of direct gebruikt.

**Niet formeel bewezen:** dat geen individueel algoritmisch codefragment enige overeenkomst vertoont met upstream code. Daarvoor is een volledige bron-op-bronvergelijking met de exacte officiële upstream-revisies en licentie-/provenance-review nodig. Die upstream snapshots zijn hier niet lokaal aanwezig en konden in deze uitvoering niet worden opgehaald. Het is daarom onjuist om met absolute zekerheid te claimen dat nooit code is overgenomen.

## Schermcredits

De eerdere regel `NO NES_SND_EMU CODE USED` is uit **native FM1 CREDITS** en **WebSimulator CREDITS** verwijderd. De neutrale tekst `NES TECHNICAL REFERENCE` blijft naast de auteursvermelding staan.

## Controlebestanden

- Compiled modules: bringup.c, track_core.c, track_name.c, track_mixer.c, track_song.c, track_chip.c, track_drum.c, beetle_drum_rom.c, track_project_store.c, track_journal.c, beetle_flash_layout.c, audio_core.c, native_performance.c, native_seq_ui.c, native_pattern_bank.c, native_project_codec.c, native_wave_fx.c, native_fx.c, native_factory.c, wave_editor.c, wave_store.c, native_digest.c, native_panel_midi.c, native_performance_midi.c, crt0.S, hal/fm1_vec.S, hal/fm1_isr.S
- Referentiedocumentatie/licenties: CREDITS-AND-LICENSES.md, LICENSE, LICENSES/FELUCCA-LICENSING.txt, LICENSES/NES-SND-EMU-LGPL-2.1.txt, firmware/fm1-native/LICENSE, firmware/fm1-native/LICENSES/MIT-Rings.txt, firmware/fm1-native/LICENSES/Apache-2.0-msfa.txt, firmware/fm1-native/LICENSES/OFL-DotGothic16.txt, firmware/fm1-native/LICENSES/OFL-InterTight.txt, firmware/fm1-native/LICENSES/Apache-2.0.txt, firmware/fm1-native/LICENSES/MIT-DaisySP.txt, firmware/fm1-native/LICENSES/MIT-Fukiai.txt
