#/bin/bash

./symmetri/gui/generate_icon $(echo $((1 + $RANDOM % 9))) icon.svg
source ../.github/workflows/macos/create_icons.sh
svg_to_icns icon.svg
rm -r Farbart.app
mkdir -p Farbart.app/Contents Farbart.app/Contents/MacOS Farbart.app/Contents/Resources
cp icons/icon.icns Farbart.app/Contents/Resources/farbart.icns
cp ../.github/workflows/macos/Info.plist Farbart.app/Contents/Info.plist
cp symmetri/gui/Farbart Farbart.app/Contents/MacOS/Farbart
tar -czf Farbart.tar.gz Farbart.app
