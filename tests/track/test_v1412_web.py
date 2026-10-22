#!/usr/bin/env python3
"""Optional real-Chromium test of the stand-alone BEETLE WebSimulator.

The browser exposes a test hook in memory only while this test runs;
there is no hook in the shipped simulator file.
Requires: pip install playwright; Chromium installed.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parents[2]
html = (root / 'simulator.html').read_text()
anchor = '})(TRACK_CORE,TRACK_AUDIO);</script>'
assert html.count(anchor) == 1
html = html.replace(anchor,
    'window.__beetleTest={model,route,whiteDown,editShortcut,render};' + anchor)
with sync_playwright() as pw:
    browser = pw.chromium.launch(headless=True, executable_path='/usr/bin/chromium',
                                  args=['--no-sandbox', '--disable-dev-shm-usage'])
    try:
        page = browser.new_page()
        errors = []
        page.on('pageerror', lambda exc: errors.append(str(exc)))
        page.set_content(html, wait_until='load', timeout=60000)
        page.evaluate("__beetleTest.model.screen='tracker';__beetleTest.render()")
        assert page.evaluate('__beetleTest.model.noteRecord') is True
        page.evaluate("__beetleTest.route('REC')")
        assert page.evaluate('__beetleTest.model.noteRecord') is False
        page.evaluate("__beetleTest.whiteDown(0)")
        assert page.evaluate('__beetleTest.model.row') == 0
        page.evaluate("__beetleTest.route('REC')")
        page.evaluate("__beetleTest.whiteDown(0)")
        assert page.evaluate('__beetleTest.model.row') == 1
        page.evaluate("__beetleTest.model.screen='punch';__beetleTest.render();__beetleTest.route('REC')")
        assert page.evaluate('__beetleTest.model.record') is True
        assert page.evaluate('__beetleTest.model.noteRecord') is True
        page.evaluate("__beetleTest.model.screen='song';__beetleTest.render()")
        assert page.evaluate('__beetleTest.model.record') is False
        page.evaluate("__beetleTest.model.songRow=0;__beetleTest.model.project.song="
                      "[{pattern:0,lanes:[128,128,128,128,128,128],repeats:1},"
                      "{pattern:1,lanes:[128,128,128,128,128,128],repeats:1}];"
                      "__beetleTest.model.songRange=[0,1];__beetleTest.editShortcut('copyLane')")
        assert page.evaluate('__beetleTest.model.songRowsClip.length') == 2
        page.evaluate("__beetleTest.model.songRow=1;__beetleTest.editShortcut('pasteLane')")
        assert page.evaluate('__beetleTest.model.project.song.length') == 3
        assert not errors, errors
        print('PASS V1.4.12 WebSimulator (Chromium): REC modes, song row copy/paste, render')
    finally:
        browser.close()
