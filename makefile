CFLAGS = -Wall -Wextra -g
SOURCE_FILES = main.c prompt.c warp.c peek.c pastevents.c syscmds.c proclore.c seek.c bgHandler.c signals.c neonate.c iMan.c fg_and_bg.c
OUTPUT_FILENAME = shell

main:
	gcc ${CFLAGS} ${SOURCE_FILES} -o ${OUTPUT_FILENAME}

clean:
	rm -f ./${OUTPUT_FILENAME} .history.txt
