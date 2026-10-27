# BEETLE V1.6.8 — Project selector input fix

## Root-level bug fixed

The initial NEW/LOAD boot chooser previously ran NEW PROJECT immediately on a SEL rising edge. This happened before the rest of a two-key chord could be observed. Changing the DELETE modifier (EDIT+SEL to EDIT+GLO) could not fix an earlier handler executing NEW.

`beetle_boot_select_confirm()` now confirms a standalone SEL only on release, provided no other key was held with SEL during that press. A second key cancels the pending action even if released before SEL. Both EDIT-first/SEL-second and SEL-first/EDIT-second are tested. After opening LOAD PROJECT, only EDIT+SEL on an occupied slot enters DELETE PROJECT?. The original gesture cannot directly confirm YES: all chord keys must be released before a fresh SEL event can confirm. NO is default.

## Tests

Run `cc -std=c11 -Wall -Wextra -Werror tests/track/test_v168_project_selector.c -o /tmp/beetle-selector-test && /tmp/beetle-selector-test`. Also run `python tests/track/test_help_contract.py` and `python tests/track/test_v167_ch6_preview_and_delete.py`.

## Physical validation checklist

1. On NEW/LOAD boot chooser hold EDIT then press SEL: no new project should start. Repeat with SEL first, then EDIT.
2. Select LOAD with K1. Press/release SEL **alone**, opening its twelve-slot list.
3. Highlight an occupied slot; hold EDIT then press SEL: DELETE PROJECT? appears and NO is selected. Repeat with SEL then EDIT.
4. Release both keys, confirm NO with a new SEL tap. Check no project was deleted.
5. Press EDIT+GLO: it should not open DELETE or NEW. Confirm that normal standalone SEL still loads the correct slot.
6. Only on a disposable test project, deliberately select YES and verify exactly one slot is deleted. Make independent project and sample backups first.

Passing native/host tests and package audits does not constitute physical FM1 test or OTA/recovery verification.
