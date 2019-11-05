#!/bin/bash
IDEVER="1.8.10"
WORKDIR="/tmp/autobuild_$$"
mkdir -p ${WORKDIR}
# Install Ardino IDE in work directory
TARFILE="${HOME}/Downloads/arduino-${IDEVER}-linux64.tar.xz"
if [ -f ${TARFILE} ]
then
    tar xf ${TARFILE} -C ${WORKDIR}
else
    exit -1
fi
# Create portable sketchbook and library directories
IDEDIR="${WORKDIR}/arduino-${IDEVER}"
LIBDIR="${IDEDIR}/portable/sketchbook/libraries"
mkdir -p "${LIBDIR}"
export PATH="${IDEDIR}:${PATH}"
cd ${IDEDIR}
which arduino
# Configure board package
arduino --pref "boardsmanager.additional.urls=https://adafruit.github.io/arduino-board-index/package_adafruit_index.json" --save-prefs
arduino --install-boards "adafruit:nrf52"
BOARD="adafruit:nrf52:feather52840"
arduino --board "${BOARD}" --save-prefs
CC="arduino --verify --board ${BOARD}"
arduino --pref "compiler.warning_level=none" \
--pref "custom_softdevice=feather52840_s140v6" \
--pref "custom_debug=feather52840_l0" \
--pref "update.check=false" \
--pref "editor.external=true" --save-prefs
cd ${IDEDIR}/portable/sketchbook
#ln -s ~/Sync/usbmseble/USBMSEBLE .
#find USBMSEBLE/ -name '*.ino' -print0 | xargs -0 -n 1 $CC
# Install for SparkFun NRF52840
#cp -R ~/Sync/nRF52840_Breakout_MDBT50Q/Firmware/Arduino/variants/ ${IDEDIR}/portable/packages/adafruit/hardware/nrf52/0.10.1/
#cat ~/Sync/nRF52840_Breakout_MDBT50Q/Firmware/Arduino/sparkfun_boards.txt >>${IDEDIR}/portable/packages/adafruit/hardware/nrf52/0.10.1/boards.txt
