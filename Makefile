CC = gcc
CCOPTS = -Wall -c -g -ggdb
LINKOPTS = -Wall -g -ggdb -pthread

EXEC=scheduler
OBJECTS=scheduler.o schedulerSimulation.o list.o tests.o sched_threads.o

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(LINKOPTS) -o $@ $^

%.o:%.c
	$(CC) $(CCOPTS) -o $@ $^

test: scheduler
	- timeout 20 bash -c "./scheduler -test -fifo 5"
	- timeout 20 bash -c "./scheduler -test -rr 5 10"

clean:
	- $(RM) $(EXEC)
	- $(RM) $(OBJECTS)
	- $(RM) *~
