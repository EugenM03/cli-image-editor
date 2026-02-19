#Copyright Munteanu Eugen 315CAb 2022-2023
#SETUP
PARAMETERS=-Wall -Wextra -std=c99

build:
	gcc image_editor.c $(PARAMETERS) -lm -o image_editor

clean:
	rm -f image_editor