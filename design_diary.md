# Design Diary – RemoteOps

**Module:** IE3090 – Network Programming  
**Registration Number:** IT24103761  
**Project:** RemoteOps

## 05 October 2026 – Project Setup and Basic TCP Connection
I created the RemoteOps project structure on CentOS and personalised the implementation using my registration number. The Agent port was calculated as `9410`, the session ID as `SID:1673`, and the authentication token as `OPS-3761`.

I created the personalised source files, Makefile, storage directory, and documentation files. Git and GitHub SSH access were also configured.

The first implementation focused on basic TCP client/server communication using `socket()`, `bind()`, `listen()`, `accept()`, and `connect()`. During this stage I corrected setup issues including Makefile problems and source files being placed in the wrong roles.

## 06 October 2026 – Authentication, Concurrency and Commands
I added session authentication using `OPS-3761`. Commands are rejected until authentication succeeds. I tested correct authentication, incorrect tokens, and commands sent before authentication.

I then implemented POSIX thread-per-client concurrency using `pthread_create()` and `pthread_detach()`. This allowed the Agent to handle several Controllers independently.

Next, I implemented `SYSINFO` using Linux `/proc` files and `LISTPROC` using a controlled process-listing command. I also added the restricted EXEC whitelist containing only `DATE`, `UPTIME`, `DISKFREE`, `HOSTNAME`, and `WHOAMI`.

One important debugging issue was that an older Agent executable was still running during testing. After stopping the old process, rebuilding, and restarting the latest executable, the new functionality worked correctly.

## 06 October 2026 – File Transfer, UDP Monitoring and Logging
I implemented PUT and GET file transfer using explicit file sizes and repeated send/receive operations. Uploaded files are stored in `./agentfiles/IT24103761/`.

I tested file transfer using `test.txt` and verified the original and downloaded files using SHA-256 and `cmp`.

I then implemented UDP monitoring. `MONITOR START <udp_port>` starts periodic system-statistics datagrams, and `MONITOR STOP` stops the stream. I selected an interval of approximately five seconds.

Finally, I added timestamped logging to `remoteops_IT24103761.log`. A mutex protects the log because several worker threads may write at the same time. I tested both graceful disconnect using `QUIT` and unexpected disconnect using `Ctrl+C`. The Agent remained running after the unexpected disconnect.

## 07 October 2026 – Final Testing and Documentation
I performed final tests for authentication, SYSINFO, LISTPROC, EXEC, PUT, GET, missing-file handling, UDP monitoring, five simultaneous Controllers, logging, graceful disconnect, and unexpected disconnect.

I also reviewed the implementation report and added protocol evidence, error handling, a testing summary, design rationale, development-process evidence, and a conclusion.

The main lessons from the project were that TCP must be treated as a byte stream, file transfers require exact byte counting, concurrent clients need independent session state, shared resources such as logs need synchronization, and authentication and command whitelisting must be enforced consistently.
