#!/bin/sh


./stop.sh

sleep 3
./fightServer --sid 151 --fd 10000 --loglv 0 --logsize 52428800 --logdays 5 --logname "log" --cfg "../server_config/server_config.lua"

