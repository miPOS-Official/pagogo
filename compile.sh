ulimit -Sn 1048576 ; VAL=$( ulimit -Sn ) ; echo -e "MAX_FILENO: $VAL" 

time g++ -o main main.cpp -I./Modules -lssl -lcrypto -lz -std=c++14 ; ./main