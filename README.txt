simpleRTEMS_HTTPClient

Supported Platform
==================
Myriad2 - This example works on Myriad2: ma2150, ma2450 silicon

Overview
==========
The example initializes the network stack in POSIX_Init, prints out networks statistics and finally creates and starts a thread. Part of this initialization involves
getting an IP address from the network DHCP and obtaining DNS servers and default gateway. 
The thread will resolve an address and create a TCP connection against a www.google.com. It will issue a HTTP GET request and wait for the response. The request response
will be printed out.

Hardware needed
==================
Myriad2 - This software should run on MV182 board. 
Ethernet connection - MV182 must be connected to a network with a DHCP server available. 

Build
==================
Please type "make help" if you want to learn available targets.

!!!Before cross-compiling make sure you do a "make clean"!!!

Myriad2 - To build the project please type:
     - "make clean"
     - "make all MV_SOC_REV={Myriad_version}"

Where {Myriad_version} may be ma2150 or ma2450.
The default Myriad revision in this project is ma2450 so it is not necessary 
to specify the MV_SOC_REV in the terminal.

Setup
==================
Myriad2 silicon - To run the application:
    - open terminal and type "make start_server"
    - open another terminal and type "make debug MV_SOC_REV={Myriad_version}"

Where {Myriad_version} may be ma2150 or ma2450.
The default Myriad revision in this project is ma2450 so it is not necessary 
to specify the MV_SOC_REV in the terminal.

Expected output
==================
UART:
UART: rtems_leon_greth_gbit_driver_setup RTEMS_SUCCESSFUL
UART: dhcpc: gr_eth: inet: 192.168.86.16   mask: 255.255.252.0
UART:              srv: 192.168.85.30     gw: 192.168.85.1
UART:
UART: Resolving name to IP addresses:
UART:
UART:                 *Host Address 0: 216.58.211.132
UART:
UART: Connection is successful to 216.58.211.132.
UART: HTTP request to www.google.com: 18 bytes sent
UART: HTTP response:
UART:
UART: ************************************************************************
UART:
UART: HTTP/1.0 302 Found
UART: Location: http://www.google.ie/?gws_rd=cr&ei=tvrhVKbZDITa7gaZp4E4
UART: Cache-Control: private
UART: Content-Type: text/html; charset=UTF-8
UART: Set-Cookie: PREF=ID=467f11172c49746a:FF=0:TM=1424095926:LM=1424095926:S=
UART: DPjklbce3hTvcVT4; expires=Wed, 15-Feb-2017 14:12:06 GMT; path=/; domain=
UART: .googlee3hTvcVT4; expires=Wed, 15-Feb-2017 14:12:06 GMT; path=/; domain=.google.com
UART: Set-Cookie: NID=67=AsrUNOt28wvuY_UutIfhd3pvGd-_fUvvWSa0jokUjbbyVcJptFt2m
UART: IWLXcCF5qoOPD-HgOMf8KlrDv3ar8rFvVBbgssczamXmxo841zBo3iV6r0bSXQCu941RIUru
UART: k0f; expires=Tue, 18-Aug-2015 14:12:06 GMT; path=/; domain=.google.com; Http expires=Tue, 18-Aug-2015 14:12:06 GMT; path=/; domain=.google.com; HttpOnly
UART: P3P: CP="This is not a P3P policy! See http://www.google.com/support/acc
UART: ounts/bin/answer.py?hl=en&answer=151657 for more info."
UART: Date: Mon, 16 Feb 2015 14:12:06 GMT
UART: Server: gws
UART: Content-Length: 256
UART: X-XSS-Protection: 1; mode=block
UART: X-Frame-Options: SAMEORIGIN
UART: Alternate-Protocol: 80:quic,p=0.08
UART:
UART: <HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=u
UART: tf-8">
UART: <TITLE>302 Moved</TITLE></HEAD><BODY>
UART: <H1>302 Moved</H1>
UART: The document has moved
UART: <A HREF="http://www.google.ie/?gws_rd=cr&amp;ei=tvrhVKbZDITa7gaZp4E4">he
UART: re</A>.
UART: </BODY></HTML>
UART:
UART: ************************************************************************
UART:


User interaction
==================

