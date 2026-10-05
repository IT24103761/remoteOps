# RemoteOps - IE3090 Network Programming

Registration Number: IT24103761

## Personalised Values

Agent TCP Port: 9410
Agent Source File: agent_761.c
Controller Source File: controller_761.c
Makefile: Makefile_761
Session ID: SID:1673
Authentication Token: OPS-3761
Log File: remoteops_IT24103761.log
Storage Path: ./agentfiles/IT24103761/
Submission Archive: IE3090_IT24103761.zip

## Project Description

RemoteOps is a remote system monitoring and management tool implemented
in C using BSD sockets.

The system consists of:

- Agent: TCP server running on the managed machine.
- Controller: TCP client used by the administrator.
- UDP monitoring channel for periodic system information updates.

The Agent will support authentication, system information, process listing,
restricted remote command execution, file upload/download, logging,
concurrent clients and UDP monitoring.
