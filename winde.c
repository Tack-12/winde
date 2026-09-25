#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define WINDE_VERSION "0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL , 0}

typedef struct {
    int screenrows;
    int screencols;
    struct termios original_term;
} editorConfig;


typedef struct {
    char* b;
    int len;
}abuf;

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

    if(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDIN_FILENO, "\x1b[999C\x1b[999B,",12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void abAppend(abuf *ab , const char *s , int len){
    char * new = realloc(ab->b , ab->len + len);

    if (new == NULL) return ;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(abuf *ab){
    free(ab->b);
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

void editorDrawRows(abuf *ab){
    int y;

    for (y = 0; y < E.screenrows ; y++){
        if( y == E.screenrows/3){
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome), "WINDE -- Version %s", WINDE_VERSION);

            if(welcomelen > E.screencols) welcomelen = E.screencols;
            int padding = (E.screencols - welcomelen)/2;
            if(padding){
                abAppend(ab, "~", 1);
                padding -- ;
            }
            while (padding--) {
                abAppend(ab, " ", 1);

            }
            abAppend(ab, welcome, welcomelen);
        }else{

        abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[k", 3);

        if (y < E.screenrows -1){
            abAppend(ab,"\r\n", 2);
        }
    }
}

void editorRefreshScreen(){

    abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDERR_FILENO,ab.b, ab.len);
    abFree(&ab);
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
