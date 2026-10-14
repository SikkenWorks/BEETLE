# BEETLE V0.47 · Originele geanimeerde bootanimatie

Deze release bouwt **rechtstreeks voort op BEETLE V0.46**. Alle bestaande audio-, Punch-In-, mixer-, sequencer-, opslag-, OTA- en UI-functionaliteit is in de nieuwe source behouden.

## Wat is nieuw?

- Het bestaande vergrote icoon op het bootscherm is vervangen door de **originele, aangeleverde `LoadingScreen.gif`**: een zwarte achtergrond, de boom rechts en het roze insect links van de boom. Er zijn geen artistieke of positionele wijzigingen gemaakt.
- **Exact 240 × 240 pixels**, twee frames van **100 ms** (10 FPS), herhalend gedurende het bootscherm. De sprite en de boom worden op hun oorspronkelijke pixelcoördinaten getoond; geen scaling, smoothing of interpolatie.
- De firmware rendert in native RGB565 (vereiste van de FM1). Voor **beide** frames zijn alle **57.600 pixels per frame** vergeleken met het originele GIF na die verplichte kleuromzetting: **100% overeenkomst**.
- De oorspronkelijke GIF en vijf aanvullende PNG’s zitten in `source/assets/loading-screen/` en zijn reproduceerbaar omgezet naar `firmware/fm1-native/beetle_boot_frames.h` door `source/tools/generate_beetle_boot.py`.
- Het bestaande PLAY/STOP-icoon en de aparte OTA-progressanimatie blijven bestaan. Alleen de werkelijke startupanimatie is vervangen.
- Bootanimatie wordt getoond in de normale tweedelige 120-rijen LCD-renderer. Geen extra RAM-framebuffer of opslagtoegang op de opstartanimatie.

## De bestanden

- `BEETLE-V0.47-COMPLETE-SOURCE.zip`: de hele broncode inclusief eerdere tools, tests, ROM-audio en installer.
- `BEETLE-V0.47-STORAGE-EXPERIMENTAL.fwsc`: de opnieuw gebouwde *experimentele* FM1-firmware.
- `BEETLE-V0.47-WebInstaller-EXPERIMENTAL.html`: bijgewerkte webinstaller, met V0.47-firmware intern aanwezig.
- `BEETLE-V0.47-LoadingScreen-Preview.html`: standalone browserpreview die **letterlijk de originele GIF** afspeelt.

## Validatie

- `cd source && python tests/track/test_beetle_v047_visual.py` voor de volledige pixel-voor-pixel-audit.
- `cd source && bash tests/track/run_v047.sh` voor de hostregressies plus animatiecontrole. De volledige historische regressiesuite kan tijdrovend zijn.
- `cd source && node tests/ota/test_stock_install_v047.cjs` voor softwarematige OTA-tests.
- `JIELI_TOOLCHAIN=/pad/naar/jieli python source/firmware/fm1-native/build.py --hardware-test` compileert de read-only testfirmware.
- Voor de experimentele opslagbuild: `--beetle-storage-lab --updater` in plaats van `--hardware-test`.
- Native JieLi-builds: read-only, pilot en storage-lab succesvol gelinkt en gevalideerd.
- Firmwarecontainer: beveiligde bootloader ongewijzigd; OTA-loader ongewijzigd; app in firmware exact gelijk aan de gebouwde binary. Webinstaller-hash en embedded image gecontroleerd.

**Let op:** dit is **niet fysiek getest op een FM1**. De `.fwsc` en installer zijn experimenteel en kunnen het apparaat onbruikbaar maken. Een initiële opslagformat kan oude ETCH/Felucca-gegevens wissen. Flash niet zonder betrouwbare herstelroute en back-up. De pixeltests bewijzen de **rendering van de software**, niet de fysieke kleuren of framerate van een specifiek LCD.
