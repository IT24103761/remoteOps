# Prompt Log – AI Interaction Record

**Module:** IE3090 – Network Programming  
**Assignment:** RemoteOps  
**Registration Number:** IT24103761  
**AI Tool Used:** ChatGPT

This log records the main substantive AI interactions used during the development of Part 1. The AI outputs were reviewed, tested, corrected where necessary, and adapted to the final implementation.

| Date | Stage | Prompt / Request Summary | How the AI Output Was Used or Changed |
|---|---|---|---|
| 05 Oct 2026 | Project setup | Asked for a step-by-step setup for the IE3090 RemoteOps assignment on CentOS using registration number IT24103761. | Used the guidance to calculate personalised values, create the project structure, create the personalised source filenames, and prepare the Makefile and folders. |
| 05 Oct 2026 | Git/GitHub setup | Asked how to configure GitHub SSH access from CentOS and push the project repository. | Followed the SSH setup and Git commands. Network issues later prevented access temporarily, so the connection was tested again and corrected before final push. |
| 05 Oct 2026 | Basic TCP connection | Asked for the Agent and Controller TCP implementation and commands to compile and test them. | Used the suggested BSD socket structure as a starting point. The code was tested on CentOS and adjusted to match the personalised port 9410 and project filenames. |
| 05–06 Oct 2026 | Debugging | Asked for help with compilation errors and incorrect source-file contents. | AI helped identify issues including the Agent/Controller code being placed in the wrong files and a stray `k` before `#include <stdio.h>`. These errors were manually corrected and the project was rebuilt. |
| 06 Oct 2026 | Authentication | Asked how to implement `AUTH OPS-3761`, reject incorrect tokens, and reject commands before authentication. | Used the explanation and code structure to add per-session authentication. Tested `AUTH WRONG`, valid authentication, and unauthenticated commands. |
| 06 Oct 2026 | Concurrency | Asked how to support multiple Controllers using POSIX threads. | Used the suggested thread-per-client model with `pthread_create()` and `pthread_detach()`. The Agent was later tested with five simultaneous Controllers. |
| 06 Oct 2026 | SYSINFO and LISTPROC | Asked how to implement system information and process listing in Linux. | Used `/proc/loadavg`, `/proc/meminfo`, and `/proc/uptime` for SYSINFO and a controlled `ps` command for LISTPROC. An old Agent executable caused confusing results, so the old process was stopped and the latest build was tested again. |
| 06 Oct 2026 | Restricted EXEC | Asked how to implement a safe EXEC command using only the required whitelist. | Used the fixed whitelist approach for DATE, UPTIME, DISKFREE, HOSTNAME, and WHOAMI. Confirmed that unsupported commands such as `EXEC LS` were rejected. |
| 06 Oct 2026 | PUT and GET | Asked how to implement binary-safe file upload and download using exact byte counts. | Used the explanation of repeated `send()`/`recv()` operations and explicit file sizes. The first PUT handling required correction because the command format needed filename and size. Final transfer integrity was verified with SHA-256 and `cmp`. |
| 06 Oct 2026 | UDP monitoring | Asked how to implement `MONITOR START` and `MONITOR STOP` using UDP. | Used the suggested UDP listener/monitoring-thread design. The final implementation sends SYSINFO-style datagrams approximately every five seconds and stops correctly on request. |
| 06 Oct 2026 | Logging and disconnect handling | Asked how to add timestamped logging and handle graceful/unexpected Controller disconnects. | Used the guidance to add `remoteops_IT24103761.log`, mutex-protected log writes, graceful QUIT handling, and unexpected-disconnect handling. The Agent was tested after terminating a Controller with `Ctrl+C`. |
| 06–07 Oct 2026 | Testing and report evidence | Asked what screenshots, commands, captions, testing evidence, and report sections were required by the assignment. | Used the guidance to collect real screenshots from the running CentOS implementation and to add protocol evidence, error handling, a testing summary, design rationale, development-process evidence, and a conclusion. |
| 07 Oct 2026 | Assignment review | Asked to compare the implementation report against the assignment brief and identify missing points. | Added missing report sections and corrected weak or incomplete evidence. The advice was used as a checklist, while final wording and screenshots were based on the actual implementation and test results. |

## Evaluation of AI Assistance

ChatGPT was mainly used for implementation guidance, debugging, explaining socket-programming concepts, and reviewing the report against the assignment requirements.

The AI was useful for:
- breaking the assignment into manageable stages;
- explaining BSD socket functions and POSIX threads;
- identifying compilation and logic errors;
- suggesting testing commands and evidence;
- explaining exact-byte file transfer and TCP framing;
- reviewing report completeness.

The AI was not accepted without checking. Some outputs required correction or adaptation. Examples include incorrect file placement during early development, a malformed source line, confusion caused by an old running executable, and changes needed to the PUT/GET protocol handling. All final functionality was tested manually on CentOS before being included in the submission.

The final code, screenshots, test results, report, and repository reflect the implementation that was actually compiled and executed.
