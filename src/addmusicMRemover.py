#!/usr/bin/env python3
"""
AddmusicM Remover - Python port of addmusicMRemover.pl

This script removes AddmusicM data from Super Mario World ROMs.
It deletes music sequence data and PCM sample data that was inserted by AddmusicM.

Usage: python addmusicMRemover.py <rom_file>
"""

import sys
import os
import struct
import argparse
from pathlib import Path

# Constants (same as Perl version)
DIR = "./music/"
DIRB = "./brr/"
FREE_RAM = 0x7EC100

CHECK_POINT = 0x80200
ARAM_L = 0x5400
SEQ_OFS = 0xFE000
PCMSET_ADD = 0xFCF00
PCMDATA = 0xFD000
PCM_SET = 0x00

def main():
    parser = argparse.ArgumentParser(description='Remove AddmusicM data from SMW ROM')
    parser.add_argument('rom', help='ROM file to process')
    args = parser.parse_args()

    rom_path = Path(args.rom)

    # Ensure .smc extension
    if rom_path.suffix.lower() != '.smc':
        rom_path = rom_path.with_suffix('.smc')

    # Check if ROM exists
    if not rom_path.exists():
        print("ROM not found.")
        input("Press ENTER to continue.")
        sys.exit(1)

    # Check ROM size
    rom_size = rom_path.stat().st_size
    if rom_size < 1000000 or rom_size > 4200000:
        print("The ROM size must be between 1 and 4 MB.")
        input("Press ENTER to continue.")
        sys.exit(1)

    # Open ROM file for reading and writing
    try:
        with open(rom_path, 'rb+') as rom_file:
            print("Processing ROM...")
            insert_seq(rom_file)
            print()
            insert_pcm(rom_file)
            print()
            print("Success!")
            print()
            input("Press ENTER to continue.")

    except Exception as e:
        print(f"Error processing ROM: {e}")
        sys.exit(1)

def insert_seq(rom_file):
    """Delete music sequence data"""
    start_aram = ARAM_L
    snes_addr = SEQ_OFS
    delete_music(snes_addr, rom_file)

def insert_pcm(rom_file):
    """Delete PCM sample data"""
    addr_pc = 0x7CE00  # PCMOFS position
    snes_addr = pc2snes(addr_pc)
    delete_pcm(snes_addr, rom_file)

def delete_pcm(snes_addr, rom_file):
    """Delete PCM sample data from PCMOFS table"""
    zero = b'\x00\x00\x00'
    pc_addr = snes2pc(snes_addr)
    print("Deleting sample data...")

    for i in range(0xFF):  # 0..0xFE
        offset = pc_addr + (i * 3)
        rom_file.seek(offset)
        buf = rom_file.read(3)
        if len(buf) != 3:
            continue

        # Convert 3 bytes to SNES address
        snes_addr_sample = struct.unpack('<I', buf + b'\x00')[0] & 0xFFFFFF
        if snes_addr_sample == 0:
            continue

        # Delete RATS data
        delete_rats_data(snes2pc(snes_addr_sample), rom_file)

        # Zero out the entry
        rom_file.seek(offset)
        rom_file.write(zero)

def delete_music(snes_addr, rom_file):
    """Delete music data from SEQOFS table"""
    zero = b'\x00\x00\x00'
    pc_addr = snes2pc(snes_addr)
    print("Deleting music data...")

    for i in range(0x100):  # 0..0xFF
        offset = pc_addr + (i * 3)
        rom_file.seek(offset)
        buf = rom_file.read(3)
        if len(buf) != 3:
            continue

        # Convert 3 bytes to SNES address
        snes_addr_seq = struct.unpack('<I', buf + b'\x00')[0] & 0xFFFFFF
        if snes_addr_seq == 0:
            continue

        # Delete RATS data
        delete_rats_data(snes2pc(snes_addr_seq), rom_file)

        # Zero out the entry
        rom_file.seek(offset)
        rom_file.write(zero)

def delete_rats_data(pc_addr, rom_file):
    """Remove RATS compressed data block"""
    zero = b'\x00'

    # Check for RATS tag (8 bytes before data)
    rats_offset = pc_addr - 8
    rom_file.seek(rats_offset)
    rats_data = rom_file.read(4)

    if rats_data != b'STAR':  # RATS in little endian would be RATS, but wait...
        # Actually, RATS is stored as "STAR" in big endian or what?
        # The Perl code checks for "53544152" which is "STAR" in hex
        if rats_data != b'STAR':
            return 0

    # Read size and parity
    size_data = rom_file.read(2)
    parity_data = rom_file.read(2)

    size = struct.unpack('<H', size_data)[0]
    parity = struct.unpack('<H', parity_data)[0]

    # Verify RATS tag
    if size + parity != 0xFFFF:
        return 0

    # Zero out the RATS block (size + 9 bytes total)
    rom_file.seek(rats_offset)
    for _ in range(size + 9):
        rom_file.write(zero)

    return 1

def snes2pc(snes_addr):
    """Convert SNES address to PC address"""
    return (((snes_addr & 0x7FFFFF) // 2 & 0xFF8000) + (snes_addr & 0x7FFF) + 0x200)

def pc2snes(pc_addr):
    """Convert PC address to SNES address"""
    snes_addr = (((pc_addr - 0x200) * 2 & 0xFF0000) + ((pc_addr - 0x200) & 0x7FFF) + 0x8000)
    if pc_addr >= 0x380200:
        snes_addr += 0x800000
    return snes_addr

def read_snes_address(pc_addr, rom_file):
    """Read 3-byte SNES address from ROM"""
    rom_file.seek(pc_addr)
    data = rom_file.read(3)
    if len(data) != 3:
        return 0

    # Little endian 24-bit
    return struct.unpack('<I', data + b'\x00')[0] & 0xFFFFFF

if __name__ == '__main__':
    # Check if run directly
    if len(sys.argv) == 1:
        print("\nDo not run this program by itself.  AddmusicK will use it as necessary.\n")
        print("Incidentally, if you don't need to convert any AMM ROMs, feel free to delete")
        print("this file, as it only has that one purpose.\n")
        input("Press ENTER to continue.")
        sys.exit(0)

    main()