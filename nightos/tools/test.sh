#!/bin/sh
cd "$(dirname "$0")/.." && exec python3 tools/drive.py build/nightos.elf
