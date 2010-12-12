all: main shooter
main: main.cpp
	g++ -o ballrecognition main.cpp `pkg-config opencv --cflags --libs`
shooter: shooter.cpp
	g++ -o shooter shooter.cpp
clean:
	rm ballrecognition.1 ballrecognition shooter.1 shooter
start:
	sudo dvgrab - > camfifo &
	./ballrecognition
