.PHONY: all clean client storage_server naming_server mv

all: client storage_server naming_server clean mv

client:
	cd ./client && \
	gcc -c client.c && \
	gcc -c shell.c && \
	gcc -c ../common/helper.c && \
	gcc -o client_exe client.o shell.o helper.o

storage_server:
	cd ./storage_server && \
	gcc -c functionalities.c && \
	gcc -c storage_server.c && \
	gcc -c ../common/helper.c && \
	gcc -o ss storage_server.o functionalities.o helper.o

naming_server:
	cd ./naming_server && \
	gcc -c server.c && \
	gcc -c FileSys.c && \
	gcc -c ../common/helper.c && \
	gcc -o nfs server.o FileSys.o helper.o

clean:
	find . -name "*.o" -type f -delete

# mv client storage_server nfs to .

mv:
	cd ..
	mv ./client/client_exe ./storage_server/ss ./naming_server/nfs .