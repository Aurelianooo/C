LDFLAGS = -pthread -lpthread
CFLAGS = -g -Wall -Werror
nn: nn.o function.o layer.o neuron.o
	$(CC) $(LDFLAGS) -o nn main.o function.o layer.o neuron.o -lm

nn.o: main.c
	$(CC) $(CFLAGS) -c main.c

function.o: function.c
	$(CC) $(CFLAGS) -c function.c

layer.o: layer.c
	$(CC) $(CFLAGS) -c layer.c

neuron.o: neuron.c
	$(CC) $(CFLAGS) -c neuron.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o nn
