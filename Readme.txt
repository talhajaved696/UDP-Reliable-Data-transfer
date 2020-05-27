# Reliable UDP Data Transfer

* Any type of file (audio/video) etc.
* Reliability added using:
1) Sequence numbers
2) Retransmission (selective repeat)
3) Window size of 5-10 UDP segments (stop n wait)
4) Re ordering on receiver side

---------------------------------------------------------------------------------
------------------------------ Reliable UDP Transfer ---------------------------- 
---------------------------------------------------------------------------------
SERVER:
o Open the LINUX terminal.
o Move to the directory where you have saved your file Server.c
o Now type ./<filename> <port_number>
o filename is received at the server side with the new name "NEW_(filename sent)"
---------------------------------------------------------------------------------
CLIENT:
o Open the LINUX terminal.
o Move to the directory where you have saved your file client.c
o Now type ./<filename> <filename> <port_number>
o filename is the name of the file client want to send to the receiver.
o File will be transmitted in this way.
---------------------------------------------------------------------------------
