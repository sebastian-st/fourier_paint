
# Check OpenCV version

CV_NAME = opencv
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(shell pkg-config opencv4 2>/dev/null; echo $$?), 0)
CV_NAME = opencv4
else ifneq ($(shell pkg-config opencv 2>/dev/null; echo $$?), 0)
 $(error could not find or check opencv version)
endif
endif

all:
	g++ -std=c++11 -o paint paint.cpp `pkg-config --libs --cflags $(CV_NAME)` -lstdc++
clean:
	rm -f paint
