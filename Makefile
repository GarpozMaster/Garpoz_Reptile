obj-m += network_server.o
network_server-objs := minimal_net_server.o

all:
        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

.PHONY: all clean
