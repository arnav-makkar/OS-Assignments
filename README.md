# OS-Assignments

LIMITATIONS OF THE CODE

1) We cannot use commands like cd,exit. This is because we cannot change working direcotries as execvp() function can only run external commands.
Commands which change the state of the shell will not work.
2) We cannot use fg(foreground command) after we send the process into the background
3) 
