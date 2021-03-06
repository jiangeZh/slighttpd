THIRD_LIBS=-levent
LIBS=-ldl
CFLAGS=-I./include -I./lib/http-parser/

slighttpd:src/master.o src/worker.o src/listener.o src/connection.o src/main.o src/http.o src/plugin.o src/config.o lib/http-parser/libhttp.a
		g++ -o $@ src/master.o src/worker.o src/listener.o src/connection.o src/main.o src/http.o src/plugin.o src/config.o lib/http-parser/libhttp.a $(THIRD_LIBS) $(LIBS)

lib/http-parser/libhttp.a:lib/http-parser/http_parser.o
	ar -r $@ $<

lib/http-parser/http_parser.o:lib/http-parser/http_parser.c lib/http-parser/http_parser.h 
	g++ -o $@ -fPIC -c $< $(CFLAGS)

src/master.o:src/master.cpp include/master.h
		g++ -o $@ -c $< $(CFLAGS)

src/worker.o:src/worker.cpp include/worker.h include/util.h
		g++ -o $@ -c $< $(CFLAGS)

src/listener.o:src/listener.cpp include/listener.h include/util.h
		g++ -o $@ -c $< $(CFLAGS)

src/connection.o:src/connection.cpp include/connection.h include/util.h ./include/http.h
		g++ -o $@ -c $< $(CFLAGS)

src/main.o:src/main.cpp include/master.h
		g++ -o $@ -c $< $(CFLAGS)

src/http.o:src/http.cpp include/http.h lib/http-parser/http_parser.h include/connection.h
	g++ -o $@ -c $< $(CFLAGS)

src/plugin.o:src/plugin.cpp include/plugin.h
	g++ -o $@ -c $< $(CFLAGS)

src/config.o:src/config.cpp include/config.h
	g++ -o $@ -c $< $(CFLAGS)

clean:
		rm -f src/*.o slighttpd
