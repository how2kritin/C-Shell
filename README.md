[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/76mHqLr5)
# Description
To run the shell, do "make && ./a.out".
To clean up residual executable and history files, run "make clean".
```
main.c and headers.h -> General main function and the runCmd function, and header files required for the entire codebase, and also to handle Keyboard Interrupt signals.
prompt.c and prompt.h -> For Specification 1 of Part A.
Specification 2 of Part A is handled in main.c itself.
warp.c and warp.h -> For Specification 3 of Part A.
peek.c and peek.h -> For Specification 4 of Part A.
pastevents.c and pastevents.h -> For Specification 5 of Part A.
syscmds.c and syscmds.h -> For Specification 6 of Part A.
bgHandler.c and bgHandler.h -> To handle background processes for Specification 6 of Part A; to handle activities for Specification 12 of Part B; and Ctrl-D for Specification 13 of Part B.
proclore.c and proclore.h -> For Specification 7 of Part A.
seek.c and seek.h -> For Specification 8 of Part A.
Specifications 9, 10, 11 of Part B -> Handled in main.c itself.
Specification 12 of Part B -> Handled in bgHandler.c and bgHandler.h.
signals.c and signals.h -> For Specification 13 of Part B (except Ctrl-D, which has been handled by bgHandler.)
fg_and_bg.c and fg_and_bg.h -> For Specification 14 of Part B
neonate.c and neonate.h -> For Specification 15 of Part B
iMan.c and iMan.h -> For Specification 16 of Part C.
```

# Assumptions
1. This shell was created on Arch Linux. There may be parts of code that could possibly break, if they are run on another distro of Linux. So, it is hereby assumed that whenever this code breaks on another Linux Distro, you will allow me to run this code on my own laptop.
2. Shell will be run as sudo, so that readlink will never give a permissions issue. 
3. If no previous warps are performed, then the first time the "warp -" command is run, ```OLDPWD not set``` is printed. It will keep being printed for as long as the ```OLDPWD``` env variable is not set (basically a char array in my shell).
4. For peek -l -> Print everything exactly how Bash does it, apart from the Lexicographical Order. So, for a symlink, display name of file with the path of file that it is linked to, just like how bash does it.
5. For peek -> Without the -l flag, display everything in a single line, separated by spaces. For clarification, refer to Q37 of the doubts document.
6. For peek -> Coloring -> Refer to Q38 and Q41 of the doubts document. For the file to be treated as an executable, "User" should have 'x' perm. Symlinks are to be coloured cyan.
7. For peek -> Padding is allowed.
8. For peek -l -> For files not made in the current year, display their year of modification instead of time of modification.
9. pastevents command stores history in .history.txt (hidden file). This file will never be overwritten by a testcase (according to TA's response in the doubts document).
10. For all specifications (esp spec 6) -> There exist at most 512 args for any one command.
11. There can be a maximum of 500 concurrent background processes.
12. pastevents stores ALL commands (even erroneous ones) EXCEPT pastevents and its derivatives, but modify pastevents execute and store.
13. Max length of any given command is 4096 bytes.
14. Don't store commands with pastevents anywhere as a substring, except for pastevents execute -> modify and store.
15. If pastevents execute is not given a valid argument, then it won't be stored.
16. There will be atmost 512 commands given in one single line.
17. For seek -e -> User must have 'r' perm for files and 'x' perm for directories when only a single file is found, else, display "Missing permissions for task!".
18. Q88 of the doubts document -> I will not be storing "exit" in the pastevents whenever shell is exit.
19. Q83 of the doubts document -> I will be printing the time taken for the ENTIRETY of the command given (including ;, & etc.,) and printing it in the next prompt, along with the entire command and its args.
20. Q79 of the doubts document -> I will NOT be storing ANYTHING if pastevents execute <idx> has an invalid idx. If it is in a string separated by ; then I will just ignore that pastevents, and move onto copying the other strings. I will pretend that, that pastevents command never existed.
21. Q75 of the doubts document -> For seek, in string matching: ```[VB] The given string should match a prefix of the file or folder.``` -> So, I will search for prefixes only, in seek.
22. Q107 of the doubts document -> Will be printing total block size in terms of 512-byte chunks.
23. In prompt; even if pastevents execute is present, I'll display the entire string that the user entered, not the modified string which is stored in pastevents.
24. Q120 of the doubts document -> No need to handle " " cases specially/separately for execvp().
25. RED colour messages -> ERROR, YELLOW colour messages -> WARNINGS.
26. All errors are printed to stderr.
27. proclore shows virtual memory size in BYTES. This can be read from /proc/pid/stat -> 23rd value.
28. For seek and peek, any permutation of valid flags is acceptable, and each such flag can be repeated any number of times.
29. No need to support ~, - etc. for Specification 6, since it wasn't mentioned that we have to.
30. For iMan -> It is fine if some HTML tags seep through, for some man pages. It is also fine, if some tags that show up on the webpage aren't formatted correctly on the terminal.
31. For activities -> We know that only background processes can exist, which might be running that are spawned by the shell. Any other process, if foreground, would've exited before we are able to access the shell again. So, for this reason, I need only print the background processes. However, do note that all processes, including foreground, are stored in it, but are removed as soon as they end. So, effectively, it works this way.
32. Q188 of the doubts document -> For activities -> Any state that is NOT stopped ("T" in state in /proc) is considered to be running.
33. No need to handle overflow errors for any numerical inputs.
34. No need to handle extra args given after any command.
35. For ping -> signal number can be negative, but I will make it positive and perform modulo accordingly.
36. There won't be multiple input/output redirections, and all redirections will occur at the end of main command only. So, if that command is supposed to be run as background, then '&' will be put before the redirection.
37. There can be a maximum of 511 pipes in a command.
38. I assume that each opening apostrophe/quote (' or ") will always be followed by a closing apostrophe/quote.
39. Apostrophes/Quotes (' or ") won't be used anywhere in user-defined commands. They will only be used in system commands.
40. Multiple pipes of the form ```|||||``` are treated to be INVALID.
41. Q216 of the doubts document -> I am prioritizing I/O redirection over piping.
42. "~" will act as the home directory of the shell, only for SEEK and PEEK. For the rest, it may not work as expected.
43. If there's an error in one command while redirecting I/O, when there's multiple commands separated by a ; or &, that one command will fail to execute, and the rest of the commands will execute normally.
44. activities -> Only stores command name, but not arguments. Same goes for my background handler array.
45. neonate -> Terminates only when an x is pressed. Won't terminate with Ctrl-C, Ctrl-Z or Ctrl-D.
46. If there is an erroneous command in a pipeline, then that one command will fail, but the rest of the pipeline will execute normally. I am basically implementing what Bash does.
47. Typing "exit" into the terminal will do exactly what pressing Ctrl-D does (kill all background processes, and exit the shell).
    [//]: # (21. Q105 and Q116 of the doubts document -> Accounting for extra spaces everywhere. However, will be accounting for them in execvp&#40;&#41; commands aswell, so, <pre>"echo   "Lorem      Ipsum""</pre> will be printed as <pre>"Lorem Ipsum"</pre>, wherein, the spaces in between are gone, and the quotes are retained -> REDUNDANT!!)
