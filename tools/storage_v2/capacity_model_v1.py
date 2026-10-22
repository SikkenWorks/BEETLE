#!/usr/bin/env python3
"""BEETLE independent capacity study; never writes firmware, flash or projects.

All algorithms in this file are project-specific calculations, not copied from
other FM1 firmware. Inputs are representative project payload lengths measured
from BEETLE test fixtures. Compression output sizes are exploratory desktop
numbers, not native FM1 compression guarantees.
"""
from dataclasses import dataclass
from math import ceil

FLASH = 288 * 1024
SECTOR = 4096
SECTORS = FLASH // SECTOR
PROJECT_SLOTS = 12
INDEX_SECTORS = 2
GC_RESERVE_SECTORS = 4
SAMPLE_RATE = 22050
SAMPLE_BLOCK_FRAMES = 256
SAMPLE_BLOCK_BYTES = 132  # planning assumption: 4-byte header + 128 bytes 4-bit
RECORD_HEADER = 32
PROGRAM_PAGE = 256

@dataclass(frozen=True)
class Scenario:
    name: str
    project_sizes: tuple[int, ...]  # uncompressed or externally compressed measurement


def sample_size(seconds: int) -> int:
    return ceil(seconds * SAMPLE_RATE / SAMPLE_BLOCK_FRAMES) * SAMPLE_BLOCK_BYTES


def packed_project_bytes(sizes: tuple[int, ...]) -> int:
    # A/B copies, packed to 256B program pages, not a sector per record.
    return sum(2 * ceil((size + RECORD_HEADER) / PROGRAM_PAGE) * PROGRAM_PAGE for size in sizes)


def evaluate(case: Scenario) -> dict:
    assert len(case.project_sizes) == PROJECT_SLOTS
    records = packed_project_bytes(case.project_sizes)
    bookkeeping = (INDEX_SECTORS + GC_RESERVE_SECTORS) * SECTOR
    remaining = max(0, FLASH - records - bookkeeping)
    sample_sectors = remaining // SECTOR
    capacity_bytes = sample_sectors * SECTOR
    max_secs = max(n for n in range(0, 30) if sample_size(n) <= capacity_bytes)
    return {'scenario': case.name,
            'project_AB_bytes': records,
            'bookkeeping_reserved_bytes': bookkeeping,
            'sample_budget_bytes': capacity_bytes,
            'capacity_seconds_whole': max_secs,
            '10sec_possible': sample_size(10) <= capacity_bytes,
            '20sec_possible': sample_size(20) <= capacity_bytes}


def main():
    mountain, nuggie = 8779, 1724
    mountain_proof, nuggie_proof = 2670, 442
    scenarios = [
        Scenario('1 large + 11 small, current codec', (mountain,) + (nuggie,) * 11),
        Scenario('1 large + 11 small, desktop compression measurement',
                 (mountain_proof,) + (nuggie_proof,) * 11),
        Scenario('12 mountain-sized, current codec', (mountain,) * 12),
        Scenario('12 mountain-sized, desktop compression measurement', (mountain_proof,) * 12),
        Scenario('12 maximum-size incompressible projects', (12256,) * 12),
    ]
    print(f'FM1 BEETLE project region: {FLASH} bytes = {SECTORS} sectors')
    for n in (10, 15, 20):
        s = sample_size(n)
        print(f'{n:2d} sec 22.05kHz DPCM4 planning: {s} bytes, {ceil(s/SECTOR)} sectors')
    print('NOTE: Results omit per-sample metadata/padding/fragmentation; upper estimates only.')
    print('NOTE: Free capacity alone never authorizes partition migration or NOR writes.')
    for scenario in scenarios:
        print(evaluate(scenario))
    assert sample_size(10) > 100_000
    assert not evaluate(scenarios[-1])['10sec_possible']
    assert evaluate(scenarios[1])['10sec_possible']
    assert evaluate(scenarios[1])['20sec_possible']
    print('PASS: capacity scenarios and fundamental worst-case refusal')


if __name__ == '__main__':
    main()
