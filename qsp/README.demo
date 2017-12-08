SNR And QSP  Demo Application

Introduction
This document describes  the SNR_QSP_Test  demo application usage.
The SNR_QSP_Test  app  creates a simple network with 1 SNR model and 1 x QSP model. The models are connected using a port binding between the SNR model’s  PCH.ENET  port  and the QSP model’s  PCH.ENET  port.  This allows communication between the 2 models over a simulated ethernet link.  
In this demo the QSP model acts as a server, the SNR model acts as a client.   The QSP model acts as a dhcp server and will supply the SNR model with an ip address at boot time.  Also, the QSP server implements a tftp server to allow simple file transfers,  a websever (lighttpd) to allow  testing of http(s) functionality. SFTP, SCP, SSH and various other basic networking  protocols are also supported.

Booting And Running the Demo
The demo requires the user to supply the following items
•   Linux kernel and filesystem for QSP, implemented in a compressed disk image (craff format). See document ‘Building Yocto Linux Kernel And Filesystem For QSP Server’   for the full set of steps to build a  Yocto-based Linux for QSP.  The resultant disk image will be in a .craff file

•   Linux kernel and filesystem for SNR. Please refer to site ‘  https://github.com/axxia/meta-axxia/wiki/Environment’   for details on building a Yocto Linux image for SNR.  

•   A BIOS binary image (snr_bios.bin) for SNR. This is provided in ASE in the  ase/images  folder.

Steps to boot the demo
1) In the SNR_QSP_Test folder, create symlinks to the Linux kernel for QSP, Linux kernel for SNR and BIOS binary for SNR. For example:
•  ln –s  <path to QSP  .craff file>   qsp_diskimage.craff
•  ln –s <path to SNR .craff file>    diskimage.craff
•  ln –s <path to snr_bios.bin>   snr_bios.bin

2) Edit file topology-snr-qsp.xml  to  attach  the Linux and bios images using the above symlinks. Set the following  sim parameters eith by directly editing  topology-snr-qsp.xml  using a text editor,  or use the ASE GUI:

•  In snr-bts model simparams:
        <SimParameter id="78" name="pch.sata0.disk_image" value="diskimage.craff"/>
        <SimParameter id="79" name="pch.spi0.nvm_image" value="snr_bios.bin"/>

•	In the qsp model simparams:
        <SimParameter id="10" name="pch.sata0.disk_image" value="qsp_diskimage.craff"/>

 3) Boot the simulation on command line using  following command:
•   asesim –t topology-snr-qsp.xml  -N

At this point, you should see multiple terminals open
•  2 x ‘Partition windows, 1 for SNR, 1 for QSP. These display text output from ASE for each model
•  3 x console windows, 1 for SNR, 2 for QSP. These display terminal output from Linux on each model.  For QSP, the user can choose the serial console window or Textual Graphics Console

4) Wait until both models boot to Linux prompt. Log in with username ‘root’

5) The QSP model has a static IP address assigned to eth0 port – 10.10.0.10.  QSP acts as a dhcp server in the system and will supply address 10.10.0.20 to SNR. Check that both models have correct address using command ‘ifconfig eth0’  in each model’s serial console



Testing Net Services Between SNR and QSP
This section describes some tests that can be done to test various network protocols between the SNR and QSP models
SNR’s eth0 port should have an IP address assigned by the dhcp server on QSP. The address should be 10.10.0.20.  QSP server address is 10.10.0.10. On SNR, try ‘ping 10.10.0.10’   to ping to QSP


TFTP Test
To test tftp file transfers,  QSP runs a tftpserver. There is a file ‘testfile.txt’   in folder /tftpboot in QSP  filesystem that can be transferred to SNR.   In SNR terminal type:
•  tftp  -b 65500 –g –r testfile.txt  10.10.0.10
•  SNR should receive file testfile.txt
Note the ‘-b’  option in tftp command sets the ‘blocksize’ of each tftp transfer to 65500 bytes.  This allows faster transfers compared to the standard blocksize of 512 bytes

SFTP Test
To test  sftp, transfer the file testfile.txt from QSP to SNR with following commands in SNR serial console:
•  sftp root@10.10.0.10
•  cd  /tftpboot
•  get testfile.txt
•  quit
SNR should receive file testfile.txt

HTTP Test
To test http functionality, QSP runs an http server (lighttpd).  The index.html file can be read by SNR using the following command:
•  wget –O - http://10.10.0.10/index.html 
SNR should receive and print index.html  (should print ‘it works’ )

SCP Test
To test scp functionality, transfer file testfile.txt from QSP toSNR using following command  in SNR serial console:
•  scp   root@10.10.0.10:/tftpboot/testfile.txt  ./testfile.txt

SNR should receive file testfile.txt
