#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_LINE_LENGHT 1024
#define MAX_COMMAND_LENGHT 32
#define INITIAL_MATRIX_LINES_NUMBER 8
#define INIT_CACHE_SIZE 1
#define INIT_STATE_CODE 0
#define CHANGE_STATE_CODE 1
#define DELETE_STATE_CODE 2

int numberOfWrittenLines;
int numberOfStoredLines;
bool read;

typedef struct{
    char** matrix;
    int* linesLenght;
} textMatrixType ;
textMatrixType textMatrix;

char* str; // todo: memory leak bitchhhh
char *line;

typedef struct {
    int numOfStrings;
    int typeOfCommand;
    char **cacheMatrix;
} cacheType;

cacheType* cache;
int cacheCurrPosition;
int cacheSize;
void saveDeleteStateinCache();
void resizeCache();

char* readCommand();
char* readLine();

void chooseCase(char* currCommand);

void changeLines(char const *currCommand);
void deleteLines(char const *currCommand);
void printLines(char const *currCommand);
void undoCommand(int);
void redoCommand(int);
void undoRedoPreRead(char* currCommand, int opType);

int intBeforeComma(const char*);
int intAfterComma(const char* str);

void reallocTextMatrixLines(int);


// todo: eliminare gli assegnamenti non fondamentali potrebbe ottimizzare il programma
// todo: consider writing strealloc or at least use strdup for writing  in cache
int main () {

    // used for gets warnings
    str  = (char*) malloc(MAX_LINE_LENGHT * sizeof(char));
    line  = (char*) malloc(MAX_LINE_LENGHT * sizeof(char));


    char currCommand[MAX_COMMAND_LENGHT]; //todo: do i ever need to free this?
    numberOfWrittenLines = 0;
    read = 1; // 0 is standard, 1 means no need to read because last iteration was a do/undo

    // todo: better to initialise when needed than on start
    textMatrix.matrix = (char**) malloc(INITIAL_MATRIX_LINES_NUMBER * sizeof(char*));
    textMatrix.linesLenght = (int*) malloc(INITIAL_MATRIX_LINES_NUMBER * sizeof(int));
    numberOfStoredLines = INITIAL_MATRIX_LINES_NUMBER;

    for (int i = 0; i < INITIAL_MATRIX_LINES_NUMBER; i++) {
        textMatrix.matrix[i] = (char*) malloc(1 * sizeof(char));
        strcpy(textMatrix.matrix[i], "");
        textMatrix.linesLenght[i] = 0;
    }

    // ------------------------------- cache -------------------------------
    cacheCurrPosition = 0;
    cacheSize = INIT_CACHE_SIZE;

    cache = (cacheType*) malloc(INIT_CACHE_SIZE * sizeof(cacheType));
    cache[0].cacheMatrix = (char**) malloc(sizeof(char*));
    cache[0].numOfStrings = 0;
    cache[0].typeOfCommand = INIT_STATE_CODE;
    // ---------------------------------------------------------------------

    do {
        if (read){
            strcpy(currCommand, readCommand());
        }
        chooseCase(currCommand);

    } while (strncmp("q", currCommand, strlen("q"))!=0);


    // free everything

    for (int j = 0; j < numberOfStoredLines; ++j) {
        free(textMatrix.matrix[j]);
    }
    free(textMatrix.linesLenght);
    free(textMatrix.matrix);

    for (int k = 0; k < cacheSize; ++k) {
        for (int m = 0; m < cache[k].numOfStrings; m++) {
            free(cache[k].cacheMatrix[m]);
        }
        free(cache[k].cacheMatrix);
    }
    free(cache);
    free(str);
    free(line);


    return 0;
}


// change, delete, print, undo, redo, quit
void chooseCase(char *currCommand){

    switch (currCommand[strlen(currCommand)-2]){
        case 'q':
            break;

        case 'c':
            changeLines(currCommand);
            read = 1;
            break;

        case 'd':
            deleteLines(currCommand);
            read = 1;
            break;

        case 'p':
            printLines(currCommand);
            read = 1;
            break;

        case 'u':
            undoRedoPreRead(currCommand, 0);
            read = 0;
            break;

        case 'r':
            undoRedoPreRead(currCommand, 1);
            read = 0;
            break;
    }
}

// todo: testare meglio resizing degli array dinamici
void changeLines(const char *currCommand){

    int k = 0;
    // todo: assignment only used for readability
    int startLine = intBeforeComma(currCommand) - 1;
    int lastLine = intAfterComma(currCommand) - 1;
    int numOfLinesToChange = lastLine - startLine + 1;

    //realloc matrix space if needed
    reallocTextMatrixLines(lastLine);

    //manage cache size
    resizeCache();
    // create space to save textMatrix in cache
    cacheSize += 1;
    cacheCurrPosition += 1; // care for first iteration
    cache = (cacheType*) realloc(cache, (cacheSize) * sizeof(cacheType));
    cache[cacheCurrPosition].cacheMatrix = (char**) malloc((numOfLinesToChange + 1) * sizeof(char*)); // todo: check for memory leak
    // create space for command
    cache[cacheCurrPosition].cacheMatrix[0] =  (char*) malloc((strlen(currCommand) + 2) * sizeof(char));

    // save the command in the first line
    strcpy(cache[cacheCurrPosition].cacheMatrix[0], currCommand);


    // save changes both to textMatrix and cache
    for (int i = startLine; i <= lastLine; i++) {
        // save in matrix
        strcpy(line, readLine());
        textMatrix.matrix[i] = (char*) realloc(textMatrix.matrix[i], (strlen(line) + 2) * sizeof(char));
        strcpy(textMatrix.matrix[i], line);
        textMatrix.linesLenght[i] = strlen(textMatrix.matrix[i]);
        // save in cache
        cache[cacheCurrPosition].cacheMatrix[k+1] =  (char*) malloc((textMatrix.linesLenght[i] + 2) * sizeof(char));
        strcpy(cache[cacheCurrPosition].cacheMatrix[k+1], line);
        k++;
    }

    // save number of written lines
    cache[cacheCurrPosition].numOfStrings = numOfLinesToChange + 1;

    // cache node code
    cache[cacheCurrPosition].typeOfCommand = CHANGE_STATE_CODE;

    // Aggiorna numero di righe del documento
    if(numberOfWrittenLines <= lastLine) {
        numberOfWrittenLines = lastLine + 1;
    }
}


// todo: ottimizzabile delete senza fffetto?
void deleteLines(const char *currCommand){

    int startLine = intBeforeComma(currCommand) - 1;
    int lastLine = intAfterComma(currCommand) - 1;
    int numDelLines = lastLine - startLine + 1;

    // fa scorrere le righe successive al posto di quelle da cancellare
    for (int j = lastLine + 1; j < numberOfWrittenLines; ++j) {
        textMatrix.matrix[j-numDelLines] = (char*) realloc(textMatrix.matrix[j-numDelLines], (strlen(textMatrix.matrix[j]) + 2) * sizeof(char));
        strcpy(textMatrix.matrix[j-numDelLines], textMatrix.matrix[j]);
        textMatrix.linesLenght[j-numDelLines] = strlen(textMatrix.matrix[j-numDelLines]);

    }


    // Aggiorna numero di righe del documento
    if (numberOfWrittenLines - numDelLines > 0){
        numberOfWrittenLines -= numDelLines;
    } else {
        numberOfWrittenLines = 0;
    }
    saveDeleteStateinCache();

}


void printLines(const char *currCommand){

    // todo: assignment only used for readability
    int startLine = intBeforeComma(currCommand) - 1;
    int lastLine = intAfterComma(currCommand) - 1;

    for (int i = startLine; i <= lastLine; ++i) {

        if ( i >= 0 && i < numberOfWrittenLines){
            printf("%s", textMatrix.matrix[i]);
            /*if(textMatrix.matrix[0][0] ==  '1' && textMatrix.matrix[0][1] == '1' && textMatrix.matrix[0][2] =='8'){
                printf("cacca");
            }*/
        } else {
            printf(".\n");
        }
    }

    //todo: delete
    //fflush(stdout);
}


void undoCommand(int numOfOperations){

    int desiredCacheIndex = cacheCurrPosition - numOfOperations;
    int numOfBackSteps = 1;
    int numOfForwardSteps = 1;
    int matrixLineCounter;
    int fullMatrixIndex;

    if (desiredCacheIndex < 0){
        desiredCacheIndex = 0;
    }

    // Initial or delete case -full matrix found
    if ( cache[desiredCacheIndex].typeOfCommand == DELETE_STATE_CODE || cache[desiredCacheIndex].typeOfCommand == INIT_STATE_CODE ){
        for (int i = 0; i < cache[desiredCacheIndex].numOfStrings; i++) {
            textMatrix.matrix[i] = (char*) realloc(textMatrix.matrix[i], ((strlen(cache[desiredCacheIndex].cacheMatrix[i]) + 2) *
                                                                          sizeof(char)));
            strcpy(textMatrix.matrix[i], cache[desiredCacheIndex].cacheMatrix[i]);
            textMatrix.linesLenght[i] = strlen(textMatrix.matrix[i]);
        }
        numberOfWrittenLines = cache[desiredCacheIndex].numOfStrings;


        // Change case -change command found
    } else if(cache[desiredCacheIndex].typeOfCommand == CHANGE_STATE_CODE) {

        // find nearest full matrix (going back in the cache)
        while (cache[desiredCacheIndex - numOfBackSteps].typeOfCommand == CHANGE_STATE_CODE) {
            numOfBackSteps++;
        }
        fullMatrixIndex = desiredCacheIndex - numOfBackSteps;

        // copy full matrix
        for (int p = 0; p < cache[fullMatrixIndex].numOfStrings; p++) {
            textMatrix.matrix[p] = (char *) realloc(textMatrix.matrix[p],
                                                    ((strlen(cache[fullMatrixIndex].cacheMatrix[p]) + 2) *
                                                     sizeof(char)));
            strcpy(textMatrix.matrix[p], cache[fullMatrixIndex].cacheMatrix[p]);
            textMatrix.linesLenght[p] = strlen(textMatrix.matrix[p]);
        }
        numberOfWrittenLines = cache[fullMatrixIndex].numOfStrings;

        // process all operation needed to return the desiredCache node
        int startLine;
        int lastLine;

        // for each needed step
        while (numOfForwardSteps <= numOfBackSteps) { // operate the change operation

            // parse command
            startLine = intBeforeComma(cache[fullMatrixIndex + numOfForwardSteps].cacheMatrix[0]) - 1;
            lastLine = intAfterComma(cache[fullMatrixIndex + numOfForwardSteps].cacheMatrix[0]) - 1;
            matrixLineCounter = 1; // starts at 1 in order to skip the first line, which contains the command

            // add/change lines
            for (int i = startLine; i <= lastLine; i++) {
                textMatrix.matrix[i] = realloc(textMatrix.matrix[i], (strlen(cache[fullMatrixIndex + numOfForwardSteps].cacheMatrix[matrixLineCounter]) + 2) * sizeof(char));
                strcpy(textMatrix.matrix[i], cache[fullMatrixIndex + numOfForwardSteps].cacheMatrix[matrixLineCounter]);
                textMatrix.linesLenght[i] = strlen(textMatrix.matrix[i]);
                matrixLineCounter++;
            }

            // Aggiorna numero di righe del documento
            if (numberOfWrittenLines <= lastLine) {
                numberOfWrittenLines = lastLine + 1;
            }
            numOfForwardSteps++;
        }

    }

    if(desiredCacheIndex > 0){
        cacheCurrPosition = desiredCacheIndex;
    } else {
        cacheCurrPosition = 0;
    }
}


void redoCommand(int numOfOperations){

    int desiredCacheIndex = cacheCurrPosition - numOfOperations;
    int numOfBackSteps = 1;
    int numOfForwardSteps = 1;
    int matrixLineCounter;
    int fullMatrixIndex;

    if (cacheSize <= desiredCacheIndex){
        desiredCacheIndex = cacheSize-1;
    }

    // Initial or delete case -full matrix found
    if ( cache[desiredCacheIndex].typeOfCommand == DELETE_STATE_CODE || cache[desiredCacheIndex].typeOfCommand == INIT_STATE_CODE ){
        for (int i = 0; i < cache[desiredCacheIndex].numOfStrings; i++) {
            textMatrix.matrix[i] = (char*) realloc(textMatrix.matrix[i], ((strlen(cache[desiredCacheIndex].cacheMatrix[i]) + 2) *
                                                                          sizeof(char)));
            strcpy(textMatrix.matrix[i], cache[desiredCacheIndex].cacheMatrix[i]);
            textMatrix.linesLenght[i] = strlen(textMatrix.matrix[i]);
        }

        // Change case -change command found
    } else if(cache[desiredCacheIndex].typeOfCommand == CHANGE_STATE_CODE) {

        // find nearest full matrix (going back in the cache)
        while (cache[desiredCacheIndex - numOfBackSteps].typeOfCommand == CHANGE_STATE_CODE) {
            numOfBackSteps++;
        }
        fullMatrixIndex = desiredCacheIndex - numOfBackSteps;

        // copy full matrix
        for (int p = 0; p < cache[fullMatrixIndex].numOfStrings; p++) {
            textMatrix.matrix[p] = (char *) realloc(textMatrix.matrix[p],
                                                    ((strlen(cache[fullMatrixIndex].cacheMatrix[p]) + 2) *
                                                     sizeof(char)));
            strcpy(textMatrix.matrix[p], cache[fullMatrixIndex].cacheMatrix[p]);
            textMatrix.linesLenght[p] = strlen(textMatrix.matrix[p]);
        }

        // process all operation needed to return the desiredCache node
        int startLine;
        int lastLine;

        // for each needed step
        while (numOfForwardSteps > numOfBackSteps) { // operate the change operation

            // parse command
            startLine = intBeforeComma(cache[fullMatrixIndex + numOfForwardSteps].cacheMatrix[0]) - 1;
            lastLine = intAfterComma(cache[fullMatrixIndex + numOfForwardSteps].cacheMatrix[0]) - 1;
            matrixLineCounter = 0;

            // add/change lines
            for (int i = startLine; i <= lastLine; i++) {
                strcpy(textMatrix.matrix[i], cache[fullMatrixIndex + numOfForwardSteps].cacheMatrix[matrixLineCounter]);
                textMatrix.linesLenght[i] = strlen(textMatrix.matrix[i]);
                matrixLineCounter++;
            }

            // Aggiorna numero di righe del documento
            if (numberOfWrittenLines <= lastLine) {
                numberOfWrittenLines = lastLine + 1;
            }
            numOfForwardSteps++;
        }

    }

    numberOfWrittenLines = cache[desiredCacheIndex].numOfStrings;

    if(desiredCacheIndex > 0){
        cacheCurrPosition = desiredCacheIndex;
    } else {
        cacheCurrPosition = 0;
    }
}

// questa funzione somma tutte le effettive undo e redo che può, finchè non trova altri comandi che devono essere eseguiti
// opType: 0 is UNDO, 1 is REDO, 2 is a different operation
void undoRedoPreRead(char* currCommand, int opType){

    int numOfOperations = 0;


    do {
        if(opType){
            // Redo Case
            numOfOperations -=  intBeforeComma(currCommand);
            if((cacheCurrPosition - numOfOperations) > cacheSize-1){
                numOfOperations = - (cacheSize - cacheCurrPosition - 1);
            }

        } else {
            // Undo Case
            numOfOperations += intBeforeComma(currCommand);

            if((cacheCurrPosition - numOfOperations) < 0){
                numOfOperations =  cacheCurrPosition;
            }
        }

        // read next line
        strcpy(currCommand, readCommand());

        switch (currCommand[strlen(currCommand)-2]){
            case 'u':
                opType = 0;
                break;
            case 'r':
                opType = 1;
                break;
            default:
                opType = 2;
                break;
        }

    } while (opType!=2);

    if(numOfOperations == 0){
        // do nothing
    } else if(numOfOperations > 0){
        // undo Case
        undoCommand(numOfOperations);

    } else if(numOfOperations < 0){
        // redo Case
        redoCommand(numOfOperations);
    }

}


char* readCommand(){
    str = fgets(str, MAX_COMMAND_LENGHT* sizeof(char), stdin);
    return str;
}


char* readLine(){
    str = fgets(str, MAX_LINE_LENGHT* sizeof(char), stdin);
    return str;
}


int intBeforeComma(const char* string){
    char *p;
    return strtol(string, &p, 10);
}


int intAfterComma(const char* string){
    char *p;
    char temp1[32];
    char* t;
    strcpy(temp1, string);
    strtok(temp1, ",");
    t = strtok(NULL, " ");
    return strtol(t, &p, 10);
}


void reallocTextMatrixLines(int lastLineNeeded){
    int numLinesToRealloc;

    // aggiunge ogni volta che serve il numero di linee che mancano per poter scrivere le nuove stringhe
    // todo: si dovrebbe raddoppiare lo spazio (ma l'implementazione provata non funziona!) - PROBABILMENTE ORA E' FACILE DA FARE!!!!!
    while (lastLineNeeded >= numberOfStoredLines){

        numLinesToRealloc = 4;

        // realloc per aumentare il numero di stringhe nella matrice
        textMatrix.matrix = realloc(textMatrix.matrix, (numberOfStoredLines + numLinesToRealloc) * sizeof(*textMatrix.matrix));
        textMatrix.linesLenght = realloc(textMatrix.linesLenght, (numberOfStoredLines + numLinesToRealloc) * sizeof(int));

        // Update numberOfStoredLines
        numberOfStoredLines +=  numLinesToRealloc;

        // Allocate memory for the new strings
        for (int i = numberOfStoredLines - numLinesToRealloc; i < numberOfStoredLines; ++i) {
            textMatrix.matrix[i] = (char*) malloc(1 * sizeof(char*));
        }
    }
}

// saves TextMatrixState in the Cache
// todo: OTTIMIZZARE! usa realloc ogni singola volta che si modifica la cache. Realloc è molto dispendiosa in termini di tempo,
//  conviene riallocare una quantità maggiore di memoria, in modo da non doverla riallocare ogni volta,
//  ricordarsi di introdurre unavriabile per tenere conto delleposizioni ccorrentemente occupate della cache, perchè ora come ora vengono occupate tutte
void saveDeleteStateinCache(){

    //manage cache size
    resizeCache();

    int i = 0;
    // create space to save textMatrix in cache

    cacheSize += 1;
    cacheCurrPosition += 1; // care for first iteration

    cache = (cacheType*) realloc(cache, (cacheSize) * sizeof(cacheType));

    cache[cacheCurrPosition].cacheMatrix = (char**) malloc((numberOfWrittenLines + 1) * sizeof(char*)); // todo: check for memory leak
    for (i = 0; i < (numberOfWrittenLines); ++i) {
        cache[cacheCurrPosition].cacheMatrix[i] =  (char*) malloc((textMatrix.linesLenght[i] + 2) * sizeof(char));
        // save textMatrix in cache line by line
        strcpy(cache[cacheCurrPosition].cacheMatrix[i], textMatrix.matrix[i]);
    }

    // save textMatrix in cache
    cache[cacheCurrPosition].numOfStrings = numberOfWrittenLines;

    // cache node code
    cache[cacheCurrPosition].typeOfCommand = DELETE_STATE_CODE;

    // imposta cache max size
    /*
    if(cacheSize > 3000){
        for (int j = 0; j < cache[cacheSize-3000].numOfStrings; ++j) {
            free(cache[cacheSize-3000].cacheMatrix[j]);
        }
    }
     */
}

// se necessario svuota gli indirizzi di cache successivi a quello corrente
void resizeCache(){

    if(cacheCurrPosition < cacheSize - 1){
        // free cache
        for (int i = cacheCurrPosition + 1; i < cacheSize; ++i) {
            for (int j = 0; j < cache[i].numOfStrings; ++j) {
                free(cache[i].cacheMatrix[j]);
            }
            free(cache[i].cacheMatrix);
        }

        cacheSize = cacheCurrPosition + 1;
        cache = (cacheType*) realloc(cache, (cacheSize) * sizeof(cacheType));
    }
}