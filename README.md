# OS-Assignments

LIMITATIONS OF THE CODE

1) We cannot use commands like cd,exit. This is because we cannot change working direcotries as execvp() function can only run external commands.
Commands which change the state of the shell will not work.
2) We cannot use fg(foreground command) after we send the process into the background. Foreground commands do not work as we do not keep a record of all the background processes.
3) Signal handling(like CTRL+C AND CTRL+Z) do not work. This is due to the fact that the above signals need proper background and foreground management.
4)Complex processes like shell substitution and shell expansion do not work. This is as implementing these functionalities would be much beyond the
scope of our assignment as the actual shell handles these commands internally before executing the command
5)Background processes for multiple pipes dont work as bg command works only for functions with single commands
