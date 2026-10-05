# RemoteOps Design Diary

## 5 October 2026

Configured the CentOS development environment and connected the system
to GitHub using SSH.

Cloned the remoteOps GitHub repository and created the personalized
project structure for registration number IT24103761.

Calculated personalized values:

- TCP Port: 9410
- SID: 1673
- Authentication Token: OPS-3761
- Agent File: agent_761.c
- Controller File: controller_761.c
- Makefile: Makefile_761
- Log File: remoteops_IT24103761.log
- Storage Path: ./agentfiles/IT24103761/

I plan to use POSIX threads for the concurrency model so that multiple
Controller clients can be served independently by the Agent.

## 6 October 2026 – Basic TCP Communication

Implemented and tested the initial TCP communication between the
RemoteOps Agent and Controller.

The Agent creates a TCP socket, binds to the personalized port 9410,
listens for connections and accepts a Controller.

The Controller connects to the Agent using 127.0.0.1 and port 9410.

The listening socket was verified using the ss command. The Controller
successfully sent a HELLO message and received a response containing
SID:1673.
