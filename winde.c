#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

struct termios original_term;

void die ( const char *s){
    perror(s);
    exit(1);
}

void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term) == -1) die("tcsetattr");
}

void enableRawMode(){

    if (tcgetattr(STDERR_FILENO, &original_term) == -1) die("tcsetattr");
    atexit(disableRawMode);

    struct termios raw = original_term;
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if( tcsetattr(STDERR_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int main(){
    enableRawMode();

    while( 1 ){
        char c = '\0';

        if(read(STDIN_FILENO, &c , 1) == -1 && errno != EAGAIN) die("read");
        if(iscntrl(c)){
            printf("%d\n", c);
        }
        else{
            printf("%d ('%c')\n", c ,c );
        }
        if( c == 'q') break;
    }

    return 0;
}
