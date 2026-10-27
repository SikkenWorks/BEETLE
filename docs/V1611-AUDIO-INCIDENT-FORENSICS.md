# BEETLE V1.6.11 — Severe loud-output incident, source-level investigation

## Operator report
During drum-channel copy in a large Song Chain, audio crackled, then became extremely loud and apparently disregarded physical MASTER, persisting across the chain. This is a **hearing/equipment hazard**. User reproduction is the only available hardware evidence; no FM1 recording/ADC/DMA trace has been provided to prove the exact electrical root cause.

## Confirmed problematic paths in V1.6.10 source

1. `bringup.c` sampled MASTER using the foreground loop (20 ms target), but that loop calls `track_fm1_poll`, which can call `beetle_bd4_live_poll` to read flash/verify PCM chunks for up to six sample voices, plus project UI operations. While the foreground stalls, the ALNK0 ISR in `audio_target.c` continues using the last published MASTER gain, with no freshness check. **Confirmed code-level unsafe stale-gain path.** Whether the user's potentiometer actually changed while this path was blocked cannot be established here.
2. `audio_target.c::native_budget_late` previously reduced ETCH voice budget; the BEETLE ISR renders `track_chip`, so this shedding did not relieve the active Song/Drum CPU workload. A missed DMA deadline could recur, and `bringup.c` automatically restarted a tripped audio engine. **Confirmed code-level unsafe DMA/restart path**, consistent with crackling/repeating bursts; exact physical amplifier response unmeasured.
3. `track_runtime.h` previously permitted SEQ+OP1 and SEQ+OP3 copy operations while playback ran; these held global interrupt disable across copying pattern, punch, and FX state while active Song Mode composes/rendered patterns in ALNK0. **Confirmed concurrency and deadline risk** associated with the user's action.
4. `track_lane_clipboard.h` included CH6 NOTE3 bytes when copying/pasting non-drum lanes. **Confirmed cross-lane clipboard bug** (silent drum event corruption), not established as the cause of loud playback.

5. **USB multitrack exception:** `audio_target.c` sends `track_usb_stem_pcm` directly when `uac_multitrack` is on; those six dry USB streams intentionally bypass the final hardware MASTER, while stereo USB/analog are post-MASTER. If the reported peak was heard through a computer mixing all six USB stems, turning the FM1 MASTER down would not affect that output. This is a *separate confirmed routing fact*, not proof of what the user heard. Any reproduction must document analog vs stereo USB vs six-stem USB monitoring, mixer sum and gain staging.

## Corrections

- At the final DAC stage, apply fresh, bounded physical MASTER data for every output frame; stale >80ms or invalid ADC immediately forces zero. Physical gain upward movement is bounded while downward movement is immediate.
- On first confirmed DMA ownership/deadline failure, stop transport, disable analog output and latch AUDIO STOP. Never automatically restart BEETLE audio; manual recovery requires HOME+PLAY, and users should reduce MASTER and unplug amplification first.
- Reject lane/pattern copy while running; keep Song/live data immutable during playback.
- Preserve NOTE3 only in actual CH6 drum lane copy/paste.

## Limitations and required physical acceptance before clearing safety hold

- Host software and package checks cannot establish actual amplifier or DAC peak voltage, analog PC6 switching transients, or FM1 ADC correctness. This is NOT a certified output limiter and cannot replace analog measurement/monitoring.
- First test without headphones, powered speakers or amplifiers; back up flash and use a verified recovery route. Bench tests by a hardware-qualified tester should measure output into a suitable load with instrumentation and gain staged at minimum, validate knob across 0..1023, stress simultaneous six-voice BD4 and 64-row Song playback, force ADC timeouts/DMA delays, and exercise copy keys mid-playback.
- The responsible source hazards are fixed; **the exact physical cause of the reported 5× loudness has not been proven**. If any abnormal loudness occurs, immediately power off/disconnect and collect diagnostic logs; do not increase volume or reconnect speakers to investigate.
