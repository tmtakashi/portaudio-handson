gcc_options = -std=c++17 -Wall --pedantic-errors

main : main.cpp include lib
	g++ $(gcc_options) -Llib/.libs -lportaudio -Iinclude $< -o $@

run : main
	./main

clean :
	rm -f ./main

.PHONY : run clean