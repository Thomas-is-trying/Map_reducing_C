#include "mr_protocol_utilities.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

ssize_t readn(int fd, void *buf, size_t n) {
    //variabile che indica il numero di byte n da leggere rimanenti
    size_t left = n;
    //faccio un cast a char* per usare l'aritmetica dei puntatori dato che char per convenzione è 1 byte
    char *b_ptr = (char*)buf;
    //ciclo che dura finché non ci sono più byte da leggere
    while (left > 0)
    {
        //variabile che mi dice quanti byte sono stati letti a questa iterazione
        ssize_t r = read(fd, b_ptr, left);
        if (r == -1)
        {
            //se l'errore che ha settato r a -1 è EINTR provocato da _exit(0) allora fa un continue
            if (errno == EINTR) {continue;}
            return -1;
        }
        /* EOF: ritorna quanto letto finora, ovvero m */
        if (r == 0) { return n - left; } 
        //aggiorno b_ptr aggiungendo r byte
        b_ptr += r;
        //aggiorno left rimuovendo r byte
        left -= r;
    }
    //ritorna il numero di byte letti
    return n;
}

ssize_t writen(int fd, const void *buf, size_t n) {
    //variabile che indica il numero di byte n da scrivere rimanenti
    size_t left = n;
    //faccio un cast a char* per usare l'aritmetica dei puntatori dato che char per convenzione è 1 byte
    char *b_ptr = (char*)buf;
    //ciclo che dura finché non ci sono più byte da scrivere
    while (left > 0) {
        //variabile che mi dice quanti byte sono stati scritti a questa iterazione
        ssize_t w = write(fd, b_ptr, left);
        
        if (w == -1)
        {
            //se l'errore che ha settato r a -1 è EINTR provocato da _exit(0) allora fa un continue
            if (errno == EINTR) { continue; }
            return -1;
        }
        //aggiorno b_ptr aggiungendo w byte
        b_ptr += w;
        //aggiorno left rimuovendo w byte
        left -= w;
    }
    //ritorna il numero di byte scritti
    return n;
}

int send_line_msg(int fd, const char *file_name, size_t file_name_len,unsigned long line_number, const char *line, size_t line_len) {
    //dichiaro una variabile header
    mr_line_header_t header;
    //costruisco l'header con le informazioni utili per la de-serelizzazione da parte del mapper
    header.file_name_len = file_name_len;
    header.line_length = line_len;
    header.line_number = line_number;

    // mando l'header tramite written
    if (writen(fd, &header, sizeof(header)) != (ssize_t)sizeof(header)) {return -1;}

    // mando il contenuto di filename tramite written
    if (writen(fd, file_name, file_name_len) != (ssize_t)file_name_len) {return -1;}
    // mando il contenuto di line tramite written
    if (writen(fd, line, line_len) != (ssize_t)line_len) {return -1;}
    return 0;
}

int recv_line_msg(int fd, char **file_name, size_t *file_name_len,unsigned long *line_number, char **line, size_t *line_len){
    //dichiaro una variabile header che userò per ricostruire il messaggio
    mr_line_header_t header;
    // inf mi dice lo stato del mio
    ssize_t inf;
    // uso readn per riempire header con le informazioni che mi servono
    inf = readn(fd, &header, sizeof(header));
    //controllo di aver ricevuto tutto
    if (inf != (ssize_t)sizeof(header)) {
        if (inf == 0) {
            return 1;  
            /* EOF pulito, nessun messaggio: non è un errore */
        }
        /* EOF a metà header: messaggio malformato */
        errno = EPROTO;
        return -1;
    }
    //controlo che il nome del file rispetti i limiti dichiarati
    if ((size_t)header.file_name_len > MR_MAX_MSG_LEN) {
        errno = EMSGSIZE;
        return -1;
    }
    //controlo che il contenuto della linea rispetti i limiti dichiarati
    if ( (size_t)header.line_length > MR_MAX_MSG_LEN) {
        errno =  EMSGSIZE;
        return -1;
    }
    
    /* inizializzo lo spazio per contenere il file_name */
    char *fname_buf = malloc(header.file_name_len + 1);
    if (fname_buf == NULL) {
        return -1;
    }
    // leggo header.file_name_len bytes dopo header, quindi leggo file_name e ne metto il contenuto in fname_buf
    //controllo di aver ricevuto tutto
    if (readn(fd, fname_buf, header.file_name_len) != (ssize_t)header.file_name_len) {
        if (errno == 0) { errno = EPROTO; } 
        free(fname_buf);
        return -1;
    }
    //aggiungo il terminatore a fname_buf
    fname_buf[header.file_name_len] = '\0';

    /* inizializzo lo spazio per contenere il contenuto della line */
    char *line_buf = malloc(header.line_length + 1);
    if (line_buf == NULL) {
        free(fname_buf);
        return -1;
    }
    // leggo header.line_length dopo header, quindi leggo line e ne metto il contenuto in line_buf
    //controllo di aver ricevuto tutto
    if (readn(fd, line_buf, header.line_length) != (ssize_t)header.line_length) {
        if (errno == 0) { errno = EPROTO; }
        /* [LOG] mr_log_write("?", "ERROR", "recv_line_msg: lettura line incompleta"); */
        free(fname_buf);
        free(line_buf);
        return -1;
    }
    //aggiungo il terminatore a line_buf
    line_buf[header.line_length] = '\0';


    /*assegno gli output*/
    *file_name = fname_buf;
    *file_name_len = header.file_name_len;
    *line_number = header.line_number;
    *line = line_buf;
    *line_len = header.line_length;

    return 0;
}

int send_pair_msg(int fd, const char *token, const void *value, size_t value_len) {
    //dichiaro una variabile header
    mr_pair_header_t header;
    //costruisco l'header con le informazioni utili per la de-serelizzazione da parte del reducer
    header.token_len = strlen(token);//posso farlo dato che il token(non logico) è una stringa 
    header.value_len = value_len;
    // mando l'header tramite written
    if (writen(fd, &header, sizeof(header)) != (ssize_t)sizeof(header)) {return -1;}
    // mando il contenuto di token tramite written
    if (writen(fd, token, strlen(token)) != (ssize_t)strlen(token)) {return -1;}
    // mando il contenuto di value tramite written
    if (writen(fd, value, value_len) != (ssize_t)value_len) {return -1;}
    return 0;
}

int recv_pair_msg(int fd, char **token, void **value, size_t *value_len, size_t *token_len) {
    //dichiaro una variabile header
    mr_pair_header_t header;
    // inf mi dice lo stato del mio
    ssize_t inf;
    // uso readn per riempire header con le informazioni che mi servono
    inf = readn(fd, &header, sizeof(header));
    //controllo di aver ricevuto tutto
    if (inf != (ssize_t)sizeof(header)) {
        if (inf == 0) {
            return 1;  
            /* EOF pulito, nessun messaggio */
        }
         /* EOF a metà header: messaggio malformato */
        errno = EPROTO;
        return -1;
    }

    //controlo che il token rispetti i limiti dichiarati
    if ((size_t)header.token_len > MR_MAX_MSG_LEN) {
        errno =  EMSGSIZE;
        return -1;
    }
    //controlo che value rispetti i limiti dichiarati
    if ((size_t)header.value_len > MR_MAX_MSG_LEN) {
        errno =  EMSGSIZE;
        return -1;
    }

    /* inizializzo lo spazio per contenere il token */
    char *token_buf = malloc(header.token_len + 1);
    if (token_buf == NULL) {return -1;}

    // leggo header.token_len bytes dopo header, quindi leggo token e ne metto il contenuto in token_buf
    //controllo di aver ricevuto tutto
    if (readn(fd, token_buf, header.token_len) != (ssize_t)header.token_len) {
        if (errno == 0) { errno = EPROTO; }
        free(token_buf);
        return -1;
    }
    //aggiungo il terminatore a token_buf
    token_buf[header.token_len] = '\0';

    /* inizializzo lo spazio per contenere il contenuto di value */
    /*value (dato opaco, NIENTE terminatore)  */
    char *value_buf = malloc(header.value_len);
    if (value_buf == NULL) {
        free(token_buf);
        return -1;
    }
    // leggo header.value_len bytes dopo header, quindi leggo value e ne metto il contenuto in value_buf
    //controllo di aver ricevuto tutto
    if (readn(fd, value_buf, header.value_len) != (ssize_t)header.value_len) {
        if (errno == 0) { errno = EPROTO; }
        free(token_buf);
        free(value_buf);
        return -1;
    }

    /*assegno gli output*/
    (*token) = token_buf;
    *token_len = header.token_len;
    (*value) = value_buf;
    *value_len = header.value_len;

    return 0;
}

int send_result_msg(int fd, const char *token, const void *result, size_t result_len) {
    //dichiaro una variabile header
    mr_result_header_t header;
    //costruisco l'header con le informazioni utili per la de-serelizzazione da parte del reducer
    header.token_len = strlen(token);//posso farlo dato che il token(non logico) è una stringa 
    header.result_len = result_len;
    
     // mando l'header tramite written
    if (writen(fd, &header, sizeof(header)) != (ssize_t)sizeof(header)) {return -1;}
    // mando il contenuto di token tramite written
    if (writen(fd, token, strlen(token)) != (ssize_t)strlen(token)) {return -1;}
    // mando il contenuto di result tramite written
    if (writen(fd, result, result_len) != (ssize_t)result_len) {return -1;}
    return 0;
}

int recv_result_msg(int fd, char **token, void **result, size_t *result_len,size_t *token_len) {
    //dichiaro una variabile header
    mr_result_header_t header;
    // inf mi dice lo stato del mio
    ssize_t inf;
    // uso readn per riempire header con le informazioni che mi servono
    inf = readn(fd, &header, sizeof(header));
    //controllo di aver ricevuto tutto
    if (inf != (ssize_t)sizeof(header)) {
        if (inf == 0) {
            return 1;  /* EOF pulito, nessun messaggio */
        }
        /* EOF a metà header: messaggio malformato */
        errno = EPROTO;
        return -1;
    }

    //controlo che il token rispetti i limiti dichiarati
    if ((size_t)header.token_len > MR_MAX_MSG_LEN) {
        errno =  EMSGSIZE;
        return -1;
    }
        //controlo che result rispetti i limiti dichiarati
    if ((size_t)header.result_len > MR_MAX_MSG_LEN) {
        errno =  EMSGSIZE;
        return -1;
    }
     /* inizializzo lo spazio per contenere il token */
    char *token_buf = malloc(header.token_len + 1);
    if (token_buf == NULL){return -1;}

    // leggo header.token_len bytes dopo header, quindi leggo token e ne metto il contenuto in token_buf
    //controllo di aver ricevuto tutto
    if (readn(fd, token_buf, header.token_len) != (ssize_t)header.token_len) {
        if (errno == 0) { errno = EPROTO; }
        free(token_buf);
        return -1;
    }
    //aggiungo il terminatore a token_buf
    token_buf[header.token_len] = '\0';

    /* inizializzo lo spazio per contenere il contenuto di result */
    /*result (dato opaco, NIENTE terminatore)  */
    char *result_buf = malloc(header.result_len);
    if (result_buf == NULL) {
        free(token_buf);
        return -1;
    }
    // leggo header.result_len bytes dopo header, quindi leggo result e ne metto il contenuto in result_buf
    //controllo di aver ricevuto tutto
    if (readn(fd, result_buf, header.result_len) != (ssize_t)header.result_len) {
        if (errno == 0) { errno = EPROTO; }
        free(token_buf);
        free(result_buf);
        return -1;
    }

    /*assegno gli output*/
    (*token) = token_buf;
    *token_len = strlen(token_buf);
    (*result) = result_buf;
    *result_len = header.result_len;

    return 0;
}