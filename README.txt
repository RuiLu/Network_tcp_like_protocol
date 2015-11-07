PROJECT DOCUMENTATION
=====================
In this assignment, I implement a simplified TCP-like transport layer protocol. My TCP-like protocol can provide reliable, ignored delivery of a stream of bytes. And it can recover from in-network packet loss, packet corruption, packet duplication and can handle with dynamic network delays. As the requirement, I did not consider the situation of congestion or flow control. Besides, I use the stop-and-wait mechanism so there won’t be packet reordering.

PROGRAM FEATURE
===============
1. My TCP-like transport layer protocol doesn’t include initial connection. But I use FIN flag to end the connection.
2. The Sequence number starts from 0, and the Acknowledge number starts from 1. So when the file is transferred successfully, the Sequence Number would be the size of this file, and the Acknowledge number would be the number of packets sent.
3. Retransmission time-out-interval I set is 300ms, which is not calculated by estimatedRTT 
4. I only use ACK and FIN flags.
5. The checksum of my protocol us computed over the TCP header and data.
6. I set up a maximum retransmission number. If the number of retransmission is over it, the stop the whole transferring process.
7. The received file is exactly the same as the sent file, passing the diff test.
8. My protocol actually has 24-byte header. Because I use option filed to store the window size.

USAGE SCENARIOS
===============
Sender:
The command line to invoke data sender is as follows:

java sender <datafilename> <remote_IP> <remote_port> <ack_port_num> <log_filename> <window_size>

which can be implemented like this (tested in local:

$ java sender send.pdf 127.0.0.1 12001 12010 send_log.txt 2024
Send FIN to end to the connection…
Delivery completed successfully
Total bytes sent = 237204
Segments sent = 251
Segment retransmitted = 132

Receiver:
The command line to invoke data receiver is as follows:

java receiver <datafilename> <listening_port> <sender_ip> <sender_port> <log_filename> <window_size>

which can be implemented like this (tested in local:

$java receiver rcv.pdf 12000 127.0.0.1 12010 rcv_log.txt
Receiver is over…
Delivery completed successfully.

The data on log file of sender are as follows:

<timestamp>    <src_ip>   <dest_ip>      <seq> <ack>    <flags> <estimatedRTT>
1446858700309, 127.0.0.1, 192.168.1.145, #117, #234000, 010000, 28.475163602050856
1446858700310, 192.168.1.145, 127.0.0.1, #234000, #118, 010000, 
1446858700615, 192.168.1.145, 127.0.0.1, #234000, #118, 010000, 
1446858700916, 192.168.1.145, 127.0.0.1, #234000, #118, 010000, 
1446858701222, 192.168.1.145, 127.0.0.1, #234000, #118, 010000, 
1446858701242, 127.0.0.1, 192.168.1.145, #118, #236000, 010000, 27.4157681517945
1446858701246, 192.168.1.145, 127.0.0.1, #236000, #119, 010000, 
1446858701448, 127.0.0.1, 192.168.1.145, #118, #236000, 010000, 49.23879713282018
1446858701448, 192.168.1.145, 127.0.0.1, #0, #0, 000001, 
1446858701549, 127.0.0.1, 192.168.1.145, #0, #1, 010000, 55.70894749121766
1446858701549, 127.0.0.1, 192.168.1.145, #2, #0, 000001, 
1446858701448, 192.168.1.145, 127.0.0.1, #0, #0, 010000, 

The data on log file of receiver are as follows:

<timestap>     <src_ip>   <dest_ip>      <seq>    <ack> <flags>
1446858701241, 127.0.0.1, 192.168.1.145, #234000, #118, 010000
1446858701242, 192.168.1.145, 127.0.0.1, #118, #236000, 010000
1446858701447, 127.0.0.1, 192.168.1.145, #234000, #118, 010000
1446858701448, 192.168.1.145, 127.0.0.1, #118, #236000, 010000
1446858701455, 127.0.0.1, 192.168.1.145, #236000, #119, 010000
1446858701549, 127.0.0.1, 192.168.1.145, #0, #0, 000001
1446858701549, 192.168.1.145, 127.0.0.1, #0, #1, 010000
1446858701549, 192.168.1.145, 127.0.0.1, #2, #0, 000001
1446858701556, 127.0.0.1, 192.168.1.145, #0, #3, 010000


TCP STRUCTURE
=============
The structure of my protocol is the same is TCP.
My protocol header has exactly 24 bytes, the extra 4 bytes are used to store the window size. 

SENDER & RECEIVER STATUS
========================
Statuses of sender and receiver are the same as those of stop-and-wait mechanism.

LOSS RECOVERY
=============
It’s pretty simple - wait. If sender doesn’t receive any response packet, it retransmits. If sender receive response packet but with improper Acknowledge number, it retransmits. If receiver doesn’t receive any packet, it just wait.

ADDITIONAL FEATURES
===================
1. In order to achieve the goal that sent file and received file are exactly the same, the receiver should know the exact length of byte data. Therefore, I adopt a 24-byte header structure to store WINDOW-SIZE. So sender and receiver can have the same WINDOW-SIZE.

2. …
