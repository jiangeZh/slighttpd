THIRD_LIBS=-levent
LIBS=-ldl
CFLAGS=-I./include

master:src/master.o src/worker.o src/listener.o src/connection.o src/main.o
		g++ -g -o $@ src/master.o src/worker.o src/listener.o src/connection.o src/main.o $(THIRD_LIBS) $(LIBS)

src/master.o:src/master.cpp include/master.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/worker.o:src/worker.cpp include/worker.h include/util.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/listener.o:src/listener.cpp include/listener.h include/util.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/connection.o:src/connection.cpp include/connection.h include/util.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/main.o:src/main.cpp include/master.h
		g++ -g -o $@ -c $< $(CFLAGS)

clean:
		rm -f src/*.o master
