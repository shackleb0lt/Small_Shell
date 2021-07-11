all:
	gcc -Wall -std=gnu99 -o shell  main.c -lreadline
clean:
	rm shell