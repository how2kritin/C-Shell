CFLAGS = -Wall -Wextra -g
SOURCE_FILES = main/main.c main/prompt.c custom-commands/warp.c custom-commands/peek.c custom-commands/pastevents.c main/syscmds.c custom-commands/proclore.c custom-commands/seek.c main/bgHandler.c custom-commands/signals.c custom-commands/neonate.c networking/iMan.c custom-commands/fg_and_bg.c
OUTPUT_FILENAME = shell

all:
	gcc ${CFLAGS} ${SOURCE_FILES} -o ${OUTPUT_FILENAME}

clean:
	rm -f ./${OUTPUT_FILENAME} .history.txt
