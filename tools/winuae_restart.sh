#!/bin/sh
# Restart WinUAE with the CommandConquer config and wait for AmiAgent (clean slate after a crash).
cd /c/Workprojects/AmigaGAMES/CommandConquer
taskkill //F //IM winuae64.exe >/dev/null 2>&1; sleep 3
powershell -Command "Start-Process 'C:\Program Files\WinUAE\winuae64.exe' -ArgumentList '-f','C:\amiga-dev\winuae\CommandConquer_A1200_OS32_net.uae','-s','use_gui=no'"
for i in $(seq 1 40); do python tools/amiga_remote.py --config C:/amiga-dev/amiagent-winuae.cfg ping 2>&1 | grep -q '^OK' && echo "WinUAE up" && exit 0; sleep 5; done
echo "WinUAE did not come up"; exit 1
