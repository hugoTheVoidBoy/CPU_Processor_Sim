
#ifndef INTERRUPTS_H

#define INTERRUPTS_H

#include <stdbool.h>

#define NUM_LINES 52 // double number of lines in file (26)

#define LINE_LENGTH 7 // 6 characters + 1 for the null terminator

// Define the types of structs that can be stored in node

enum DataType {

    PARTITION,

    PCB,

    INSTRUCTION,

    EXTERNAL_FILE

};

struct external_file{

    char name[20];

    unsigned int size;

};

struct partition {

    unsigned int number;

    unsigned int size;

    char *program_name;

};

struct pcb {

    int pid;
    char *program_name;
    int partition_number;
    int size;

    //char *i_o_info;

};

struct node {
    void *data; // data can point to ANYTHING
    enum DataType type;
    struct node *next;
};
struct instruction {

    char *name;

    int vector;

    int time;

    char *program_name;

};

typedef struct external_file *external_file_t;
typedef struct partition *partition_t;
typedef struct pcb *pcb_t;
typedef struct instruction *ins_t;
typedef struct node *node_t;

// Function declarations
node_t initialize_partition();
node_t initialize_file_list();
void read_vector_table(const char *filename);
ins_t create_instruction(const char *name, int vector, int time);
node_t create_node(void *data, enum DataType type);
node_t push_node(node_t head, node_t temp);
void split_line(char *line, char *instruction, int *vec, int *time, char *program_name) ;
node_t read_trace(char *input_file, node_t partition_head, node_t files_head);
void read_CPU_counter(int *current_time, int *CPU_counter, int CPU_time);
char* dec_to_hex(int vector_decimal);
int generate_random_num(int x);
void read_sysc_counter(int *current_time, int *sysc_counter, ins_t curr);
void read_end_io(int *current_time, ins_t curr);
void print_instructions(node_t head, node_t pcb0, node_t partition_head, node_t files_head );
void saveToFile(int time, node_t pcb);
node_t initialize_PCB();
node_t new_PCB(node_t pcb0);
partition_t find_best_fit_partition(node_t head, int size);
void read_fork(int *current_time, int fork_time, node_t pcb0);
void read_exec(int *current_time, node_t partition_head, node_t file_head, ins_t instruction, node_t pcb0);
node_t initialize_file_list();
int find_file_size(node_t file_head, const char *program_name);
int main(int argc, char *argv[]);

#endif
