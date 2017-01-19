
Usage of the commands:

  #  Show uboot environment serverip
    uboot_env --get --name serverip

  #  read from sysconfig and write it to htf.txt
    read_img sysconfig htf.txt

  #  write htf.txt to sysconfig, forward direction, update uboot environment
    write_img htf.txt sysconfig 0 1


