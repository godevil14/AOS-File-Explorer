## Tracker
- Compile using ```g++ tracker.cpp -o tracker```
- Run using ```./tracker  tracker_info.txt 1```

## Client
- Compile using ```g++ client.cpp -o client -lcrypto -lssl -O2
- Ignore the warnings
- Run using ```./client 127.0.0.1:9090 tracker_info.txt```
- IP is loopback IP so dont change it. You can change the port for different client.
- Uncomment line 275 to demonstrate how a file can be taken from seeder as well as leecher.