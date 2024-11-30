gcc -Wall -Wextra -o phantomid-with-tree \
    ./main.c network.c phantomid.c \
    -I/usr/include/openssl \
    -L/usr/lib \
    -lssl -lcrypto -pthread -lrt