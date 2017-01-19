rm -rf toolspack
rm -f toolspack.tar
mkdir -p toolspack/nart
mkdir -p toolspack/tcmd
mkdir -p toolspack/scripts
cp -rf $WORKAREA/host/tools/systemtools/testcmd_tlv/Linux/athtestcmd toolspack/tcmd/
cp -rf $WORKAREA/host/tools/systemtools/art2_peregrine/src/art2/.output/* toolspack/nart/
cp -f $WORKAREA/host/tools/systemtools/tools/eepromUtil/qc98xx_template/boardData*.bin toolspack/nart/
cp -f $WORKAREA/host/support/loadAR9888UTF.sh toolspack/scripts/
cp -f $WORKAREA/host/tools/.bashrc_tools toolspack/
cp /usr/local/lib/*.ko toolspack
cp $WORKAREA/target/.output/AR9888/hw.$TARGET_VER/bin/utf.bin toolspack/utf.bin
cp $WORKAREA/target/.output/AR9888/hw.$TARGET_VER/bin/athwlan.bin toolspack/athwlan.bin
cp $WORKAREA/target/.output/AR9888/hw.$TARGET_VER/bin/otp.bin toolspack/otp.bin
cp $WORKAREA/../../scripts/fakeBoardData_AR6004_AP.bin toolspack/fakeBoardData_AR6004.bin
cp -f $WORKAREA/host/os/linux/tools/bmiloader/bmiloader toolspack/bmiloader
cp -f $WORKAREA/host/os/linux/tools/athdiag/athdiag toolspack/athdiag
cp $WORKAREA/../../scripts/rc.tools toolspack/scripts/rc.tools
cp $WORKAREA/../../scripts/rc.wlan toolspack/scripts/rc.wlan
tar -cf toolspack.tar toolspack
mkdir -p tools_rel
mv toolspack.tar tools_rel
cp README.txt tools_rel
cp toolspackinstall.sh tools_rel 
tar -cf tools_rel.tar tools_rel
rm -rf toolspack
rm -rf tools_rel
