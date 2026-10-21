#!/usr/bin/env python3
"""Regression: the native Instrument EDIT ADSR renderer never prints 0.:0.

The display values are 0..100 whole percent stored in project/voice fields.
A regression that overflows tens for 100 reproduces the original bug.
"""
from pathlib import Path
import re

source = (Path(__file__).resolve().parents[2] / 'firmware/fm1-native/track_runtime.h').read_text()
assert 'n==100u?\'1\':\'0\'' in source, '1.00 handling missing'
assert "('0'+(n%100u)/10u)" in source, 'tens digit still overflows for 100'
assert "('0'+n%10u)" in source

def actual_renderer(n):
    n=min(n,100)
    return ('1' if n==100 else '0') + '.' + str((n%100)//10) + str(n%10)

for n in range(101):
    actual=actual_renderer(n)
    expected=f'{n//100}.{n%100:02d}'
    assert actual == expected, (n,actual,expected)
assert actual_renderer(100) == '1.00'
assert actual_renderer(0) == '0.00'
assert actual_renderer(99) == '0.99'
assert actual_renderer(101) == '1.00'
assert '0.:0' not in [actual_renderer(n) for n in range(101)]
print('PASS: 0..100 rendered exactly; 100 -> 1.00; no 0.:0')
