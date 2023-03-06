#include <math.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util.h"
#include "logger.h"

void draw_percbar(char *buf, double frac) { //essentially finding the percentage //using fac for number to calcute percentage
    double perc = frac * 100;
    if (isnan(perc) || perc <= 0.0) {
        perc  = 0.0; //set to float 0.0 if number isn't number 
    }
    if (perc >= 100.0) {
        perc = 100.0;
    }
    double inc = perc / 5; //set to increments of 5
    for (size_t i = 0; i < 28; i++) {
        if (i == 0) {
            buf[i] = '[';
        } else if (i == 21) {
            buf[i] = ']';
        } else if (i == 22) {
            buf[i] = ' ';
        } else {
            if (inc >= 0.89) {
                buf[i] = '#';
                inc--;
            } else {
                buf[i] = '-';
            }
        }
    }
    sprintf(&buf[23], "%.1f", perc);
    strcat(buf, "%");

}


void uid_to_uname(char *name_buf, uid_t uid) //gets user from task uid
{
    /*Try to read data from /etc/passwd */
    char buffer_reader[128] = { 0 }; //create buffer reader with 128 space
    char user_name[32] = { 0 }; //create username with 32 space
    int passwd_fd = open("/etc/passwd", O_RDONLY);
    sprintf(name_buf, "%x", uid); //if no user, put uid in buffer 
    while (1) {
        if (lineread(passwd_fd, buffer_reader, 127) <= 0) {
            return;
        }
        char *uid_ptr = buffer_reader; //store whatever read to buffer read to uid pointer
        strcpy(user_name, strsep(&uid_ptr, ":")); //streo user name and tokenize number
        strsep(&uid_ptr, ":");
        int found_uid = atol(uid_ptr);
        if (uid == found_uid) { //check for uid matches
            strcpy(name_buf, user_name); 
            name_buf[15] = '\0';
        }
    }
    close(passwd_fd);
    //strcpy(name_buf, "(UNKNOWN)");
}



int open_path(char *proc_dir, char *path) 
{
    if (proc_dir == NULL || path == NULL)  {
        errno = EINVAL;
        return -1;
    }

    size_t str_size = strlen(proc_dir) +strlen(path) + 2;
    char *full_path = malloc(str_size * sizeof(char));
    if (full_path == NULL) {
        return -1;
    }
    snprintf(full_path, str_size, "%s/%s", proc_dir, path);
    LOG("Openeing path: %s\n", full_path);

    int fd = open(full_path, O_RDONLY);
    free(full_path);
    
    return fd;
}


/**
 * Retrieves the next token from a string.
 *
 * Parameters:
 * - str_ptr: maintains context in the string, i.e., where the next token in the
 *   string will be. If the function returns token N, then str_ptr will be
 *   updated to point to token N+1. To initialize, declare a char * that points
 *   to the string being tokenized. The pointer will be updated after each
 *   successive call to next_token.
 *
 * - delim: the set of characters to use as delimiters
 *
 * Returns: char pointer to the next token in the string.
 */
char *next_token(char **str_ptr, const char *delim)
{
    if (*str_ptr == NULL) {
        return NULL;
    }

    size_t tok_start = strspn(*str_ptr, delim);
    size_t tok_end = strcspn(*str_ptr + tok_start, delim);

    /* Zero length token. We must be finished. */
    if (tok_end  == 0) {
        *str_ptr = NULL;
        return NULL;
    }

    /* Take note of the start of the current token. We'll return it later. */
    char *current_ptr = *str_ptr + tok_start;

    /* Shift pointer forward (to the end of the current token) */
    *str_ptr += tok_start + tok_end;

    if (**str_ptr == '\0') {
        /* If the end of the current token is also the end of the string, we
         * must be at the last token. */
        *str_ptr = NULL;
    } else {
        /* Replace the matching delimiter with a NUL character to terminate the
         * token string. */
        **str_ptr = '\0';

        /* Shift forward one character over the newly-placed NUL so that
         * next_pointer now points at the first character of the next token. */
        (*str_ptr)++;
    }

    return current_ptr;
}

int example(void)
{
    char str[] = "     This is a really great string, is it not?!";

    int tokens = 0;
    char *next_tok = str;
    char *curr_tok;
    /* Tokenize. Note that ' ,?!' will all be removed. */
    while ((curr_tok = next_token(&next_tok, " ,?!")) != NULL) {
        printf("Token %02d: '%s'\n", tokens++, curr_tok);
    }

    return 0;
}

     


ssize_t lineread(int fd, char *buf,  size_t sz) 
{
    size_t counter = 0;
    while (counter < sz) 
    {
        char c;
        ssize_t read_sz = read(fd, &c, 1);

        if(read_sz == 0) 
        {
            return 0;
        } else if (read_sz == -1) {
            return -1;
        } else {
            buf[counter] = c;
            counter++;

            if (c == '\n') {
                return counter;
            }
        }
    }
    return counter;
}