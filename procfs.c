#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>

#include "logger.h"
#include "procfs.h"
#include "util.h"

int pfs_hostname(char *proc_dir, char *hostname_buf, size_t buf_sz)
{
    int fd = open_path(proc_dir, "sys/kernel/hostname");
    if (fd == -1) {
        perror("open_path");
        return -1;
    }
    ssize_t read_sz = lineread(fd, hostname_buf, buf_sz);
    if (read_sz == -1) {
        return -1;
    }
    close(fd);


    hostname_buf[read_sz - 1] =  '\0';
    return 0;


    //return -1;
}

int pfs_kernel_version(char *proc_dir, char *version_buf, size_t buf_sz)
{
    int fd = open_path(proc_dir, "sys/kernel/osrelease");
    if (fd == -1) {
        perror("open_path");
        return -1;
    }
    ssize_t read_sz = lineread(fd, version_buf, buf_sz);
    if (read_sz == -1) {
        return -1;
    }
    close(fd);

    size_t dash_loc = strcspn(version_buf, "-");

    version_buf[dash_loc] = '\0';
    return 0;

    //return -1;
}

int pfs_cpu_model(char *proc_dir, char *model_buf, size_t buf_sz)
{
    int fd = open_path(proc_dir, "cpuinfo");
    if (fd == -1) {
        perror("open_path");
        return -1;
    }

    //model_buf is going to be 128 characters display.c
    //so we'll use that plus headroom here
    char line[256];

    ssize_t read_sz;

    while ((read_sz = lineread(fd, line, 256)) > 0 ){
        if (strncmp(line, "model name", 10) == 0) {
            LOG("Found it! -> %s\n", line);
            size_t model_loc = strcspn(line, ":");
            size_t newline_loc = strcspn(&line[model_loc], "\n");
            strcpy(model_buf, &line[model_loc + newline_loc]);
        }
        LOG("-> %s\n", line);
    
    }
    return -1;
}

int pfs_cpu_units(char *proc_dir)
{
    //parse data and count 
    char *cpuinfo = malloc(sizeof(char) * (strlen(proc_dir) + strlen("/cpuinfo") + 1));
    strcpy(cpuinfo, proc_dir);
    strcat(cpuinfo, "/cpuinfo");
    int cpuinfo_fd = open(cpuinfo, O_RDONLY);
    if (cpuinfo_fd == -1) {
        close(cpuinfo_fd);
        free(cpuinfo);
        return -1;
    }
    int cpu_unit_cnt = 0;
    while (1) {  
        //processor       
        //<info>
        //processor       
        // <info>           
        char line[128];
        memset(line, 0, 128);
        if (lineread(cpuinfo_fd, line, 127) <= 0) {
            break;
        }
        //Check this is line we need to count 
        if (strstr(line, "processor") != NULL) {
            cpu_unit_cnt++; //increment units being processed everytime processor line found
        }
        
    }
    close(cpuinfo_fd);
    free(cpuinfo);
    return cpu_unit_cnt;

    //return 0;
}


double pfs_uptime(char *proc_dir)
{
    char line[100] = {0};
    char *uptime;
    char *endline;
    
    char proc_uptime_array[100] = {0};
    strcpy(proc_uptime_array, proc_dir);

    strcat(proc_uptime_array, "/uptime");
    int fd = open(proc_uptime_array, O_RDONLY);
    
    if (fd == -1) {
        perror("fd");
        return -1;
    }

    ssize_t read = lineread(fd, line, sizeof(line) / sizeof(char));
    char *ptr_temp = line;

    if (read == -1) {
        perror("lineread");
        return -1;
    }

    uptime = next_token(&ptr_temp, " ");
    return strtod(uptime, &endline);


    //return 0.0;
}




int pfs_format_uptime(double time, char *uptime_buf)
{
    strcpy(uptime_buf, " ");
    int seconds_in_minute = 60;
    int seconds_in_hours = seconds_in_minute * 60;
    int seconds_in_day = seconds_in_hours * 24;
    int days = time / seconds_in_day;
    if (days > 0) {
        char line[100] = {0};
        sprintf(line, "%d days, ", days);
        strcat(uptime_buf, line);
    }
    int leftover_seconds = (int)time % seconds_in_day;
    int hours = leftover_seconds / seconds_in_hours;
    if (hours > 0) {
        char line[100] = {0};
        sprintf(line, "%d hours, ", hours);
        strcat(uptime_buf, line);
    }
    leftover_seconds = leftover_seconds % seconds_in_hours;
    int minutes = leftover_seconds / seconds_in_minute;
    if (days > 0) {
        char line[100] = {0};
        sprintf(line, "%d minutes, ", minutes);
        strcat(uptime_buf, line);
    }
    leftover_seconds = leftover_seconds % seconds_in_minute;
    int seconds = leftover_seconds;
    if (seconds_in_minute > 0) {
        char line[100] = {0};
        sprintf(line, "%d seconds ", seconds);
        strcat(uptime_buf, line);
    }


    return -1;
}

struct load_avg pfs_load_avg(char *proc_dir)
{
    //parse data after setting up file path
    //essentially loading in different averages
    struct load_avg lavg = { 0 }; //creates line to read data into
    char *loadavg = malloc(sizeof(char) * (strlen(proc_dir) + strlen("/loadavg") + 1)); 
    strcpy(loadavg, proc_dir);
    strcat(loadavg, "/loadavg");
    char line[64]; //line is going to be where data is read into
    memset(line, 0, 64); //set the memory to 0 to clear string
    int loadavg_fd = open(loadavg, O_RDONLY); //open load average file
    if (loadavg_fd == -1) { //check to see if file was opened properly
        close(loadavg_fd); //if it isn't then close the file and free memory and return average
        free(loadavg);
        return lavg;
    }
    //Read data 
    lineread(loadavg_fd, line, 63); //lineread is going to read line from file descriptor 
    char *line_ptr = line;
    for (size_t i = 0; i < 3; i++) { //for each number printed out 
        char *token = strsep(&line_ptr, " ");
        if (token == NULL) {
            break;
        }
        if (i == 0) {
            lavg.one = atof(token); //if first in array, set lavg.one to atof token and token is string seperate 
        } else if (i == 1) {
            lavg.five = atof(token); //if second in array, set lavg.five to atof token and token is string seperate 
        } else if (i == 2) {
            lavg.fifteen = atof(token); //if third in array, set lavg.fifteen to atof token and token is string seperate 
        }
        
    }
    close(loadavg_fd); //close file 
    free(loadavg); //free memory 
    return lavg; 

   //struct load_avg lavg = { 0 };
   //return lavg;
}


double pfs_cpu_usage(char *proc_dir, struct cpu_stats *prev, struct cpu_stats *curr)
{
    int fd = open_path(proc_dir, "/stat"); //open path and save to file descriptor
    char line[100] = {0};

    if (fd == -1) {
        perror("fd"); //check fd if it opens properly
        return -1;
    }

    ssize_t read = lineread(fd, line, sizeof(line) / sizeof(char)); //read line which is the first line printed out inside of /proc/stat
    char *ptr_temp = line; //set pointer to line 
                            //then pass pointer to line

    if (read == -1) {
        perror("lineread"); //if we can't read line error out
        return -1;
    }
    int counter = 0;
    char *curr_tok;
    //set current token to next token
    while ((curr_tok = next_token(&ptr_temp, " ")) != NULL) { //iterate through tokens, check first token //split words up to tokens
        //check if what were reading is cpu and if it is then continue 
        if (counter == 0) {
            counter++; //first thing read in is CPU, throw it away, increment counter and continue
            continue;
        }
        if (counter == 4) { //check if this is value for idle //checks idle wait time becuase need to calculate usage
            prev->idle = atoi(curr_tok); 
        }
        prev->total += atoi(curr_tok);//add every value that's not cpu, which is then added to total
        counter++;
    }
    //pause for sleep execution of program of 1 second
    //then find average of both to find usage
    sleep(1);

    fd = open_path(proc_dir, "/stat"); //open the file again 
    memset(line, 0, 100);

    if (fd == -1) {
        perror("fd");
        return -1;
    }

    
    read = lineread(fd, line, sizeof(line) / sizeof(char));
    ptr_temp = line;

    if (read == -1) {
        perror("lineread");
        return -1;
    }
    counter = 0;

    while ((curr_tok = next_token(&ptr_temp, " ")) != NULL) { //iterate through tokens, check first token //building curr instead of prev
        //check if what were reading is cpu and if it is then continue 
        if (counter == 0) {
            counter++;
            continue;
        }
        if (counter == 4) { //check if this is value for idle
            curr->idle = atoi(curr_tok);
        }
        curr->total += atoi(curr_tok);
        counter++;
    }
    return 1 - ((curr->idle - prev->idle) / (double) (curr->total - prev->total));
     //gives our cpu usage percentage 
     //curr and prev are both built
    


}

struct mem_stats pfs_mem_usage(char *proc_dir)
{
    // Try to parse data from there 
    //open proc file and meminfo
    struct mem_stats mstats = { 0 };
    char *meminfo_filename = malloc(sizeof(char) * (strlen(proc_dir) + strlen("/meminfo") + 1));
    strcpy(meminfo_filename, proc_dir);
    strcat(meminfo_filename, "/meminfo");
    int meminfo_fd = open(meminfo_filename, O_RDONLY); //check to see if file opens 
    if (meminfo_fd == -1) { //if it doesn't open then it closes file and frees memory
        close(meminfo_fd); 
        free(meminfo_filename);
        return mstats;
    }
    double mem_total = 0;
    double mem_available = 0;
    while (1) {
        //MemTotal
        //MemFree
        // MemAvailable
        char line[128]; //from mem info file descroptor,  were reading 128 in line array
        memset(line, 0, 128);

        if (lineread(meminfo_fd, line, 127) <= 0) { //reading in line
            break;
        }
        if (strstr(line, "MemTotal:") != NULL) { //memtotal is inside of line, if it is then set the line pointer to equal line and get next token(check for values)
            char *line_ptr = line;
            next_token(&line_ptr, " "); //memtotal is going to equal atoi of next token 
            mem_total = atof(next_token(&line_ptr, " "));
        }
        if (strstr(line, "MemAvailable:") != NULL) { //memavailable is inside of line, if it is then set the line pointer to equal line and get next token(check for values)
            char *buf_ptr = line; //got token, let it go to waste
            next_token(&buf_ptr, " "); //memavailable is goint to equal atoi of next token 
            mem_available = atof(next_token(&buf_ptr, " ")); //get real token, next token which is what we want
        }
    }
    //Store data 
    mstats.total = mem_total / 1048576.00; //add into total 
    mstats.used = (mem_total - mem_available) / 1048576.00;
    close(meminfo_fd); //close file descriptor
    free(meminfo_filename); // free data 
    return mstats;

    //struct mem_stats mstats = { 0 };
    //return mstats;
}



struct task_stats *pfs_create_tstats()
{
    struct task_stats *tstats = calloc(1, sizeof(struct task_stats)); //create pointer type and allocate space for a struct of tasks
    if (tstats == NULL) { //if line 363 causes error 
        free(tstats); //if space wasn't created correctly, free space
        return NULL;
    } else { //active task is being allocated as a pointer for struct that's going to have information for task
        tstats->active_tasks = calloc(1, sizeof(struct task_info)); //allocated space for task_info
    }
    return tstats; //return tstats after

    //return NULL;
}




void pfs_destroy_tstats(struct task_stats *tstats)
{
    free(tstats->active_tasks);//takes in ttasks that was created
    free(tstats); //free memoty of active tasks and tstats overall
}


int is_digit(char *str) //helpfer function that takes in char as string 
{
    int i;//helper function that takes in char as tring 
    for (i = 0; i < strlen(str); i++) {  //i checks length of string
        if (isdigit(str[i]) != 0) { //if it is digit, return 1 or return 0 if it isn't
            return 1;
        }
    }
    return 0;

}



int pfs_tasks(char *proc_dir, struct task_stats *tstats)
{
    DIR *dir;
    if ((dir = opendir(proc_dir)) == NULL) { //try to open directory and stored into DIR
        return 0;
    }
    struct dirent *file; //directory entry pointer
    int count = 0; //count starting at 0
    while ((file = readdir(dir)) != NULL) { //condition looping on is reading in directory
        // Try to list all folder inside the /proc. If this is process id -> digit directory 
        if (file->d_type == DT_DIR && is_digit(file->d_name)) { //read  one direcotry and store into file //checkint to see if we get to end then while loop condiiton will fail
            tstats->active_tasks = realloc(tstats->active_tasks, sizeof(struct task_info) * (count + 1)); //reallocate space for active-task
            tstats->active_tasks[count].pid = atoi(file->d_name); //create filename and open it
            char *status_filename = malloc(sizeof(char) * (strlen(proc_dir) + strlen("/") + strlen(file->d_name) + strlen("/status") + 1)); //allocate memory for status file
            strcpy(status_filename, proc_dir);
            strcat(status_filename, "/");
            strcat(status_filename, file->d_name);
            strcat(status_filename, "/status"); //build up status filename which is proc_dir
            int status_fd = open(status_filename, O_RDONLY); //open status filename and save that  file descriptor
            while (1) { //loop as long as soon as line is instantiate line, it will stop looping when we read each line up until last line andbreak when theres no next line
                char line[128];
                memset(line, 0, 128);
                if (lineread(status_fd, line, 127) <= 0) { //checking for everytime there's a next line
                    break;
                }
                if (strstr(line, "Name:") != NULL) {// check line if it has word Name //keys that line can be
                    char task_name[100]; //instatnate task name
                    int index = strcspn(line, ":");   //return index where that character is that its finding 
                    strcpy(task_name, &line[index + 2]); //use index to copy it over from lineindex + 2
                    strcpy(tstats->active_tasks[count].name, task_name); //copy task name that just got extracted to taskstruct
                    int length = strlen(tstats->active_tasks[count].name); //saves name under count.name
                    if (length >= 25) { //get lenght of name
                        tstats->active_tasks[count].name[25] = '\0'; //if greater than 25 put null terminating character at the end(signify end of string)
                    } else {
                        tstats->active_tasks[count].name[length + 1] = '\0'; //truncate the name right where the name is
                    } 
                }
                if (strstr(line, "Uid:") != NULL) {// check line if it has word Uid
                    char uid_str[100]; //create uid string
                    int index = strcspn(line, ":"); //find where : is
                    strcpy(uid_str, &line[index + 2]); //copy over uid without :
                    char *uid_ptr = uid_str; //create pointer to uid string 
                    tstats->active_tasks[count].uid = atoi(strsep(&uid_ptr, "\t")); //set uid equal to atoi of the uid pointer //set to uid for each of the tasks
                }                                                                   
                if (strstr(line, "State:") != NULL) { // check line if it has word State
                    char task_state[100]; //create task state
                    int index = strcspn(line, ":"); //check where : is
                    strcpy(task_state, &line[index + 2]); //copy over to state
                    if (strstr(task_state, "running") != NULL) { //check if state is running, copy over to state 'running' 
                        tstats->running++;
                        strcpy(tstats->active_tasks[count].state, "running");
                        count++; //incrment counter 
                    } else if (strstr(task_state, "disk sleep") != NULL) {
                        tstats->waiting++; //increments waiting and copy over disk sleep to current state
                        strcpy(tstats->active_tasks[count].state, "disk sleep");
                        count++;
                    } else if (strstr(task_state, "sleeping") != NULL || strstr(task_state, "idle")) { //if state is sleeping/idle incrment sleeping 
                        tstats->sleeping++; //increment sleeping
                    } else if (strstr(task_state, "tracing stop") != NULL) { //state is tracing stopped
                        tstats->stopped++; //increment stopped
                        strcpy(tstats->active_tasks[count].state, "tracing stop"); //copy over trace stopped as the state
                        count++;
                    } else if (strstr(task_state, "stopped") != NULL) { //if state is stopped incrment stopped
                        tstats->stopped++;
                        strcpy(tstats->active_tasks[count].state, "stopped"); // copy over stopped 
                        count++;
                    } else if (strstr(task_state, "zombie") != NULL) {
                        tstats->zombie++;
                        strcpy(tstats->active_tasks[count].state, "zombie");
                        count++;
                    }

                         
                    
                }


            }
            tstats->total++; //copy over stats to total and increment 
            close(status_fd);//close file descriptor
            free(status_filename);// free data

        }
    }
    closedir(dir);
    return 1;


    //return -1;
}



//open the direcotry
    //DIR *dr = opendir(proc_dir);

    //check if the path was a legal path --> if not return 0
    //if (dr == NULL) {
    //    return 0;
    //}

    //contents for the direcotry entry read into the variable de
    //struct dirent *de;

    //while (de = readdir(dr) != NULL) { // reading in direcotry conenct from the directory prointer dr into the idrecotry entry pointer until its end of the directory 
        //if(!strcmp("uptime", de->d_name)) {
            //break;
        //}

    //} //after the while loop finsihes then we know that de points to the file /prc/uptime

    //int fd = open(strcat(proc_dir, de->d_name)); //open file that we just found

    //struct stat * file_stats; //pointer to an object that has statistical info about a file 
    //call stat on file we want to open, and save the stats into file_stats variable. if this operation fails
    // then stat returns -1 and return 0 from this function to indicate error
    //if (stat(strcat(proc_dir, de->d_name), file_stats) == -1) return 0.0;

    //char file_info[file_stats->st_size];

    //read(fd, file_info, file_stats->st_size);

    //printf("The file content is: %s", file_info);
