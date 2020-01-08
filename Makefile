all:
	g++ -o paint paint.cpp `pkg-config --libs --cflags opencv4`
clean:
	rm -f paint
