CC=gcc
TARGET = testcase

$(TARGET): testcase.o fs.o fat.o buf.o Disk.o
	$(CC) -o $(TARGET) testcase.o fs.o fat.o buf.o Disk.o

testcase.o: testcase.c
	$(CC) -c testcase.c

fs.o: fs.c fs.h
	$(CC) -c fs.c

fat.o: fat.c fat.h
	$(CC) -c fat.c

buf.o: buf.c buf.h queue.h
	$(CC) -c buf.c

Disk.o: Disk.c Disk.h
	$(CC) -c Disk.c

clean:
	-rm -f testcase.o fs.o fat.o buf.o Disk.o
	-rm -f $(TARGET)
