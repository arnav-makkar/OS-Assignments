https://github.com/arnav-makkar/OS-Assignments/tree/main

Group project by Bhuvan Raju(2023171) and Arnav Makkar(2023127).<br>
Group ID: 89

All parts, including the bonus section have been completed.


<br>

### Some of the commands that are not supported have been listed as follows

<h4>(a) Change of Directory:</h4>
Commands like cd, exit, set, unset, export and env are not supported since execvp() runs external programs only. It cannot change the state of the shell itself.

<h4>(b) Foreground/Background Management:</h4>
Background commands (denoted with &) cannot be brought back to the foreground since there is no tracking of background processes.

<h4>(c) Signal Handling:</h4>
Signals (apart from CTRL+C and CTRL+Z) have not been implemented. This is because signals need proper background and foreground process management.

<h4>(d) Shell Substitution and Expansion:</h4>
Complex processes like shell substitution and shell expansion do not work. Implementing these would be much beyond the scope of our assignment as the actual shell handles these commands internally before executing the command.

<h4>(e) Background Processes with Pipes:</h4>
Background processes for piped commands have not been implemented. The shell only manages simple background command execution.

