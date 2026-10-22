# BEETLE V1.4.9 — Original BD4S Streaming / Prefetch Lab

**Development stage, 21 October 2026. NOT an FM1 firmware update.**

## What actually changed

The project now contains independently written C code to stream the original
BD4S sample format through two small decoded PCM windows per voice. The new
module is `firmware/fm1-native/beetle_bd4_prefetch.[ch]`. It has a bounded
background worker, a RAM-only audio renderer, six independently addressable
voices, and thirteen assignable sample-source IDs (U01–U13). It does **not**
call or embed another firmware's sample engine.

The original project includes upstream Felucca/ETCH lineage and therefore
retains GPL-3.0-only and relevant upstream credits. No licence notices or
existing BEETLE functions have been removed.

## Architecture

| Stage | Function | Allowed to read external storage? |
|---|---|---|
| BD4S source (13 slots) | Verified, versioned compressed mono blobs | Storage provider owns this |
| Main-loop prefetch | Read 16-byte header / 6-byte block index, load up to 134 compressed bytes, decode <=64 frames per operation | YES, outside audio IRQ |
| Six-voice ring window | Two 64-frame PCM windows per voice; bounded preallocated memory | NO |
| Render | Output 44,100 Hz mono with simple 2:1 linear interpolation from 22,050 Hz PCM | NO |

- The host-compiled **six-voice structure is 2,816 bytes**. This is an *incremental
  RAM cost in a standalone test*, not proof that BEETLE's physical target has
  enough accessible free RAM. Do not place the structure on the FM1 audio ISR stack.
- Each PCM window covers 64 samples at 22.05 kHz (about 2.9 ms); two windows
  represent about 5.8 ms of source audio. Storage, decoder and main loop must
  be scheduled sufficiently ahead of the real-time DMA renderer.
- All data buffers are fixed size. The render function never calls the storage
  reader, decoder, allocator or logger. Its work is bounded per output frame.
- `beetle_bd4_stream_start` refuses an unprimed voice instead of silently
  starting it at the wrong musical time. **Sample-accurate sequencer lookahead
  and synchronized voice triggering are NOT implemented yet.**
- When an artificially stalled prefetch starves the test renderer, underruns
  are counted. The renderer fades the missing signal, and the background worker
  skips expired frames to resume. **This is error handling, not a claim of
  transparent playback through a real underrun.**

## Tested

Run from the top-level repository:

```bash
python tests/dpcm4/run_prefetch.py
python tests/dpcm4/test_codec.py
python tests/dpcm4/test_storage_integration.py
```

The first test generates thirteen original BD4S sources, validates C rendering
against the Python-decoded reference at every output frame, runs six simultaneous
voices without underruns when prefetch is serviced, exercises 1-, 2-, 63-, 64-,
65- and 257-frame edge cases, and plays a 20-second individual file from start
to finish. It also injects missing reads, verifies underrun reporting and
recovery, and tests invalid source headers. UndefinedBehaviorSanitizer is
included in its host compiler command.

The existing stage-3 offline flash simulation separately fits **20.06 seconds
in total across 13 slots**, with a MOUNTAINKING-sized project and eleven
NUGGIE-sized projects. It **does not** demonstrate that every possible combination
of 12 full projects and 20 seconds will fit.

## Still required before a physical FM1 .fwsc

1. Establish real target RAM/linker ownership and verify the additional 2.8 KiB
   does not clash with audio scratch, display/USB buffers, or legacy 6 KiB
   project-owned one-shot RAM.
2. Prove which physical flash regions can be used by transactional Storage V2,
   with independent OTA and factory-recovery verification. The offline model
   must **not** be wired to `0x97000..0xDEFFF` in an already provisioned device
   without safe migration and a complete external backup.
3. Implement and verify the single-producer/single-consumer synchronization
   between real FM1 audio ISR and main loop. Current lab APIs are used with
   serialized access and are not a concurrency proof.
4. Implement sequencer lookahead, retriggers, sample-accurate note placement,
   coexistence with the existing six drumvoices and eight synthvoices, mixer
   gain/panning, project links and legacy V22/V23 migration.
5. Add a USB protocol with capability negotiation, CRC-checked chunk upload,
   nonblocking final commit, readback, and an explicit `STORAGE FULL` refusal.
6. Measure worst-case DMA deadlines with 6 drumvoices, 8 synthvoices, USB
   multitrack, full-screen LCD activity and simultaneous user samples. Test
   MOUNTAINKING transitions, long playback and unplug/replug/power failure.
7. Only then compile, package and independently hardware-validate a new firmware
   image. Product identity must stay **`FM-1_2123`** (V1.3.8).

## Release provenance

- All 826 V1.4.8 baseline files remain in the stage-4 source ZIP unchanged,
  except explicitly documented README/CHANGELOG additions.
- The previous **BEETLE V1.4.8 experimental .fwsc and WebInstaller are unchanged**.
  No V1.4.9 .fwsc is produced; pretending that host playback is native FM1
  playback would be incorrect and unsafe.
- Audio diagnostics for earlier DMA glitches remain parked for a later phase,
  as requested. They must be integrated before a production-quality claim.
