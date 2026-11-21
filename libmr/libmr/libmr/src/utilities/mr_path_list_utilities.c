#define _POSIX_C_SOURCE 200809L
#include "mr_path_list_utilities.h"
#include "mr_protocol_utilities.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>
#include <dirent.h>

int mr_path_list_init(mr_path_list_t* l){
    //controlla che il puntatore l non sia NULL
    if(l== NULL){errno = EINVAL; return -1;}
    // iniziallizza le altre variabili
    l->capacity = MR_PATH_LIST_START_CAPACITY;
    l->count = 0;
    l->total_line_count = 0;
    // fa una malloc per char* quindi arr è un puntatore con spazio [[char*],*MR_PATH_LIST_START_CAPACITY]
    l->arr = (char**)malloc(sizeof(char*)*MR_PATH_LIST_START_CAPACITY);
    if(l->arr== NULL){return -1;}
    return 0;
}

int mr_path_list_add_realloc(mr_path_list_t* l, char* p){
    //controlla che il puntatore l e il path non siano NULL
    if(l== NULL||p==NULL){errno = EINVAL; return -1;}
    //alloco lo spazio per fare una copia integrale di p; quindi uso strlen e aggiungo 1 per il '\0'
    char* path = (char*)malloc(strlen(p)+1);
    if(path == NULL){return -1;}
    //copio la stringa, strcpy copia anche \0
    strcpy(path,p);
    //controllo se ho finito lo spazio
    if (l->count == l->capacity)
    {   
        //creo un un array con realloc(copio il contenuto di arr e aggiungo altro spazio)
        char** temp=realloc(l->arr,(sizeof(char*)*l->capacity*MR_PATH_LIST_GROW_FACTOR));
        if(temp == NULL){ free(path);return -1;}
        else{
            //assegno ad arr il puntatore all'array appena creato
            l->arr = temp;
            // aggiorno la variabile capacity perchè rappresenti effettivamente la capacity di l
            l->capacity *= MR_PATH_LIST_GROW_FACTOR;
            //aggiungo path a l
            l->arr[l->count] = path;
            //aggiorno il numero di elementi presenti all'interno di l
            l->count++;
        }
    }
    else{
        //se ho abbastanza spazio allora aggiungo semplicemente l'elemento
        l->arr[l->count] = path;
        l->count++;
    }
    return 0;
}

static int cmp(const void* a,const void* b){
    //eseguo i cast per rappresentare i puntatori ai vari path
    const char* A = *(const char* const*) a;
    const char* B = *(const char* const*) b;
    //uso strcmp  come cmp 
    return strcmp(A,B);
}
int mr_path_list_reorder(mr_path_list_t* l){
    //controlla che il puntatore l sia null
    if(l == NULL){errno = EINVAL; return -1;}
    //usa qsort su l->arr com cmp che usa strcmp
    qsort(l->arr,l->count,sizeof(char*),cmp);
    return 0;
}

int mr_path_list_destroy(mr_path_list_t* l){
    //controlla che il puntatore l sia null
    if(l == NULL){errno = EINVAL; return -1;}
    //itera l->arr liberandone gli elementi che erano stati allocati
    for (size_t i = 0; i < l->count; i++)
    {
        free(l->arr[i]);
    }
    //ne libera il guscio
    free(l->arr);
    return 0;
};

int mr_path_scan_recursive(char* path,mr_path_list_t* list){
    //controlla che il puntatore l e il path non siano null
    if( path == NULL || list == NULL){errno = EINVAL; return -1;};
    //dichiaro la struct stat che usero per discernere tra file e dir
    struct stat st_buf;
    // uso stat per fare un get degli atributi del file e ne metto il risultato in &st_buf status è usato per segnalare un errore
    int status = stat (path, &st_buf);
    if (status != 0) {
        return -1;
    }
    //controllo se stat è un REG ovvero un regular file e nel caso faccio un add a l
    if(S_ISREG(st_buf.st_mode)){
        if(mr_path_list_add_realloc(list,path) == -1){return -1;};
        return 0;
    }
    /*controllo se stat è un DIR ovvero una dir e nel caso faccio una chiamata ricorsiva mr_path_scan_recursive 
    su ogni elemento trane "." e ".." ovvero la dir attuale e la dir genitore per evitare un loop infinito*/
    else if (S_ISDIR(st_buf.st_mode))
    {
        //dichiaro la struct dirent che usero per leggere i path di ogni membro della dir
        struct dirent* de;
        // apro la lo stream della cartella indicata da path e ne salvo il puntatore in dir
        DIR* dir = opendir(path);
        if (dir == NULL){return -1;}
        //finchè readdir non mi ritorna NULL avanzo nel leggere gli elementi della dir
        while ((de = readdir(dir)) != NULL){
            // faccio un check su char d_name[] per assicurarmi di skippare "." e ".."
            if (!((strcmp(de->d_name,".") == 0) || (strcmp(de->d_name,"..")== 0)))
            {
                //uso il path per creare il path assoluto all'elemento che sto prendendo in considerazione, e lo assegno a new_path
                char* new_path;
                if(mr_path_build_from_name(de->d_name,path,&new_path)){closedir(dir);return-1;};
                //faccio la chiamata ricorsiva a mr_path_scan_recursive(new_path,list), se è un file allora farà add sennò si ripeterà il processo
                if(mr_path_scan_recursive(new_path,list) == -1){closedir(dir);free(new_path);return -1;};
                //allocata nelle funzione ausiliaria mr_path_build_from_name
                free(new_path);
            }  
                   
        }
        //chiudo lo stream dir
        closedir(dir);
        return 0;  
    }
    return -1;
}

int mr_path_build_from_name(char* filename, char* path, char** result){
    // controlla che tutti i puntatori siano diversi da NULL
    if(filename == NULL || path == NULL || result == NULL){errno = EINVAL; return -1;};
    //spazio per la stringa di result il +2 sono il char '/' e il terminatore di stringa '\0'
    size_t len = strlen(path) + strlen(filename) + 2;
    //libero lo spazio per la stringa nel punto deferenziato da result(puntatore a puntatore)
    *result = (char*)malloc(len);
    if(*result == NULL){return -1;}

    //compongo la stringa e la scrivo nel punto deferenziato da result
    snprintf(*result, len, "%s/%s", path, filename);
    return 0;
}
int mr_path_list_send_lines_to_mapper(mr_path_list_t* l,int (*sender)(int fd, const char *file_name, size_t file_name_len,unsigned long line_number, const char *line, size_t line_len),int fd, mr_log_ctx_t *log_ctx){
    // controlla che tutti i puntatori siano diversi da NULL
    if(l == NULL||sender == NULL || log_ctx== NULL){errno = EINVAL; return -1;}
    /* dichiaro le due variabili che dovrò poi passare al sender
       line_number : numero della linea all'interno del file che sto elaborando
       read :numero di char letti da getline, dimensione effettiva della linea da mandare*/
    size_t line_number;
    ssize_t read;

    /*Variabili per getline
     line_buff = NULL,buffer per la linea che devo mandare, getline alloca automaticamente la memoria per line
      line_buff_size, la dimensione del buffer, getline lo usa per  sapere la capacity del buffer se la linea è più grande fa una realloc
    */
    size_t line_buff_size = 0;
    char* line_buff = NULL;

    //primo loop che itera tutti i file all'interno di l
    for (size_t i = 0; i < l->count; i++)
    {
        //setto la line_numeber a 1 ogni volta che cambio file
        line_number = 1;
        //apro il lo stream del file l->arr[i] in modalità lettura e lo assegno a current_file
        FILE* current_file = fopen(l->arr[i],"r");
        if(current_file == NULL){ free(line_buff);return -1;}
        //[LOG] faccio il logging dell'apertura dei vari input files
        mr_log_write(log_ctx, "main", "INPUT_OPEN", "-", "file input {%s} aperto",l->arr[i]);
        //leggo tutto il file linea per linea fino a che getline non ritorna -1
        while ((read = getline(&line_buff,&line_buff_size,current_file)) != -1)
        {   
            //controllo se l'ultimo char è un newline
            if (line_buff[read-1] == '\n')
            {
                //sostituisco il newline con il terminator
                read--;
                line_buff[read] ='\0';
            }

            //uso la funzione sender per mandare a fd la linea
            if(sender(fd,l->arr[i],strlen(l->arr[i]),line_number,line_buff,read)== -1){
                //[LOG] faccio il logging della chiusura dei vari input files
                mr_log_write(log_ctx, "main", "INPUT_CLOSED", "-", "file input {%s} chiuso PER ERRORE",l->arr[i]);   
                fclose(current_file);
                free(line_buff);
                return -1;};
            line_number++;
            l->total_line_count++;

        }
        //chiudo il file su cui stavo lavorando
        fclose(current_file);
        //[LOG] faccio il logging della chiusura dei vari input files
        mr_log_write(log_ctx, "main", "INPUT_CLOSED", "-", "file input {%s} chiuso",l->arr[i]);
    }
    //libero il line_buff una volta che ho finito di mandare tutti i file
    free(line_buff);
    return 0;
}

//TO-DO forse questa funzione non dovrebbe essere logicamente in questo file ma per ora non so dove metterla

int mr_receive_from_reducer(int (*receiver)(int fd, char **token, void **result, size_t *result_len,size_t *token_len),int fd,char* output_path,mr_log_ctx_t *log_ctx){
    // controlla che tutti i puntatori siano diversi da NULL
    if(receiver == NULL || output_path == NULL){errno = EINVAL; return -1;}
    //dichiaro tutte le variabili locali che verranno sovrascritte dalla funzione receiver
    char* token = NULL;
    void* result = NULL;
    size_t result_len;
    size_t token_len;

    //inf mi serve per ottenere info sullo stato della ricezione
    int inf;

    //apro lo stream del file di output in modalità scrittura
    FILE* output = fopen(output_path,"w");
    if(output == NULL){return -1;}
     //[LOG] faccio il logging dell'apertura del output file
     mr_log_write(log_ctx, "main", "OUTPUT_OPEN", "-", "file output {%s} aperto",output_path);
    //itero sulla pipe con la funzione receiver finchè mi ritorna 0
    while ((inf = receiver(fd,&token,&result,&result_len,&token_len))==0)
    {
        //scrivo sull'output direttamente i byte ricevuti dalla pipe
        fwrite(&token_len,sizeof(size_t),1,output);
        fwrite(token,token_len,1,output);
        fwrite(&result_len,sizeof(size_t),1,output);
        fwrite(result,result_len,1,output);
        //libero le risorse che non utilizzo più e le riassegno per evitare errori
        free(token);
        free(result);
        token = NULL;
        result = NULL;
        
    }
    //se inf è uguale a 1 allora la pipe è vuota e tutti i risultati sono stati trasmetti
    if (inf == 1)
    {
        //[LOG] Segno la ricezzione dell'output
        mr_log_write(log_ctx, "main", "OUTPUT_WRITTEN", "-", "output scritto in: {%s}", output_path);

        fclose(output);

        //[LOG] faccio il logging della chiusura del output file
        mr_log_write(log_ctx, "main", "OUTPUT_CLOSED", "-", "file output {%s} chiuso",output_path);

        return 0;
    }
    //se inf != 1 e != 0 allora c'è stato un problema
    else{
        fclose(output);
        //[LOG] faccio il logging della chiusura del output file
        mr_log_write(log_ctx, "main", "OUTPUT_CLOSED", "-", "file output {%s} chiuso PER ERRORE",output_path);
        free(token);
        free(result);
        return -1;
    }
}