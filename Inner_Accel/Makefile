CC=g++
CFLAGS=-g3
IFLAGS=-I$$SYSTEMC/include
LFLAGS=-L$$SYSTEMC/lib-linux64 -lsystemc -lm
DEPS=main.cpp sw_component.cpp hw_component.cpp
OBJS=main.o sw_component.o hw_component.o

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(IFLAGS) $(LFLAGS)

chol: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(IFLAGS) $(LFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) chol main.o sw_component.o hw_component.o
