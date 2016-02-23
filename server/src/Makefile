ELF:=../bin/server
OBJ:=child.c main.c send_fd.c pool_n.c
$(ELF):$(OBJ)
	gcc $^ -o $@ -lcrypt  -I../include

.PHONY:clean
clean:
	rm -rf ../bin/server
