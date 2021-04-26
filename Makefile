all: clean build init

build:
	gcc initializer.c -o initializer.o
	gcc consumer.c -o consumer.o
	gcc producer.c -o producer.o
	gcc finalizer.c -o finalizer.o

init:
	./initializer.o memoria1 256 10

prod:
	./producer.o memoria1 auto

cons:
	./consumer.o memoria1 auto

end:
	./finalizer.o memoria1

clean:
	rm -rf ./*.o