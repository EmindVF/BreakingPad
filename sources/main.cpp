#include <iostream> 
#include <fstream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string>
#include <cstring>

#define BREAKINGPAD_VERSION "0.0.1"
#define TAB_STOP 8
#define QUIT_TIMES 3

//char (1-3) method
//char (x) x-1 times failed
//char 

//assembler
extern "C" void readChar(char* c, int* nread);
extern "C" void readFile(char* fileName,char** dest,char* e);
extern "C" void writeFile(char* fileName,char* source,char* e);

extern "C" void aCipherCaesar(char* source, char* dest,char* key);
extern "C" void aCipherSubstitution(char* source, char* dest,char* key);
extern "C" void aCipherTransposition(char* source, char* dest, char* key);

extern "C" void aDecipherCaesar(char* source, char* dest,char* key);
extern "C" void aDecipherSubstitution(char* source, char* dest,char* key);
extern "C" void aDecipherTransposition(char* source, char* dest,char* key);

//Ого, вот это да, вот это вы придумали 
#define CTRL_KEY(K) ((K) & 0x1f)

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP ,
    ARROW_DOWN,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
    DELETE_KEY 
};

// ECHO - Эхо символов в консоль
// ICANON - чтение посимвольно, а не построчно
// ISIG - очистить комбинации ctrl-c ctrl-z
// IXON - ctrl-s, ctrl-q (стоп/продолжить ввод данных в терминал)
// IEXTEN - ctrl-v 
// ICRNL - ctrl-m, ctrl-j
// OPOST - отключение автоматичкского \r
// BRKINT, INPCK, ISTRIP, CS8 - traditional flags for raw mode 
// VMIN - min number of bytes before read returns
// VTIME - max time to wait before read returns (in 100 ms)

// \x1b[2J - escape sequence, J - clear, [2 - entire screen
// \x1b[H  - reposition the cursor

//data
struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
};

struct editorConfig{
    struct termios origTermios;

    int screenrows;
    int screencols;

    int cx, cy;
    int rx;

    int rowoff;
    int coloff;

    int numrows;
    erow *row;

    char *filename;

    char statusmsg[80];
    time_t statusmsg_time;

    int dirty;
};

editorConfig E;

//some functions defines because bruh memento
void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt);
void cipherCaesar();
void cipherSubstitution();
void cipherTransposition();


//terminal
void die(const char *s) {
    // clear the screen
    write(STDOUT_FILENO,"\x1b[2J",4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.origTermios)==-1){
        die("tcsetattr");
    };
}

void enableRawMode(){
    if(tcgetattr(STDIN_FILENO, &E.origTermios)==-1){
        die("tcgetattr");
    }
    atexit(disableRawMode);

    struct termios raw = E.origTermios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDERR_FILENO, TCSAFLUSH, &raw)==-1){
        die("tcsetattr");
    }
}

int editorReadKey() {

    //while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        //if (nread == -1 && errno != EAGAIN) die("read");
    //}

    int nread=0;
    char c=0;
    do{
        readChar(&c, &nread);
    }while(nread!=1);

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if(seq[1]>='0' && seq[1]<='9'){
                if(read(STDIN_FILENO, &seq[2], 1)!=1){
                    return '\x1b';
                }
                if(seq[2]=='~'){
                    switch (seq[1]){
                        case '1': return HOME_KEY;
                        case '3': return DELETE_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
    return c;
}

int getWindowSize(int *rows, int *cols){
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)==-1||ws.ws_col==0){
        return -1;
    } else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

int getCursorPosition(int *rows, int *cols) {
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    printf("\r\n");
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
    }
    editorReadKey();
    return -1;
}

//erow operations
int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        rx++;
    }
    return rx;
}

void editorUpdateRow(erow *row) {
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;
    free(row->render);
    row->render = (char*)malloc(row->size + tabs*(TAB_STOP-1) + 1);
    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void editorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.numrows) return;
    E.row = (erow*)realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    E.row[at].size = len;
    E.row[at].chars = (char*)malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize=0;
    E.row[at].render=NULL;
    editorUpdateRow(&E.row[at]);

    E.numrows++;
    E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = (char*)realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = (char *)realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorSetRowsToString(char* s){
    int y;
    for (y = 0; y < E.numrows; y++) 
    {
        free(E.row[y].chars);
        free(E.row[y].render);
    }
    free(E.row);

    E.row = NULL;
    E.numrows = 0;

    const char* delimiter = "\n\r";
    char* token = std::strtok(s, delimiter);
    while (token != nullptr) {
        editorInsertRow(E.numrows,token, strlen(token));
        token = std::strtok(nullptr, delimiter);
    }

    E.dirty = 0;
}
//editor operations
void editorInsertChar(int c){
    //if on tilde
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows,const_cast<char*>(""), 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorDelChar() {
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;
    erow *row = &E.row[E.cy];
    if (E.cx > 0) {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

void editorInsertNewline() {
    if (E.cx == 0) {
        editorInsertRow(E.cy, const_cast<char*>(""), 0);
    } else {
        erow *row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

//file i/o
char * editorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1;
    *buflen = totlen;
    char *buf = (char*)malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    *p=0;
    return buf;
}

void editorOpen(){
    char* tempName = editorPrompt(const_cast<char*>("File name: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("File opening aborted");
        return;
    }

    FILE *fp = fopen(tempName, "r");
    if (fp) {
        free(E.filename);
        E.filename = tempName;
        //clear rows
        int y;
        for (y = 0; y < E.numrows; y++) 
        {
            free(E.row[y].chars);
            free(E.row[y].render);
        }
        free(E.row);
        E.row = NULL;
        E.numrows = 0;

        char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;
        while ((linelen = getline(&line, &linecap, fp)) != -1) {
            while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
                linelen--;
            editorInsertRow(E.numrows,line, linelen);
        }
        free(line);
        fclose(fp);
        E.dirty = 0;
        editorSetStatusMessage("File opened successfully");
        return;
    }
    editorSetStatusMessage("Cannot open file");
}

void editorSave() {

    char* tempName = editorPrompt(const_cast<char*>("Save as: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Save aborted");
        return;
    }
    /*
    if (E.filename == NULL) {
        E.filename = editorPrompt(const_cast<char*>("Save as: %s (ESC to cancel)"));
        if (E.filename == NULL) {
            editorSetStatusMessage("Save aborted");
            return;
        }
    }*/

    int len;
    char *buf = editorRowsToString(&len);

    //safe to file
    int fd = open(tempName, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                editorSetStatusMessage("%d bytes written to disk", len);

                free(E.filename);
                E.filename = tempName;

                E.dirty = 0;
                return;
            }
        }
        close(fd);
    }
    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
    E.dirty = 0;
}

//cipher
void editorCipher(){
    char* tempName = editorPrompt(const_cast<char*>("Method (\"1\"-Caesar,"
        "\"2\"-Substitution,\"3\"-Transposition): %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Cipher aborted");
        return;
    }
    if(tempName[0]=='\0'){
        free(tempName);
        editorSetStatusMessage("Invalid method choice");
        return;
    }
    if(tempName[0]=='1'&&tempName[1]=='\0'){
        free(tempName);
        cipherCaesar();
        return;
    }
    if(tempName[0]=='2'&&tempName[1]=='\0'){
        free(tempName);
        cipherSubstitution();
        return;
    }
    if(tempName[0]=='3'&&tempName[1]=='\0'){
        free(tempName);
        cipherTransposition();
        return;
    }
    free(tempName);
    editorSetStatusMessage("Invalid method choice");
    return;
}

void cipherCaesar(){
    char* tempName = editorPrompt(const_cast<char*>("Choose key (1-25): %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Cipher aborted");
        return;
    }
    int k;
    try {
        k = std::stoi(tempName);
        free(tempName);
    } catch (const std::exception& e) {
        free(tempName);
        editorSetStatusMessage("Invalid key");
        return;
    }
    if(k<1||k>25){
        editorSetStatusMessage("Invalid key");
        return;
    }
    tempName = editorPrompt(const_cast<char*>("Choose filename: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Cipher aborted");
        return;
    }
    char t = k;
    char e = 0;
    int len;
    char* source = editorRowsToString(&len);
    //source[len]=0;
    char* dest = (char*)malloc(len+1+3);
    dest[0] = 1;//false attempts-1
    dest[1] = 1;//method = 1
    dest[2] = t;
    dest[len+1+2]=0;

    //writeFile(const_cast<char*>("last_source.txt"), source, &e);
    //writeFile(const_cast<char*>("last_new_source.txt"), new_source, &e);
    //writeFile(const_cast<char*>("last_dest_tran_ci.txt"), dest, &e);

    aCipherCaesar(source, dest+3, &t);
    writeFile(tempName, dest, &e);

    free(source);
    free(tempName);
    free(dest);

    if(e){
        editorSetStatusMessage("There was an error");
        return;
    } else{
        editorSetStatusMessage("Successfull");
        return;
    }
}

void cipherTransposition(){
    char* tempName = editorPrompt(const_cast<char*>("Choose key (2-255): %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Cipher aborted");
        return;
    }
    int k;
    try {
        k = std::stoi(tempName);
    } catch (const std::exception& e) {
        free(tempName);
        editorSetStatusMessage("Invalid key");
        return;
    }
    if(k<2||k>255){
        editorSetStatusMessage("Invalid key");
        free(tempName);
        return;
    }
    tempName = editorPrompt(const_cast<char*>("Choose filename: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Cipher aborted");
        return;
    }
    char e = 0;
    int len;
    char* source = editorRowsToString(&len);
    //source[len]=0;
    int bytesToAdd=(k-len%k)%k;

    //disableRawMode();
    //std::cout<<len<<" "<<k<<" "<<bytesToAdd;
    //exit(0);

    char* new_source=(char*)calloc(len+bytesToAdd+1,1);
    for(int i=0;i<len+bytesToAdd;i++){
        new_source[i]=' ';
    }
    new_source[len+bytesToAdd]=0;
    memcpy(new_source,source, len);

    char* dest = (char*)malloc(len+bytesToAdd+1+3);
    dest[0] = 1;//false attempts-1
    dest[1] = 3;//method = 1
    dest[2] = char(k);
    dest[len+bytesToAdd+1+2]=0;

    char t = k;

    //writeFile(const_cast<char*>("last_source.txt"), source, &e);
    //writeFile(const_cast<char*>("last_new_source.txt"), new_source, &e);
    //writeFile(const_cast<char*>("last_dest_tran_ci.txt"), dest, &e);

    aCipherTransposition(new_source, dest+3, &t);
    writeFile(tempName, dest, &e);

    free(source);
    free(tempName);
    free(dest);
    free(new_source);

    if(e){
        editorSetStatusMessage("There was an error");
        return;
    } else{
        editorSetStatusMessage("Successfull");
        return;
    }
}

void cipherSubstitution(){
    char* tempName = editorPrompt(const_cast<char*>("Choose keyword (maxlen = 26, unique letters): %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Cipher aborted");
        return;
    }
    int n = strlen(tempName);
    bool nonAlpha = false;
    char a[26]{};

    for(int i = 0;i<n;i++){
        if(!isalpha(tempName[i])){
            nonAlpha = true;
            break;
        }
        a[tolower(tempName[i])-'a']++;
    }
    if(nonAlpha){
        free(tempName);
        editorSetStatusMessage("Invalid keyword");
        return;
    }
    for(int i=0;i<26;i++){
        if(a[i]>1){
            nonAlpha = true;
            break;
        }
    }
    if(nonAlpha){
        free(tempName);
        editorSetStatusMessage("Invalid keyword");
        return;
    }

    char* key = (char*)malloc(27);
    key[26] = '\0';
    int i;
    for(i=0;i<n;i++){
        key[i]=tolower(tempName[i])-'a';
    }
    for(int j=0;j<26;j++){
        if(!a[j]){
            key[i]=j;
            i++;
        }
    }

    char* fileName = editorPrompt(const_cast<char*>("Choose filename: %s (ESC to cancel)"));
    if (fileName == NULL) {
        editorSetStatusMessage("Cipher aborted");
        return;
    }

    char e = 0;
    int len;
    char* source = editorRowsToString(&len);
    //source[len]=0;
    char* dest = (char*)malloc(len+1+2+26);
    dest[0] = 1;//false attempts-1
    dest[1] = 2;//method = 1
    for(int i=0;i<26;i++){
        dest[i+2]=key[i]+10;
    }
    dest[len+2+26]=0;

    aCipherSubstitution(source, dest+28, key);
    writeFile(fileName, dest, &e);

    free(source);
    free(tempName);
    free(fileName);
    free(dest);
    free(key);

    if(e){
        editorSetStatusMessage("There was an error");
        return;
    } else{
        editorSetStatusMessage("Successfull");
        return;
    }
}

void decipherCaesar(char* source, char* e){
    char key = source[2];
    char* tempName = editorPrompt(const_cast<char*>("Input key: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Decipher aborted");
        return;
    }

    int k;
    try {
        k = std::stoi(tempName);
    } catch (const std::exception& exc) {
        free(tempName);
        editorSetStatusMessage("Invalid key");
        (*e)++;
        return;
    }
    if(k<1||k>25){
        editorSetStatusMessage("Invalid key");
        free(tempName);
        (*e)++;
        return;
    }

    if (k!=key){
        editorSetStatusMessage("Invalid key");
        free(tempName);
        (*e)++;
        return;
    }
    int len = strlen(source+3);
    char* dest = (char*)malloc(len+1);
    dest[len]=0;

    aDecipherCaesar(source+3, dest, &key);

    editorSetRowsToString(dest);
    free(dest);
    free(tempName);
}

void decipherSubstitution(char* source,char* e){
    char* tempName = editorPrompt(const_cast<char*>("Input key: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Decipher aborted");
        return;
    }
    int n = strlen(tempName);
    bool nonAlpha = false;
    char a[26]{};

    for(int i = 0;i<n;i++){
        if(!isalpha(tempName[i])){
            nonAlpha = true;
            break;
        }
        a[tolower(tempName[i])-'a']++;
    }
    if(nonAlpha){
        free(tempName);
        editorSetStatusMessage("Invalid key");
        (*e)++;
        return;
    }
    for(int i=0;i<26;i++){
        if(a[i]>1){
            nonAlpha = true;
            break;
        }
    }
    if(nonAlpha){
        free(tempName);
        editorSetStatusMessage("Invalid key");
        (*e)++;
        return;
    }

    char* key = (char*)malloc(27);
    key[26] = '\0';
    int i;
    for(i=0;i<n;i++){
        key[i]=tolower(tempName[i])-'a';
    }
    for(int j=0;j<26;j++){
        if(!a[j]){
            key[i]=j;
            i++;
        }
    }
    char* skey = (char*)malloc(26);
    for(int i=0;i<26;i++){
        skey[i]=source[i+2]-10;
    }

    for(int i=0;i<26;i++){
        if(key[i]!=skey[i]){
            free(tempName);
            editorSetStatusMessage("Invalid key");
            (*e)++;
            return;
        }
    }
    int len=strlen(source+28);
    char* dest = (char*)malloc(len+1);
    dest[len]=0;

    aDecipherSubstitution(source+28, dest, key);

    editorSetRowsToString(dest);
    free(dest);
}

void decipherTransposition(char* source,char* e){
    char key = source[2];
    char* tempName = editorPrompt(const_cast<char*>("Input key: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("Decipher aborted");
        return;
    }

    int k;
    try {
        k = std::stoi(tempName);
    } catch (const std::exception& err) {
        free(tempName);
        editorSetStatusMessage("Invalid key");
        (*e)++;
        return;
    }
    if(k<2||k>255){
        free(tempName);
        editorSetStatusMessage("Invalid key");
        (*e)++;
        return;
    }

    if (k!=static_cast<unsigned char>(key)){
        editorSetStatusMessage("Invalid key");
        free(tempName);
        (*e)++;
        return;
    }

    int len = strlen(source+3);
    char* dest = (char*)malloc(len+1);
    dest[len]=0;

    aDecipherTransposition(source+3, dest, &key);

    editorSetRowsToString(dest);
    free(dest);
}

void editorDecipher(){
    char* tempName = editorPrompt(const_cast<char*>("File name: %s (ESC to cancel)"));
    if (tempName == NULL) {
        editorSetStatusMessage("File opening aborted");
        return;
    }
    char e=0; 
    char* dest;
    readFile(tempName, &dest, &e);
    if(e){
        editorSetStatusMessage("kek");
        free(tempName);
        editorSetStatusMessage("There was an error opening the file");
        return;
    }

    char m = dest[1];
    e = dest[0]-1;
    switch(m){
        case 1:
            decipherCaesar(dest,&e);
            break;
        case 2:
            decipherSubstitution(dest,&e);
            break;
        case 3:
            decipherTransposition(dest,&e);
            break;
        default:
            editorSetStatusMessage("There was an error");
            return;
    }

    if(e<4){
        std::fstream file(tempName, std::ios::in | std::ios::out | std::ios::binary);
        file.seekp(0);
        file.put(e+1);
        file.close();

    } else {
        std::remove(tempName);
        editorSetStatusMessage("Too many failed tries, file was deleted");
    }
    E.filename = tempName;
    free(dest);
    return;
}

//buffer 
struct abuf {
    char *b;
    int len;
};

void abAppend(struct abuf *ab, const char *s, int len) {
    char *newm = (char*)realloc(ab->b, ab->len + len);
    if (newm == NULL) return;
    memcpy(&newm[ab->len], s, len);
    ab->b = newm;
    ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

//input
char *editorPrompt(char *prompt) {
    size_t bufsize = 128;
    char *buf =(char*) malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';
    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();
        int c = editorReadKey();
        if (c == DELETE_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (c == '\x1b') {
            editorSetStatusMessage("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage("");
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = (char*)realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}

void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows)? NULL : &E.row[E.cy];

    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if(row && E.cx < row->size){
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
            }
            break;
    }
    //fix long right then down
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

void editorProcessKeypress() {
    static int quit_times = QUIT_TIMES;

    int c = editorReadKey();
    switch (c) {
        case '\r':
            editorInsertNewline();
            break;

        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0) {
                editorSetStatusMessage("WARNING!!! File has unsaved changes. "
                    "Press Ctrl-Q %d more times to quit.", quit_times);
                quit_times--;
                return;
            }
            //clear the screen
            write(STDOUT_FILENO,"\x1b[2J",4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case CTRL_KEY('s'):
            editorSave();
            break;

        case CTRL_KEY('o'):
            editorOpen();
            break;
        
        case CTRL_KEY('c'):
            editorCipher();
            break;
        
        case CTRL_KEY('d'):
            editorDecipher();
            break;

        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DELETE_KEY:
            if (c == DELETE_KEY) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    E.cy = E.rowoff;
                } else if (c == PAGE_DOWN) {
                    E.cy = E.rowoff + E.screenrows - 1;
                    if (E.cy > E.numrows) E.cy = E.numrows;
                }

                int times = E.screenrows;
                while (times--){
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
                }
            }
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            editorInsertChar(c);
            break;
    }
    quit_times = QUIT_TIMES;
}

//output 
void editorScroll() {
    E.rx = 0;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }

    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.cx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.cx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }
}

void editorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        //scroll offset
        int filerow = y + E.rowoff;
        //after text lines
        if(filerow>= E.numrows){
            //version with pad if no file
            if (E.numrows==0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                "Breaking Pad editor -- version %s", BREAKINGPAD_VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            //text itself
            int len = E.row[filerow].rsize - E.coloff;
            if(len<0) len = 0;
            if (len > E.screencols) len = E.screencols;
            abAppend(ab, &E.row[filerow].render[E.coloff], len);
        }
        
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab,"\r\n",2);
        
    }
}

void editorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);
    char status[120], rstatus[120];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
        E.filename ? E.filename : "[No Name]", E.numrows,
        E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
        E.cy + 1, E.numrows);
    if (len > E.screencols) len = E.screencols;
    abAppend(ab, status, len);
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen> E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen(){
    //this is experemental
    if(getWindowSize(&E.screenrows, &E.screencols)==-1){
        die("getWindowSize");
    }
    E.screenrows-=2;

    editorScroll();

    struct abuf ab = {NULL, 0};
    
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab,"\x1b[H", 3);
    
    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,(E.rx - E.coloff)+ 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO,ab.b, ab.len);
    abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

//init
void initEditor(){
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff=0;
    E.coloff=0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;

    if(getWindowSize(&E.screenrows, &E.screencols)==-1){
        die("getWindowSize");
    }
    E.screenrows-=2;
}

int main(){

    enableRawMode();
    initEditor();

    editorSetStatusMessage("HELP: Ctrl + : S = save | O = open | Q = quit |"
        " C = cipher | D = decipher");

    while(true){
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}