GCC=gcc
ares: ares.c
	$(GCC) ares.c -o ares -Wall -Wextra -pedantic
