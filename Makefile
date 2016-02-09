THIRD_LIBS=-levent
LIBS=-ldl
CFLAGS=-I./include -I./third/http-parser/

master:src/master.o src/worker.o src/listener.o src/connection.o src/main.o src/http.o third/http-parser/libhttp.a
		g++ -g -o $@ src/master.o src/worker.o src/listener.o src/connection.o src/main.o src/http.o third/http-parser/libhttp.a $(THIRD_LIBS) $(LIBS)

third/http-parser/libhttp.a:third/http-parser/http_parser.o
	ar -r $@ $<

third/http-parser/http_parser.o:third/http-parser/http_parser.c third/http-parser/http_parser.h 
	g++ -o $@ -fPIC -c $< $(CFLAGS)

src/master.o:src/master.cpp include/master.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/worker.o:src/worker.cpp include/worker.h include/util.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/listener.o:src/listener.cpp include/listener.h include/util.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/connection.o:src/connection.cpp include/connection.h include/util.h ./include/http.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/main.o:src/main.cpp include/master.h
		g++ -g -o $@ -c $< $(CFLAGS)

src/http.o:src/http.cpp include/http.h third/http-parser/http_parser.h include/connection.h
	g++ -o $@ -c $< $(CFLAGS)

clean:
		rm -f src/*.o master
