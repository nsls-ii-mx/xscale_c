#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <float.h>
#include <sys/wait.h>
#define MAX_LINES 32
#define MAX_LINE_SIZE 512
#define MAX_TEMPLATE_LINES 2048

char* awk_program = "BEGIN{ PROCINFO[\"sorted_in\"]=\"@ind_num_desc\" \n"
        "FS=\" |/\" \n"
        "printf(\"%-12s|%-20s|%-20s|%-20s|%-10s\\n\", \"Run\", \"Unique Reflections\", \"Completeness\", \"R Value\", \"Score\") \n"
        "printf(\"==============================================================================================\\n\") \n"
        "} \n"
"/run[0-9]/{ \n"
"        score=$7+(100-$8); \n"
"        printf(\"%-12s|%-20d|%-20.1f|%-20.1f|%-20.1f\\n\", $1, $5, $7, $8, score) | \"sort -r -k 5\"; \n"
"} ";


char lines[MAX_LINES][MAX_LINE_SIZE];
char template_lines[MAX_TEMPLATE_LINES][MAX_LINE_SIZE];

int lines_used;

typedef struct factoradic_digit DIGIT;
typedef struct factoradic_number PERMUTATION;
struct factoradic_digit {
    int value;
    DIGIT* next;
    DIGIT* prev;
};
struct factoradic_number {
    DIGIT* first;
    DIGIT* last;
};

void print_help_message() {
    printf("xscale_p runs xscale on all permutations or combinations of input data sets and produces a sorted table with completeness, r value, and number of unique reflections from each run.\n");
    printf("Usage: xscale_p [-c | -p | -b] XSCALE.INP\n");
    printf("Options:\n");
    printf("    -h, --help:         Print this message\n");
    printf("    -c, --combinations: Run xscale on all combinations of the input files\n");
    printf("    -p, --permutations: Run xscale on all permutations of the input files\n");
    printf("    -b, --both:         Run xscale on all combinations, and on those permutations that swap the primary (reference) input file\n");
}
int index_of(char* str) {
        for(int i=0; i < MAX_LINES; i++){
                if(strcmp(str, lines[i]) == 0) return i;
        }
        return -1;
}

long factorial(int n) {
    if(n<0) return -1L;
    if(n==0 || n==1) return 1L;
    return n*factorial(n-1);
}

//increment mod n! (preserve length of permutation)
void increment(PERMUTATION* perm) {
    DIGIT* current_digit = perm->last;
    int counter = 0;
    while(current_digit != NULL) {
        if(current_digit->value < counter) {
            current_digit->value++;
            return;
        }
        current_digit->value = 0;
        current_digit = current_digit->prev;
        counter++;
    }
}

//this iterates over the 2-cycles containing the first element of the permutation (for mode 2)
void increment_first_digit(PERMUTATION* perm) {
    perm->first->value++;
}

//return the size of a permutation (i.e. n such that perm \in S_n)
int get_length(PERMUTATION* perm) {
    int len = 0;
    DIGIT* current_digit = perm->first;
    while(current_digit != NULL) {
        current_digit = current_digit->next;
        len++;
    }
    return len;
}

//initialize the identity permutation of given length
PERMUTATION* init_permutation(int length) {
    PERMUTATION* perm = (PERMUTATION*) (malloc(sizeof(PERMUTATION)));
    for(int i=0; i<length; i++) {
        DIGIT* current_digit = (DIGIT*) (malloc(sizeof(DIGIT)));
        current_digit->value = 0;
        current_digit->next = NULL;
        if(i==0) {
            perm->first = current_digit;
            perm->last = current_digit;
            current_digit->prev = NULL;
        }
        else {
            perm->last->next = current_digit;
            current_digit->prev = perm->last;
            perm->last = current_digit;
        }
    }
    return perm;
}

//free memory allocated by the permutation
void free_permutation(PERMUTATION* perm) {
    DIGIT* current_digit = perm->last->prev;
    while(current_digit != NULL) {
        free(current_digit->next);
        current_digit = current_digit->prev;
    }
    free(perm->first);
    free(perm);
}

void remove_number(int* arr, int len, int index) {
    for(int i=index; i<len-1; i++) {
        *(arr+i) = *(arr+i+1);
    }
}

//return a permuted array, eg) [3,1,4,2,0] corresponding to permutation 31210
int* get_permuted_array(PERMUTATION* permutation) {
    int len = get_length(permutation);
    int* arr = (int*) calloc(len, sizeof(int));
    for(int i=0; i<len; i++) {
        *(arr+i) = i;
    }
    int* arr_perm = (int*) calloc(len, sizeof(int));
    DIGIT* current_digit = permutation->first;
    int j=0;
    while(current_digit != NULL) {
        *(arr_perm+j) = *(arr+current_digit->value);
        remove_number(arr, len, current_digit->value);
        current_digit = current_digit->next;
        j++;
    }
    free(arr);
    return arr_perm;
}

int index_of_bit(int bitstring, int n) {
    for(int i=0; i<32; i++) {
        if(n==0) return i;
        int mask = 1<<i;
        if((mask & bitstring) != 0) n--;
    }
    return -1;
}

void write_inp_file_b(int bitstring, int length, PERMUTATION* perm) {
    fprintf(stderr, "bitstring = %d\n", bitstring);
    FILE* fp = fopen("XSCALE.INP", "w+");
    int* perm_array = get_permuted_array(perm);
    int counter = 0;
    for(int i=0; i<lines_used; i++) {
        char* temp = template_lines[i];
        int index = index_of(temp);
        if(index == -1) fprintf(fp, "%s\n", temp);
        else {
            char* inp = "INPUT_FILE=";
            int mask = 1<<index;
            int current_bit_present = (bitstring & mask) >> index;
            if(current_bit_present == 0) continue;
            char* line = lines[index_of_bit(bitstring, *(perm_array+counter))];
            char* filename = line+11;
            char* ft = filename;
            while(*ft == ' ' || *ft == '*') {
                ft++;
            }
            if(*ft == '/') { //absolute path
                fprintf(fp, "%s\n", line);
            }
            else { //relative path
                fprintf(fp, "%s../%s\n", inp, ft);
            }
            counter++;
        }
    }
    free(perm_array);
    fclose(fp);
}

void write_inp_file_p(PERMUTATION* perm) {
    FILE* fp = fopen("XSCALE.INP", "w+");
    int* perm_array = get_permuted_array(perm);
    for(int i=0; i<lines_used; i++) {
        char* temp = template_lines[i];
        int index = index_of(temp);
        if(index == -1) fprintf(fp, "%s\n", temp);
        else {
            char* inp = "INPUT_FILE=";
            char* line = lines[*(perm_array+index)];
            char* filename = line+11;
            char* ft = filename;
            while(*ft == ' ' || *ft == '*') {
                ft++;
            }
            if(*ft == '/') { //absolute path
                fprintf(fp, "%s\n", line);
            }
            else { //relative path
                fprintf(fp, "%s../%s\n", inp, ft);
            }
        }
    }
    free(perm_array);
    fclose(fp);
}

void write_inp_file(int bitstring, int length) {
        FILE* fp = fopen("XSCALE.INP", "w+");
        for(int i=0; i<lines_used; i++) {
                char* temp = template_lines[i];
                int index = index_of(temp);
                if(index == -1) fprintf(fp, "%s\n", temp);
                else {
                        int mask = 1 << index;
                        int current_bit_present = (bitstring & mask) >> index;
                        if(current_bit_present) {
                                char* inp = "INPUT_FILE= ";
                                char* filename = temp + 11;
                                char* ft = filename;
                                while(*ft == ' ' || *ft == '*') {
                                    ft++;
                                }
                                if(*ft == '/') { //absolute path
                                    fprintf(fp, "%s\n", temp);
                                }
                                else { //relative path
                                    fprintf(fp, "%s../%s\n", inp, ft);
                                }
                        }
                }
        }

        fclose(fp);
}

char* get_dir_name_b(int bitstring, int length, PERMUTATION* perm) {
    int* arr = get_permuted_array(perm);
    char* str = (char*) calloc(length, sizeof(char));
    for(int i=0; i<length; i++) {
        *(str+i) = (char) (index_of_bit(bitstring, *(arr+i)) + '1');
    }
    free(arr);
    char* buffer = (char*) calloc(length+3, sizeof(char));
    strcpy(buffer, "run");
    strcat(buffer, str);
    free(str);
    return buffer;
}
char* get_dir_name_p(PERMUTATION* perm) {
    int* arr = get_permuted_array(perm);
    int len = get_length(perm);
    char* str = (char*) calloc(len, sizeof(char));
    for(int i=0; i< len; i++) {
        *(str+i) = (char) (*(arr+i) + '1');
    }
    free(arr);
    char* buffer = (char*) calloc(len+3, sizeof(char));
    strcpy(buffer, "run");
    strcat(buffer, str);
    free(str);
    return buffer;
}

char* get_dir_name(int bitstring, int length) {
        char* dir_name = malloc(sizeof(char) * 100);
        strcpy(dir_name, "run");
        for(int i=0; i < length; i++) {
                int mask = 1 << i;
                int current_bit_present = (bitstring & mask) >> i;
                if(current_bit_present) {
                        char temp[3];
                        sprintf(temp, "%d", i+1);
                        strcat(dir_name, temp);
                }
        }
        return dir_name;
}

void run_xscale_b(int bitstring, int length, PERMUTATION* perm) {
    char* dir_name = get_dir_name_b(bitstring, length, perm);
    fprintf(stderr, "dirname = %s\n", dir_name);
    mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO);
    chdir(dir_name);
    write_inp_file_b(bitstring, length, perm);
    int status = system("xscale");
    if(WEXITSTATUS(status) != 0) {
        fprintf(stderr, "%s\n", "Error running XSCALE!");
        exit(1);
    }
    free(dir_name);
    chdir("..");
}

void run_xscale_p(PERMUTATION* perm) {
    char* dir_name = get_dir_name_p(perm);
    mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO);
    chdir(dir_name);
    write_inp_file_p(perm);
    int status = system("xscale");
    if(WEXITSTATUS(status) != 0) {
        fprintf(stderr, "%s\n", "Error running XSCALE!");
        exit(1);
    }
    free(dir_name);
    chdir("..");
}

void run_xscale(int bitstring, int length) {
        char* dir_name = get_dir_name(bitstring, length);
        mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO);
        chdir(dir_name);
        write_inp_file(bitstring, length);
        int status = system("xscale");
	    if(WEXITSTATUS(status) != 0) {
		        fprintf(stderr, "%s\n", "Error running XSCALE!");
		        exit(1);
	    }
        chdir("..");
}

int test_power_of_two(int bitstring) {
        for(int i=0; i<32; i++){
                int mask = 1 << i;
                if((bitstring & mask) == bitstring) return 1;
        }
        return 0;
}

//return the length of a combination, i.e. size of the subset it represents
int combination_length(int bitstring) {
    int counter = 0;
    for(int i=0; i<32; i++) {
        int mask = 1<<i;
        if((bitstring & mask) != 0) counter++;
    }
    return counter;
}

int main(int argc, char* argv[]) {

        FILE *fp;

        int mode = 0;
        if(argc < 2) {
                fprintf(stderr, "Error: no XSCALE.INP file given as argument\n");
                exit(EXIT_FAILURE);
        }
        if(argc >= 2) {
            if(strcmp("-h", argv[1]) == 0 || strcmp("--help", argv[1]) == 0) {
                print_help_message();
                exit(0);
            }
            if(strcmp("-b", argv[1]) == 0 || strcmp("--both", argv[1]) == 0) {
                mode = 2;
            }
            if(strcmp("-p", argv[1]) == 0 || strcmp("--permutations", argv[1]) == 0) {
                mode = 1;
            }
            if(strcmp("-c", argv[1]) == 0 || strcmp("--combinations", argv[1]) == 0) {
                mode = 0;
            }
        }

        char* filename = argv[argc-1];

        fp = fopen(filename, "r");
        if(fp != NULL) {
                int line_counter = 0;
                char temp[MAX_LINE_SIZE];
                int i =0;
                while(line_counter < MAX_LINES && fgets (temp, sizeof(temp), fp) != NULL) {
                        if(strstr(temp, "INPUT_FILE") != NULL && (strstr(temp, "!") == NULL || strstr(temp, "!") > strstr(temp, "INPUT_FILE")) ) {
                                strcpy(lines[line_counter], temp);
                                lines[line_counter][strlen(lines[line_counter])-1] = 0;
                                line_counter++;
                        }
                        strcpy(template_lines[i], temp);
                        template_lines[i][strlen(template_lines[i])-1] = 0;
                        i++;
                }
                fclose(fp);
                lines_used = i;

                if(mode == 2) { //both combinations and permutations, but only permute to change the first (reference) file
                    printf("Running 'both' mode \n");
                    printf("------------------------\n");
                    int number_possibilities = 1 << line_counter;
                    for(int bitstring = 1; bitstring < number_possibilities; bitstring++) {
                            if(test_power_of_two(bitstring) == 1) continue;
                            if(fork() == 0) {
                                int l = combination_length(bitstring);
                                PERMUTATION* perm = init_permutation(l);
                                for(int i=0; i<l; i++) {
                                    run_xscale_b(bitstring, l, perm);
                                    increment_first_digit(perm);
                                }
                                free_permutation(perm);
                                exit(0);
                            }
                    }
                    for(int bitstring = 1; bitstring < number_possibilities; bitstring++) {
                        if(test_power_of_two(bitstring) == 1) continue;
                        wait(NULL);
                    }
                }
                else if(mode == 1) { //permutations
                    printf("Running permutation mode\n");
                    printf("------------------------\n");
                    PERMUTATION* perm = init_permutation(line_counter);
                    for(int i=0; i< factorial(line_counter); i++) {
                        if(fork() == 0) {
                            run_xscale_p(perm);
                            exit(0);
                        }
                        increment(perm);
                    }
                    for(int i=0; i<factorial(line_counter); i++) {
                        wait(NULL);
                    }
                    free(perm);
                }

                else if (mode == 0) { //combinations
                    printf("Running combination mode\n");
                    printf("------------------------\n");
                    int number_possibilities = 1 << line_counter;
                    for(int bitstring = 1; bitstring < number_possibilities; bitstring++) {
                            if(test_power_of_two(bitstring) == 1) continue;
                            if(fork() == 0) {
                                run_xscale(bitstring, line_counter);
                                exit(0);
                            }
                    }
                    for(int bitstring = 1; bitstring < number_possibilities; bitstring++) {
                        if(test_power_of_two(bitstring) == 1) continue;
                        wait(NULL);
                    }
                }
        }
        else {
                perror(filename);
        }

	system("grep -rnw --include=XSCALE.LP -m 1 'total' | grep '\\*' | tee temp_table.txt");
	system("sed -i -r -e 's/  +/ /g' temp_table.txt");

    char awk_cmd[1000];

    sprintf(awk_cmd, "awk '%s' temp_table.txt | tee sorted_table.txt", awk_program);
    system(awk_cmd);
    remove("temp_table.txt");

        return 0;
}
