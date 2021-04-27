all: clean build init

build:
	gcc initializer.c -o initializer.o
	gcc consumer.c -o consumer.o
	gcc producer.c -o producer.o
	gcc finalizer.c -o finalizer.o

init:
	./initializer.o lda 256 4

prod:
	./producer.o lda auto 0.2

cons:
	./consumer.o lda auto 1.5

end:
	./finalizer.o lda

clean:
	rm -rf ./*.o