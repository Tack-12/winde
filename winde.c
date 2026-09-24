#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define CTRL_KEY(k) ((k) & 0x1f)


struct termios original_term;


void clearScreen();

/* Clean up Function. */
void die ( const char *s){
    clearScreen();
    perror(s);
    exit(1);
}

/* Changes Raw mode and sets it to the original Term */
void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term) == -1) die("tcsetattr");
}

/* Enables the Raw mode using the orgi Term */
void enableRawMode(){

    if (tcgetattr(STDERR_FILENO, &original_term) == -1) die("tcsetattr");
    atexit(disableRawMode);

    struct termios raw = original_term;
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if( tcsetattr(STDERR_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

/* Read Key Press */

char editorReadKey(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO,&c ,1)) != 1){
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/* Input the Key */

void editorProcessKeyPress(){
   char c = editorReadKey();

   switch (c){
       case CTRL_KEY('q'):
           clearScreen();
           exit(0);
           break;
   }
}

/*Function uses VT100 Escape Sequences which can be found here
 * https://espterm.github.io/docs/VT100%20escape%20codes.html
 */

void editorDrawRows(){
    int y;

    for (y = 0; y < 24; y++){
        write(STDIN_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();

    write(STDERR_FILENO, "\x1b[H", 3);
}

void clearScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}



int main(){
    enableRawMode();

    while( 1 ){
        editorRefreshScreen();
        editorProcessKeyPress();
    }

    return 0;
}
