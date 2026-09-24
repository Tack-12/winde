#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

typedef struct {
    int screenrows;
    int screencols;
    struct termios original_term;
} editorConfig;

editorConfig E;

void clearScreen();

/* Clean up Function. */
void die ( const char *s){
    clearScreen();
    perror(s);
    exit(1);
}

/* Changes Raw mode and sets it to the original Term */
void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_term) == -1) die("tcsetattr");
}

/* Enables the Raw mode using the orgi Term */
void enableRawMode(){

    if (tcgetattr(STDERR_FILENO, &E.original_term) == -1) die("tcsetattr");
    atexit(disableRawMode);

    struct termios raw = E.original_term;
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

int getCursorPosition(int *rows, int *cols){
    char buf[32];
    unsigned int i = 0;

    if(write(STDERR_FILENO, "\x1b[6n", 4) != 4) return -1;

    while( i < sizeof(buf) - 1) {
        if( read (STDERR_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++ ;
    }
    buf[i] = '\0';
    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

/* Change the rows and cols using winsize from ioctl */
int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(1|| ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDIN_FILENO, "\x1b[999C\x1b[999B,",12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
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

    for (y = 0; y < E.screenrows ; y++){
        write(STDIN_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen(){

    clearScreen();
    editorDrawRows();

    write(STDERR_FILENO, "\x1b[H", 3);
}

void clearScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void initEditor(){
    if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){
    enableRawMode();
    initEditor();

    while( 1 ){
        editorRefreshScreen();
        editorProcessKeyPress();
    }

    return 0;
}
