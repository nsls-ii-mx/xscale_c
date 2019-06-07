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
"        arr[score]=sprintf(\"%-12s|%-20d|%-20.1f|%-20.1f|%-20.1f\", $1, $5, $7, $8, score); \n"
"} \n"
"END{ \n"
"    for(key in arr) { \n"
"        print(arr[key]) \n"
"    } \n"
"} ";


char lines[MAX_LINES][MAX_LINE_SIZE];
char template_lines[MAX_TEMPLATE_LINES][MAX_LINE_SIZE];
int index_of(char* str) {
        for(int i=0; i < MAX_LINES; i++){
                if(strcmp(str, lines[i]) == 0) return i;
        }
        return -1;
}

void write_inp_file(int bitstring, int length) {
        FILE* fp = fopen("XSCALE.INP", "w+");
        for(int i=0; i< MAX_TEMPLATE_LINES; i++) {
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
                                while(*ft == ' ') {
                                    ft++;
                                }
                                if(*ft == '/') {
                                    fprintf(fp, "%s\n", temp);
                                }
                                else {
                                    fprintf(fp, "%s../%s\n", inp, ft);
                                }
                        }
                }
        }

        fclose(fp);
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
int main(int argc, char* argv[]) {

        FILE *fp;

        if(argc < 2) {
                fprintf(stderr, "Error: no XSCALE.INP file given as argument\n");
                exit(EXIT_FAILURE);
        }

        char* filename = argv[argc-1];

        fp = fopen(filename, "r");
        if(fp != NULL) {
                int line_counter = 0;
                char temp[MAX_LINE_SIZE];
                int i =0;
                while(line_counter < MAX_LINES && fgets (temp, sizeof(temp), fp) != NULL) {
                        if(strstr(temp, "INPUT_FILE") != NULL) {
                                strcpy(lines[line_counter], temp);
                                lines[line_counter][strlen(lines[line_counter])-1] = 0;
                                line_counter++;
                        }
                        strcpy(template_lines[i], temp);
                        template_lines[i][strlen(template_lines[i])-1] = 0;
                        i++;
                }
                fclose(fp);

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
