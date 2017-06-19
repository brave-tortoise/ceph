CC = gcc
CFLAGS = -g

TARGET = trace-player
SRCS = trace-player.c iotrace.c
OBJS = $(SRCS:.c=.o)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ -lpthread

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(TARGET) $(OBJS)
