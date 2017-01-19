tar -xf toolspack.tar
mkdir -p nart
mkdir -p scripts
mkdir -p /usr/local/lib/tools
mv toolspack/tcmd/athtestcmd tcmd
rm -f /usr/local/bin/nar.out
mv toolspack/nart/nart.out nart
cp -f toolspack/nart/boardData*.bin /lib/firmware
mv toolspack/nart/boardData*.bin nart
mv toolspack/nart/*.so /usr/local/lib/tools
mv toolspack/scripts/rc.tools ~
mv toolspack/scripts/rc.wlan ~
mv toolspack/scripts/*.sh scripts
mv toolspack/*.ko /usr/local/lib
mv toolspack/*.bin /lib/firmware
mv toolspack/bmiloader /usr/local/bin
mv toolspack/athdiag /usr/local/bin
mv toolspack/.bashrc_tools ~
source ~/.bashrc_tools
rm -rf toolspack
