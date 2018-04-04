.PHONY :clean

main : threadpool.o cond.o main.o
	gcc -g -Wall $^ -o $@ -lpthread -lrt 
%.o : %.c
	gcc -c $^ -o $@ -lpthread -lrt

clean:
	rm -rf *.o
