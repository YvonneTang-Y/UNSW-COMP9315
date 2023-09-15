#include <stdlib.h>
#include <stdio.h>


void giveUp(char *);

int main(int argc, char argv[]){
    int page_out;
    int page_in;
    int slot;
    char *strategy;   

    if (argc < 5) giveUp('Insufficient args');
    page_out = atoi(argv[1]);
    page_in = atoi(argv[2]);
    slot = atoi(argv[3]);
    strategy = argv[4];

    if 

}




void giveUp(char *msg){
    fprintf(stderr, "Error: %s\n", msg);
    fprintf(stderr, "Usage: ./buffer_usage BlockSize InputFile\n");
    exit(EXIT_FAILURE);
}
