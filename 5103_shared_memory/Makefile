all: consumer.c producer.c main.c
	gcc -o consumer consumer.c -pthread
	gcc -o producer producer.c -pthread
	gcc -o main main.c -pthread
	
clean:
	rm consumer producer main PRODUCER_BLACK.txt PRODUCER_RED.txt PRODUCER_WHITE.txt
