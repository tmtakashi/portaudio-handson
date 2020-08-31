gcc_options = -std=c++17 -Wall --pedantic-errors
build_path = ./build
exec_path = $(build_path)/main

main : main.cpp
	mkdir -p build
	g++ $(gcc_options) -L/usr/local/lib -lportaudio -I/usr/local/include -o $(build_path)/$@ $<

run : main
	./$(exec_path)

clean :
	rm -f $(exec_path)

.PHONY : run clean
