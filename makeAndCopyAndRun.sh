make clean
make

#propagate error from script
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

echo "Copying dylib"
cp Release/realbot_mm_i386.dylib "/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/realbot/dll/"

echo "Removing reallog.txt"
# remove log file
rm "/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/reallog.txt"

# and create new one
touch "/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/reallog.txt"

# run
#"/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/hl.sh" -game cstrike --listen +map cs_italy +maxplayers 32
#"/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/hl.sh" -game cstrike --listen +map de_prodigy +maxplayers 32
#"/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/hl.sh" -game cstrike --listen +map de_dust +maxplayers 32

"/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/hl.sh" -game cstrike --listen +map as_oilrig +maxplayers 32

#"/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/hl.sh" -game cstrike --listen +map fy_iceworld +maxplayers 32
#"/Users/shendriks/Library/Application Support/Steam/SteamApps/common/Half-Life/hl.sh" -game cstrike --listen +map cs_assault +maxplayers 32
