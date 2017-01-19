Open new terminal
sudo su
tar xvf tools_rel.tar
toolspackinstall.sh

source ~/.bashrc_tools
 
1) Required only once on booting laptop
unloadmod

2)Rescan PCI
rescan
 
3)Load modules and download firmware
utfgo
 
4)Run nart or athtestcmd(tcmd) under
a)      tcmd/athtestcmd
b)      nart/nart.out -console
 
5)Unload modules
unload

