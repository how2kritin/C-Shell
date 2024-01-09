# Pre-requisites

* gcc
* make

Please install these with a package manager of your choice (usually the one that comes with your Linux distro).

---

---

# Using the Shell
**To run the shell, type:**
```bash
make && sudo ./shell
```
**Note: You are running the shell as sudo, in order to let readlink work on every file.  
Also necessary to send signals to some system processes.**

The directory in which the shell is run, is treated as the Home directory of the shell, and is represented by ~.  

**To clean up executable and history files, run:**
```bash
make clean
```
---

---

# Contents

* ### [Features](#features-1)
#### [Multiple Command Support](#support-for-multiple-commands) | [Piping and Redirection Support](#support-for-piping-and-redirection) | [warp](#warp) | [peek](#peek)
#### [pastevents](#pastevents) | [proclore](#proclore) | [seek](#seek) | [activities](#activities) | [Signals and Keyboard Interrupts](#signals-and-keyboard-interrupts) | [fg and bg](#fg-and-bg)
#### [neonate](#neonate) | [Other system Commands](#other-system-commands)
* ### [Improvements on Project Requirements](#improvements-on-project-requirements-1)
* ### [Assumptions](#assumptions-1)
* ### [Limitations](#limitations-1)
* ### [Future Scope](#future-scope-1)

# Features

## Support for multiple commands
Added support for multiple commands separated by ; or &. The latter runs the command as a background process, while the former is used to separate foreground commands.
```
<JohnDoe@SYS:~> a & b; c ; d   & e
# a and d run in the background, while b, c and e run in the foreground.
```

---

## Support for piping and redirection
Added support for piping and input/output redirection.  
**Does NOT support multiple inputs and outputs (yet).**  

### I/O Redirection:
```
>: Outputs to the filename following ">". Creates a new file if it doesn't exist, and overwrites the contents of the pre-existing file.
>>: Similar to ">", creates a new file if it doesn't exist, but APPENDS the output to the file if it already exists.
<: Reads input from the filename following "<". Returns an error saying "No such input file found!", if an input file with the given name doesn't exist.
```

### Piping:
Passes information between commands. Takes an output from command on the left, and passes it as input to the command on the right.  
Added support for any number of pipes.  
Works for commands created by me, as well as other system commands.

---

## warp
Similar to *cd* in Bash. Added support for “.”, “..”, “~”, and “-” just like in Bash.  
Supports both relative and absolute paths, along with paths from the home directory.  
Supports multiple arguments **unlike** cd, where each successive argument is executed sequentially from left to right.  
If no arguments are given, then it warps into the home directory.

### Input format:
```
warp <path1> <path2 inside path1> ...
```

### Example:
```
<JohnDoe@SYS:~> warp test
/home/johndoe/test
<JohnDoe@SYS:~/test> warp assignment
/home/johndoe/test/assignment
<JohnDoe@SYS:~/test/assignment> warp ~
/home/johndoe
<JohnDoe@SYS:~> warp -
/home/johndoe/test/assignment
<JohnDoe@SYS:~/test/assignment> warp .. tutorial
/home/johndoe/test
/home/johndoe/test/tutorial
<JohnDoe@SYS:~/test/tutorial> warp ~/project
/home/johndoe/project
<JohnDoe@SYS:~/project>
```

---

## peek

Lists all the files and directories in the specified directory in lexicographic order. By default, doesn't show hidden files.
Similar to warp, added support for “.”, “..”, “~” flags. Supports both relative and absolute paths.  
If no arguments are given, it will peek at the current working directory.  

### Input format:
```
peek <flags> <path>
```

### Flags:
```
-l: to display extra information about each file (similar to ls -l in Bash)
-a: (to display hidden files, similar to ls -a in Bash).  
Works even for flags of the form "-al", "-a -l", "-a" etc.
```

### Color coding of files:
```
Blue for directories
Cyan for symbolic links
Green for executables
White for everything else
```

---

## pastevents

Similar to the **history** command in Bash.  
Stores, and outputs upto the last 15 most recent command statements (including arguments) input to the shell.  
Storage of these commands is persistent over different shell runs.  
Doesn't store a command if it is exactly the same as the previously entered command.  
Stores all statements, except commands that include **pastevents** or **pastevents purge**.  

Supports 2 other sub-commands, apart from **pastevents**.  
### pastevents purge
Clears all the pastevents currently stored. This command is **NOT** stored in pastevents.  

### pastevents execute \<index\>
Executes the command at the specified position in pastevents (1-indexed, from most recent to oldest).  
While being stored in pastevents, this command is resolved to be the same as the command at that position in pastevents.  
So, this command is **NOT** stored if it matches the most recent command.

#### Example:
```
<JohnDoe@SYS:~> pastevents # Assuming this is the first time the shell is run
<JohnDoe@SYS:~> sleep 1
<JohnDoe@SYS:~> sleep 2
<JohnDoe@SYS:~> pastevents
sleep 1
sleep 2
<JohnDoe@SYS:~> sleep 2
sleep 1
sleep 2  # sleep 2 is not repeated
<JohnDoe@SYS:~> sleep 1; pastevents
sleep 1
sleep 2
<JohnDoe@SYS:~> pastevents # Notice how 'sleep 1; pastevents' is not considered as it contains the pastevents command
sleep 1
sleep 2
<JohnDoe@SYS:~> sleep 3; pastevents execute 1
<JohnDoe@SYS:~> sleep 3; sleep 2
<JohnDoe@SYS:~> sleep 3
<JohnDoe@SYS:~> sleep 3; sleep 2
<JohnDoe@SYS:~> pastevents
sleep 1
sleep 2
sleep 3; sleep 2 # Only output once as 'sleep 3; pastevents execute 1' is stored as 'sleep 3; sleep 2'
sleep 3
sleep 3; sleep 2
```

---

## proclore

Used to obtain information regarding a process.  
Prints the information of the shell if an argument is missing.  

### Input format:
```
proclore <pid>
```

### Information printed:
```
pid (Process ID)
Process Status (R/R+/S/S+/Z)
Process Group (Process Group ID)
Virtual Memory (in bytes)
Executable Path (relative to the Home directory of the shell, if valid)
```

### Process Status
```
R/R+: Running
S/S+: Sleeping in an Interruptible Wait
Z: Zombie. 
+ signifies that it is a foreground process.
Absence of a + signifies that it is a background process.
```

---

## seek
Looks for a file/directory in the specified target directory (or current working directory, if no directory is specified).  
Returns a list of relative paths (from the target directory) of all matching files/directories **which have the specified prefix.**  
If read/execute permissions are missing for the directory, or read permissions are missing for the file, then depending on the task, this command will display a warning and proceed if it can with the rest of the files.  
Prints "**No match found!**" if no 

### Input Format:
```
seek <flags> <search_prefix> <target_directory>
```

### Flags:
```
-d: Looks only for directories (ignores files even if the prefix matches).
-f: Looks only for files (ignores directories even if the prefix matches).
-e: Effective only when a single file or a single directory with the name is found.
If only one file (and no directories) are found, it prints the contents of the file onto the shell.
If only one directory (and no files) are found, it changes the current working directory to that directory.
Works with -d and -f flags as well.

Works with flags of the form "-de", "-d -e", "-f" etc.
NOTE: -d and -f flags can't be used at the same time.
```

### Color coding:
```
Blue for directories
Green for all other files
```

---

## activities
Prints a list of all the processes currently running, that were spawned by my shell in lexicographic order.  
**Contains each process of the form:**
```
[pid]: [command name] - [state]
```

The state of the process is either Stopped, or Running (any process that hasn't stopped is considered to be running).

---

## Signals and Keyboard Interrupts
ping command is used to send signals to processes.  
Takes the signal number with modulo 32, before checking which signal it belongs to.  
A list of the standard signals can be found over at [man signal](https://man7.org/linux/man-pages/man7/signal.7.html).

### Input Format:
```
ping <pid> <signal_number>
```

### Supported Keyboard Interrupts:
* **Ctrl+C:** Interrupt any currently running foreground process by sending it the SIGINT signal.
* **Ctrl+D:** Log out of the shell (after killing all processes). Does nothing to the actual terminal.
* **Ctrl+Z:** Push the (if any) running foreground process to the background, and change its state from "Running" to "Stopped".

**Note that Ctrl+C and Ctrl+Z do nothing to the shell if no foreground process is running.**

---

## fg and bg
### fg \<pid\>
Brings the running or stopped process with the corresponding pid to the foreground, and hands it control of the terminal.  
Changes the state of the stopped process to _"Running"_.

### bg \<pid\>
Changes the state of a stopped background process to _"Running"_ (in the background).

---

## neonate
Prints the process ID of the most recently created process on the system, and prints this pid once every <time_in_seconds> seconds until the key **'x'** is pressed.  
No other key press or keyboard interrupt can stop this. Only pressing **'x'** can stop this process.

### Input Format:
```
neonate -n <time_in_seconds>
```

---

## iMan
Fetches _man_ pages from the internet (from [man.he.net](http://man.he.net/)).  
**Requires an active internet connection.**

**Note:** You may find rogue HTML tags and character entities in the output. The HTML obtained from the man page has not been post-processed, and as such, these may occur.

---

## Other system commands
Implemented support to run other system commands which haven't been explicitly implemented by me, using _execvp()_ system call.  
These can be run as either foreground or background processes.

### Foreground Processes
**SUPPORTED BY COMMANDS THAT WERE IMPLEMENTED BY ME, AS WELL AS OTHER SYSTEM COMMANDS.**  
Shell waits for the process to complete and regains control afterward.  
Control of the terminal is handed over to this process for the time being, while it is running.  
Time taken by the foreground process and the entire command run will be printed in the next prompt, if the process takes > 2 seconds to run (round down to an integer).

#### Example:
```
<JohnDoe@SYS:~> sleep 5
# sleeps for 5 seconds
<JohnDoe@SYS:~ sleep : 5s>
```

### Background Processes
**COMMANDS THAT WERE IMPLEMENTED BY ME ARE NOT SUPPOSED TO BE RUN AS BACKGROUND PROCESSES. ONLY FOR SYSTEM COMMANDS.**  
Any command invoked with "&" is treated as a background command. The shell will spawn that process, but doesn't hand the control of the terminal to it.  
What this means is that the shell will keep taking other user commands.  
Shell prints the PID of the background process as soon as the process spawns, and once the process is done executing, will print the process name as well as the exit status of the process (along with its PID).

#### Example:
```
<JohnDoe@SYS:~> sleep 10 &
13027
<JohnDoe@SYS:~> sleep 20 &                       # After 10 seconds
Sleep exited normally (13027)
13054
<JohnDoe@SYS:~> echo "Lorem Ipsum"               # After 20 seconds
Sleep exited normally (13054)
Lorem Ipsum
```

---

---

# Improvements on Project Requirements

* Processed arguments in single quotes (' ') and double quotes (" ") separately and appropriately, so, something like ```echo "a && b"``` or ```echo 'a && b'``` will print ```a && b```, and something like ```sleep "5"```
will work the same way as ```sleep 5```.
What this also means is that everything inside quotes is treated as a single argument, and no further processing (such as treating the statement to the right of, say, a redirection operator, as the input/output file) will be done.
For example,
```
<JohnDoe@SYS:~> echo "hi"; echo "bye hello"; echo "this is && a"; echo "statement >> << | ok ; nice"
hi
bye hello
this is && a
statement >> << | ok ; nice
```
* pastevents execute \<index\> has been made to work such that, no matter however large a previous executed command may be, its output will always be properly redirected if this command is used anywhere as a part of an even larger command.  
For example,
```
<JohnDoe@SYS:~> echo "hi" ; echo "bye"
hi
bye
<JohnDoe@SYS:~> pastevents execute 1 > a.txt
<JohnDoe@SYS:~>

Checking a.txt, we find:
hi
bye
```
* exit command has been implemented to gracefully exit from the shell, and free all memory which has been dynamically allocated.
* Any permutation of flags are allowed in peek and seek, but invalid flags and invalid permutations (such as using -d and -f simultaneously in seek) will result in an error.  
However, something like ```peek -laal``` is perfectly valid.

---

---

# Assumptions

* The shell will be run as a superuser, in order to ensure that commands such as ```proclore``` won't fail on processes owned by the system.
* ~ in the shell is the directory in which the shell was started. This cannot be changed for the duration of the shell being run.
It is **NOT** the home directory of your computer.

---

---

# Limitations

* Commands that were implemented by me do not support background execution. For such commands, "&" is treated as an argument.
* Currently do not support whitespaces in paths of files/directories, due to the way I tokenize my inputs with whitespace as a delimiter.

---

---

# Future Scope

* I plan to add support for arrow keys to navigate through previously used commands in the shell, like Bash does.
* I plan to add support to show the number of background processes currently running beside the prompt of the shell, like Bash does.

---

---
