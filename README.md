# Socket-Programming

Technologies Used: C, Socket Programming

In this project, I developed a distributed file management system designed to handle multiple client requests concurrently across different machines. The system consists of a main server (serverw24) and two additional mirrored servers (mirror1 and mirror2), enhancing redundancy and load balancing capabilities.

Key Features:

Multi-Process Architecture: Each server leverages a multi-process model where incoming client connections trigger the spawning of child processes. This model allows simultaneous handling of multiple requests efficiently.
Dynamic File Handling: Clients can request detailed file information, list directories based on different criteria (such as creation time or alphabetical order), and retrieve files based on size, date, or type through compressed archives.
Fault Tolerance and Scalability: By alternating client requests among the main server and its mirrors based on a predefined sequence, the system ensures balanced load distribution and improved reliability.

Achievements:

Robust Client-Server Communication: Implemented using TCP sockets to ensure reliable and ordered data transfer.
Advanced File Search Capabilities: Developed functions to search and retrieve files by various attributes, including size range, file type, and creation date.
Custom Protocol Design: Designed a simple yet effective command protocol for client-server interaction, ensuring ease of use and extensibility.

This project not only bolstered my skills in network programming and system design but also deepened my understanding of creating scalable multi-server architectures that can handle high volumes of requests in a distributed environment.
