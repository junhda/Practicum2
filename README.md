# Practicum 2
#### Daniel Jun
#### CS 5600
#### 4/20/2023

## Notes
* don't choose a port number under 1000 (check if the port used is common in TCP)
    * also will need to set to my local IP (google config)
    * can also test this using a virtual machine as well
* when a client connects to a server's port through accept(), this function actually creates another socket for that client connection. This way, the port that was opened by the server is still open for other clients to connect to. this allows multiple clients to connect to a single port
    * when another client tries to connect to the server while another client is ALREADY connected to the server, the second client will be put in a waiting queue until the client already connected to the server disconnects. servers are single threaded and single client by default.
        * is there a way to allow multiple clients connect simultaneously? yes. 
            1. create a fork() for each client so that they are entered into a separate process (1 process per client). not common anymore
            2. thread per client: each time a new client connects, create a new thread for that client. however, need to consider file and data structure sharing with this one (deadlocks)
* client sends server a string; server reads the string and performs the stated command
    * server response to client can be in any format: XML, json, delimited strings, etc 
    * client takes the server response and then does something with it
* when running the "client" program, it must be able to take in the input argument as a command line argument rather than the default pattern from the given code
    * in prompt example, fget is the name of the program
* syncing 2 USB drives
    * question 6 says to wite to 2 usb drives at the same time. reads only have to occur from 1 or the other. In the scenario where 1 drive is removed, but the other is not, writes to the server / drives should still succeed. however, now 1 drive will get the write, while the other drive does not. Despite this, when reconnecting the 2nd drive, these drives should be synced
        * option a
            * have a scanner thread - scan for differences between 1 and 2
            * have 1 main drive and an auxillary drive
        * option 1 - journal it
            * keep a journal file in the server with a flags for write successful to both drives
            * prior to any writes, first write to the journal file
            * now write to the drives
            * update the flag in the journal file for the drives successfully written to
            * when a drive is reconnected to the server, read the journal and update the drive in a separate thread by following which journal entries have not been completed for that drive
            * Syncing thread: a background thread or process that keeps checking if usb1 and usb2 are online (sleep for somewhere between 1 - 5 seconds). 
                * if a drive is found to not be online, mark that it has been offline
                * if a drive is connected and the mark previously indicates that it was offline, resync the drives using the journals
        * Option 2
            * keep track of the most recently updated drive 
            * when a drive is connected, format it, and then add everything from the most recently updated drive
        * Option 3
            * do a merge across drives when a drive is connected
