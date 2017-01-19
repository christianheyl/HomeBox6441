Suggested sequence of operations
================================

FPGA setup
----------
1) Insert the SDIO card into the host SDIO slot
2) Power on the target making sure that the FPGA was correctly programmed.
3) Start the bsp on the target.

Host Startup Sequence
---------------------
1) Load the host side driver using the script.
	<PATH>/loadAR6000

2) Use the appropriate download script to download the application at the
   desired location in the target. Use the script 'download.ram.sh' to
   download and execute the code from RAM in the SD1x or TB111 boards. The 
   script 'download.sram.sh' can be used to download and execute the code 
   from SRAM in TB111 boards. Please ensure that the app.bin file is linked 
   for the correct memory (RAM or SRAM).

3) Set the 'bypasswmi' flag if the application requires that
	echo 1 > /sys/module/ar6000/bypasswmi

4) Bring the interface up
	ifconfig eth1 192.168.1.2 netmask 255.255.255.0 broadcast 192.168.1.255 up

5) Execute the application
	<PATH>/tests/mboxping -t 0 -r 0 -s 100 -c 10

Host Shutdown Sequence
----------------------
1) Bring the interface down
	ifconfig eth1 down

2) Unload the driver
	<PATH>/loadAR6000 unloadall

Applications and Utilities
==========================
For help with a particular command, simply type in the command without any options. 

1) mboxping: Tests the connectivity to the board. Also measures bidirectional throughput.
Notes:
a) Can run on any mailbox as long as you keep in mind the mailbox size limitations.
b) Set the bypasswmi flag to 1. This has to be done before you bring up the interface. 

2) floodtest: Measures the unidirectional throughput in both tx and rx directions.
Notes:
a) Runs on mailbox 1
b) Set the bypasswmi flag to 1. This has to be done before you bring up the interface (Currently, you need to restart the app on the target if you want to switch from meauring rx throughput to tx throughput or vice versa).

3) bmiloader: Loads the apps/registers into the target's memory.
Notes:
a) It has to be done before you bring up the interface.

4) flashloader: loads the apps into target's flash. 
Notes:
a) Set the bypass flag to 1. This has to be done after you bring up the interface.

