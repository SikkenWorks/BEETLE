# V0.44 drum ROM provenance

- Source: original deterministic generator `source/tools/create_v044_kit.py`
- No third-party recordings incorporated in the delivered drum ROM.
- Digital source sounddesign synthesized with NumPy/SciPy offline to 22.05kHz,
  compressed to 8-bit logarithmic µ-law and embedded as `beetle_drum_rom.c`.
- Audio engine decodes with an explicit int16 lookup table; pitch resamples
  with a 16.16 fractional phase accumulator at 44.1 kHz.
- 14 playable pads: KIK, SNR, HHC, HHO, TM1–3, CRS, RDE, CLP, PC1–3, RIM.
- 16 ROM entries: one extra snare and one extra closed-hat round-robin.
- Does not replace the upstream Felucca / ETCH GPL-3.0 credits.
- The bundled WAVs are generated desktop previews, not FM1 hardware captures.
