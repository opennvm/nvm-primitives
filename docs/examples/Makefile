#Assuming that libvsl.so is in /usr/lib/fio and libnvm-primitives.so is in /usr/lib/nvm
#Assuming that public header files are in /usr/include/nvm
LDFLAGS= -L/usr/lib/nvm -L/usr/lib/fio -lnvm-primitives -lvsl -ldl -lpthread

CPPFLAGS=-std=gnu99

all: nvm_sample
	./nvm_sample "/dev/fioa"

nvm_sample: nvm_sample.o
	$(CC) -o $@ $< $(LDFLAGS)

clean:
	rm nvm_sample.o nvm_sample
