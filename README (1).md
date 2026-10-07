# RemoteOps – IE3090 Network Programming Assignment

## Student Details
- **Registration Number:** IT24103761
- **Module:** IE3090 – Network Programming
- **Assignment:** RemoteOps: A Remote System Monitoring and Management Tool over TCP/IP

## Personalised Values
- Numeric registration number: `24103761`
- First four digits: `2410`
- TCP listening port: `7000 + 2410 = 9410`
- Last three digits: `761`
- Agent source file: `agent_761.c`
- Controller source file: `controller_761.c`
- Makefile: `Makefile_761`
- Last four digits: `3761`
- Reversed last four digits: `1673`
- Session ID: `SID:1673`
- Authentication token: `OPS-3761`
- Log file: `remoteops_IT24103761.log`
- File storage path: `./agentfiles/IT24103761/`
- Submission archive: `IE3090_IT24103761.zip`

## Project Overview
RemoteOps is a client/server remote monitoring and management application written in C using the standard BSD sockets API.

The system contains:
- **Agent** – TCP server running on the managed machine.
- **Controller** – TCP client used by an administrator.
- **TCP control channel** – authentication, commands, responses, file transfer, and session control.
- **UDP monitoring channel** – periodic system-information updates.
- **POSIX thread-per-client concurrency** – each Controller connection is handled independently.

## Implemented Features
1. Multiple simultaneous Controller connections.
2. Authentication using `AUTH OPS-3761`.
3. `SYSINFO` for CPU load, memory usage, and uptime.
4. `LISTPROC` for a process snapshot.
5. Restricted `EXEC` whitelist: `DATE`, `UPTIME`, `DISKFREE`, `HOSTNAME`, `WHOAMI`.
6. File upload using `PUT`.
7. File download using `GET`.
8. Periodic UDP monitoring using `MONITOR START` and `MONITOR STOP`.
9. Graceful and unexpected disconnect handling.
10. Timestamped logging to `remoteops_IT24103761.log`.

## Build Instructions
### Compile
```bash
make -f Makefile_761
```

### Clean
```bash
make -f Makefile_761 clean
```

## Running the Application
### Start the Agent
```bash
./agent_761
```

The Agent listens on TCP port `9410`.

### Start the Controller
```bash
./controller_761 127.0.0.1
```

Replace `127.0.0.1` with the Agent IP address when running across different machines.

## Protocol Commands

### Authentication
```text
AUTH OPS-3761
```

Expected:
```text
OK AUTHENTICATED SID:1673
```

### System Information
```text
SYSINFO
```

Response format:
```text
OK SYSINFO <cpu_load> <mem_used_mb> <uptime_sec> SID:1673
```

### Process Listing
```text
LISTPROC
```

Response format:
```text
OK PROCS <processes> SID:1673
```

### Restricted Command Execution
Allowed:
```text
EXEC DATE
EXEC UPTIME
EXEC DISKFREE
EXEC HOSTNAME
EXEC WHOAMI
```

Example rejected command:
```text
EXEC LS
```

Response:
```text
ERR 002 COMMAND_NOT_ALLOWED SID:1673
```

### Upload a File
```text
PUT test.txt
```

Uploaded files are stored under:
```text
./agentfiles/IT24103761/
```

### Download a File
```text
GET test.txt
```

The Agent responds with the filename and exact file size before sending the raw bytes.

### Start UDP Monitoring
```text
MONITOR START 10000
```

Periodic SYSINFO-style UDP datagrams are sent approximately every 5 seconds.

### Stop UDP Monitoring
```text
MONITOR STOP
```

### Quit
```text
QUIT
```

Expected:
```text
OK BYE SID:1673
```

## Error Responses
```text
ERR 001 AUTH_FAILED SID:1673
ERR 002 COMMAND_NOT_ALLOWED SID:1673
ERR 003 NOT_AUTHENTICATED SID:1673
ERR 004 FILE_TOO_LARGE SID:1673
ERR 005 FILE_NOT_FOUND SID:1673
```

## System Information Sources
```text
/proc/loadavg
/proc/meminfo
/proc/uptime
```

## Concurrency
The Agent uses POSIX threads. Each accepted Controller connection is handled in a worker thread using `pthread_create()`, and `pthread_detach()` is used to release resources automatically when the thread finishes.

The system was tested with at least five simultaneous Controller connections.

## TCP Framing and File Transfer
TCP is a byte stream, so the implementation does not assume that one `send()` or `recv()` call transfers a complete command or file.

Newline termination is used for text commands. PUT and GET use explicit file sizes and repeated send/receive operations so the exact number of file bytes is transferred.

File integrity was verified using SHA-256 and `cmp`.

## UDP Monitoring
The Agent sends monitoring messages in the form:
```text
SYSINFO <cpu_load> <mem_used_mb> <uptime_sec> SID:1673
```

The chosen interval is approximately 5 seconds.

## Logging
The Agent logs activity to:
```text
remoteops_IT24103761.log
```

Logged events include:
- connections;
- authentication;
- commands;
- uploads;
- downloads;
- monitoring;
- graceful disconnects;
- unexpected disconnects.

A POSIX mutex protects concurrent log writes.

## Project Structure
```text
remoteOps/
├── agent_761.c
├── controller_761.c
├── Makefile_761
├── README.md
├── design_diary.md
├── reflection.md
├── remoteops_IT24103761.log
└── agentfiles/
    └── IT24103761/
```

## Testing
The implementation was tested for:
- successful Agent startup;
- TCP port 9410;
- valid and invalid authentication;
- unauthenticated command rejection;
- SYSINFO;
- LISTPROC;
- all allowed EXEC commands;
- disallowed EXEC commands;
- PUT upload;
- GET download;
- file integrity;
- missing file handling;
- UDP monitoring start/stop;
- five simultaneous Controllers;
- graceful QUIT;
- unexpected Controller disconnect;
- timestamped logging.

## Git Repository
Repository SSH URL:
```text
git@github.com:IT24103761/remoteOps.git
```

Development was performed incrementally using descriptive Git commits.
