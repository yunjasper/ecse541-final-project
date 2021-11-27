SYSTEMC_HOME=/home/jyun6/Desktop/systemc-2.3.3

FILES= main.cpp sw_component.cpp hw_component.cpp

make:
	g++ -g3 -Wall -Wextra -std=c++11 -I. -I$(SYSTEMC_HOME)/include -L. -L$(SYSTEMC_HOME)/lib-linux64 $(FILES) -o chol_hw -lsystemc -lm

.PHONY: clean

clean:
	rm -f $(OBJS) main