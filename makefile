all: buildrooms adventure

buildrooms: johnsmol.buildrooms.c
	gcc -Wall -g -o johnsmol.buildrooms johnsmol.buildrooms.c

adventure: johnsmol.adventure.c
	gcc -Wall -g -o johnsmol.adventure johnsmol.adventure.c -lpthread

clean:
	rm johnsmol.adventure johnsmol.buildrooms

cleanall:
	rm -r johnsmol.rooms*
	rm johnsmol.buildrooms johnsmol.adventure
