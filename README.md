# Reliable Transport

## Background

In this project, I have developed a custom communication protocol built on UDP, designed for consistent delivery of real-time traffic via a virtual link. The protocol starts in the application layer, using the 'Sender.c' and 'Receiver' files, which activate the custom procedure coded into 'Reliable.c' and 'ReliableImpl.c' for file transmission. This dependable transport protocol includes state transition dynamics, arrangements for connection creation, and disconnection. To send data, it packages a segment with a header and checksum, dispatches the data to the recipient, and keeps track of the acknowledgement (ACK) segments that come back. It employs the sliding window protocol to handle the resequencing of packets and to guarantee that all packets have reached the recipient in-order, regardless of any packet losses during transmission. A retransmission timer was set based on an estimated Round-Trip Time (RTT). A three-way handshake initiates the connection, producing an inaugural sequence number for the packet. I designed these sequence numbers to "wrap-around" after reaching their peak limit to accomadate all file sizes. The protocol also incorporates congestion control, adopting a methodology similar to TCP Reno, which applies additive amplification and multiplicative reduction upon detecting packet loss due to absent ACKs or a retransmission timeout. This method optimizes the use of available bandwidth. The effectiveness of this congestion control is assessed by evaluating the parity of multiple connections through the use of Jain's fairness index.

<div align="center">
  <img src="images/network.png" width="666" height="383">
</div>
<p align="center">
  Network Model
</p>

## Softwares and Technologies
<div align="center">
   <img src="https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white"/>
</div>
