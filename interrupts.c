#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include "interrupts.h" 

char lines[NUM_LINES][LINE_LENGTH]; // 2D array to store lines
///////////////////////////////////////////////////////////////////////////

void saveToFile(int time, node_t pcb) {
    // Open the file in write mode ("w" for write)
    FILE *file = fopen("system_status.txt", "a");
    
    // Check if the file was opened successfully
    if (file == NULL) {
        printf("Error: Could not open file system_status.txt for writing.\n");
        return;
    }
    
    // Write the data to the file
    fprintf(file, "! -----------------------------------------------------------!\n");
    fprintf(file, "Save Time: %d ms\n", time);
    fprintf(file, "+--------------------------------------------+\n");
    fprintf(file, "| PID |Program Name |Partition Number | size |\n");
    fprintf(file, "+--------------------------------------------+\n");
    
    node_t current = pcb;
    while (current != NULL) {
        pcb_t data = current->data;
        fprintf(file, "| %-3d | %-12s | %-15d | %-4d |\n", data->pid, data->program_name, data->partition_number, data->size);
        current = current->next;
    }

    fprintf(file, "+--------------------------------------------+\n");
    fprintf(file, "! -----------------------------------------------------------!\n");
    
    // Close the file
    fclose(file);
}


void read_vector_table(const char *filename) {
    FILE *file = fopen(filename, "r");

    int count = 0;

    while (count < NUM_LINES && fgets(lines[count], sizeof(lines[count]), file)) {
        // Remove newline character if present
        lines[count][strcspn(lines[count], "\n")] = '\0';
        count++;
    }

    fclose(file);
}


ins_t create_instruction(const char *name, int vector, int time) { //do not change
    ins_t temp = (ins_t) malloc(sizeof(struct instruction));
   
    temp->name = (char *)malloc(strlen(name) + 1); // +1 for the null terminator
    strcpy(temp->name, name); // Copy the instruction name

    // Initialize other contents
    temp->vector = vector;
    temp->time = time;
    return temp;
}


node_t create_node(void *data, enum DataType type){ //do not change
    // Initialize memory
    node_t temp = (node_t) malloc(sizeof(struct node));

    temp->next = NULL;
    temp->data = data;
    temp->type = type;

    return temp;
}

node_t push_node(node_t head, node_t temp){ //do not change
    // prev will be used to itterate through the list
    node_t prev;

    // If the list is empty then we return a list with only the new node
    if(head == NULL){
        head = temp;     
    } else {
        // Itterate through the list to add the new node at the end
        // The last node always points to NULL, so we get the next nodes until this happens
        prev = head;

        while(prev->next != NULL){
            prev = prev->next;
        }

        // Update the old final node to point to the new node
        prev->next = temp;
    }
    temp->next = NULL;
    return head;
}
void split_line(char *line, char *instruction, int *vec, int *time, char *program_name) { // do not change
    int MAXCHAR = 128;
    char *token;
    char temp_line[MAXCHAR];

    // Make a copy of the line to avoid modifying the original string
    strncpy(temp_line, line, MAXCHAR);
    temp_line[MAXCHAR - 1] = '\0'; // Null-terminate to prevent overflow

    // Get the first token (instruction)
    token = strtok(temp_line, ", ");
    if (token != NULL) {
        strcpy(instruction, token); // Copy the instruction name
    }
       /////////////////////////////////////////
    // Get the second token (vector or time)
    token = strtok(NULL, ", ");
    if (token != NULL) {
        if (strcmp(instruction, "EXEC") == 0) {
            // For EXEC, the second token is the program name
            strcpy(program_name, token);
            
            // Get the third token (time)
            token = strtok(NULL, ", ");
            if (token != NULL) {
                *time = atoi(token); // Convert to integer
            }
        }
            ///////////////////////////////////////
        else if (strcmp(instruction, "CPU") == 0 || strcmp(instruction, "FORK") == 0) {
            // For "CPU" the second argument is time
            *time = atoi(token);
        } else {
            // For other instructions, the second argument is vector
            *vec = atoi(token);
            // Get the third token (time)
            token = strtok(NULL, ", ");
            if (token != NULL) {
                *time = atoi(token); // Convert to integer
            } else {
                *time = -1; // Default value if not present
            }
        }
    } 
}


/* FUNCTION DESCRIPTION: read_file
*/
node_t read_trace(char *input_file, node_t partition_head, node_t files_head) { 
    int MAXCHAR = 128;
    char row[MAXCHAR];
    node_t new_list = NULL, node;
    ins_t instruction;

    char name[MAXCHAR], program_name[MAXCHAR], subprogram_name[MAXCHAR];
    int vector, time;

    FILE *f = fopen(input_file, "r");
    if (f == NULL) {
        printf("Unable to open file");
    }
    // Read the rows until you get to the end of the file
    while (fgets(row, MAXCHAR, f) != NULL) {
        node_t sub_list = NULL;
        split_line(row, name, &vector, &time, program_name);
        // Create instruction and node

        instruction = create_instruction(name, vector, time);
        if (instruction == NULL) {
            printf("Error: Failed to create instruction for %s\n", name);
            continue;
        }

        instruction->program_name = strdup(program_name); // Set program_name if needed

        node = create_node(instruction, INSTRUCTION);
        if (node == NULL) {
            printf("Error: Failed to create node for instruction %s\n", name);
            free(instruction->program_name);
            free(instruction);
            continue;
        }

        if(strcmp(instruction->name, "EXEC") == 0 && find_best_fit_partition(partition_head, find_file_size(files_head, instruction->program_name)) != NULL){
            strcpy(subprogram_name, instruction->program_name);
            strcat(subprogram_name, ".txt");
            sub_list = read_trace(subprogram_name, partition_head, files_head);
        }
        new_list = push_node(new_list, node);
        if (sub_list != NULL){
            new_list = push_node(new_list, sub_list);
        }
    }

    fclose(f); // Don't forget to close the file
    return new_list;
}


/* FUNCTION DESCRIPTION: print CPU execution steps
* Print 0, 32, CPU execution
        32, 1, switch to kernel mode
        33, 3, context saved
*/
void read_CPU_counter(int *current_time, int *CPU_counter, int CPU_time) {
    
    printf("%d, %d, CPU execution\n", *current_time, CPU_time);

    if (*CPU_counter % 2 == 0)
    {
        printf("%d, 1, switch to kernel mode\n", *current_time + CPU_time);
        printf("%d, 3, context saved\n", *current_time + 1 + CPU_time);
        
        *CPU_counter += 1 ; 
        *current_time += 4 + CPU_time;
    }

    else if (*CPU_counter % 2 == 1)
    {
        printf("%d, 1, check priority of interrupt\n", *current_time + CPU_time );
        printf("%d, 1, check if masked\n", *current_time + 1 + CPU_time);
        printf("%d, 1, switch to kernel mode\n", *current_time + 2 + CPU_time);
        printf("%d, 3, context saved\n", *current_time + 3 + CPU_time);
        
        *CPU_counter += 1;
        *current_time += 6 + CPU_time;
    }
}

/* FUNCTION DESCRIPTION: print CPU execution steps
* Print 0, 32, CPU execution
        32, 1, switch to kernel mode
        33, 3, context saved
*/
char* dec_to_hex(int vector_decimal) {
    char* hex = (char*)malloc(100 * sizeof(char));  // Dynamically allocate memory for the result
    int i = 0;
    
    vector_decimal = vector_decimal * 2;

    if (vector_decimal == 0) {
        hex[i++] = '0';  // Handle case for decimal 0
        hex[i] = '\0';
        return hex;
    }

    while (vector_decimal != 0) {
        int temp = vector_decimal % 16;
        
        // Convert integer to character for hexadecimal
        if (temp < 10) {
            hex[i] = temp + '0';  // Numbers 0-9
        } else {
            hex[i] = temp - 10 + 'A';  // Letters A-F
        }
        
        i++;
        vector_decimal /= 16;
    }

    hex[i] = '\0';  // Null-terminate the string

    // Reverse the string since the hexadecimal number is constructed backwards
    for (int j = 0; j < i / 2; j++) {
        char temp = hex[j];
        hex[j] = hex[i - j - 1];
        hex[i - j - 1] = temp;
    }

    return hex;
}

int generate_random_num(int x) {
    return (rand() % (x+1));  
}

/* FUNCTION DESCRIPTION: print sys call steps
* Print 36, 1, find vector 3 in memory position 0x0006
        37, 1, load address 0X059D into the PC
        38, 232, SYSCALL: run the ISR
        270, 105, transfer data
        375, 61, check for errors
        436, 1, IRET
    
*/
void read_sysc_counter(int* current_time, int* sysc_counter, ins_t curr) {
    int sysc_time = curr->time;
    int vector_decimal = curr->vector;
    int random_number_1 = generate_random_num(sysc_time/2);
    int random_number_2 = generate_random_num(sysc_time/2);
    int random_number_3 = sysc_time - random_number_1 - random_number_2;
    
    if (*sysc_counter % 2 == 0)
    {
        printf("%d, 1, find vector %d in memory position 0x%s\n", *current_time, vector_decimal, dec_to_hex(vector_decimal));
        printf("%d, 1, load address %s into the PC\n", *current_time + 1, lines[vector_decimal*2]); 
        printf("%d, %d, SYSCALL: run the ISR\n", *current_time + 2, random_number_1); 
        printf("%d, %d, transfer data\n", *current_time + random_number_1 +2, random_number_2); 
        printf("%d, %d, check for errors\n", *current_time + random_number_1 + random_number_2 +2, random_number_3); 
        printf("%d, 1, IRET\n", *current_time + sysc_time + 2); 
        *sysc_counter += 1;
        *current_time += sysc_time + 3;
    }

    else if (*sysc_counter % 2 == 1)
    {
        printf("%d, 1, find vector %d in memory position 0x%s\n", *current_time, vector_decimal, dec_to_hex(vector_decimal));
        printf("%d, 1, load address %s into the PC\n", *current_time + 1, lines[vector_decimal*2]); 
        printf("%d, %d, SYSCALL: run the ISR\n", *current_time + 2, random_number_1); 
        printf("%d, %d, transfer data to the display\n", *current_time + random_number_1 +2, random_number_2); 
        printf("%d, %d, check for errors\n", *current_time + random_number_1 + random_number_2 +2, random_number_3); 
        printf("%d, 1, IRET\n", *current_time + sysc_time +2); 
        *sysc_counter += 1;
        *current_time += sysc_time + 3;
    }
}

/* FUNCTION DESCRIPTION: print sys call steps
* Print 1771, 1, find vector 18 in memory position 0x0024
        1772, 1, load address 0X054B into the PC
        1773, 281, END_IO
    
*/
void read_end_io(int *current_time, ins_t curr) {
        int io_time = curr->time;
        int vector_decimal = curr->vector;

        printf("%d, 1, find vector %d in memory position 0x%s\n", *current_time, vector_decimal, dec_to_hex(vector_decimal));
        printf("%d, 1, load address %s into the PC\n", *current_time + 1, lines[vector_decimal*2]); 
        printf("%d, %d, END_IO\n", *current_time + 2, io_time); 
        printf("%d, 1, IRET\n", *current_time + io_time +2); 
        *current_time += io_time + 3;
    }



node_t initialize_partition(){
    // Corrected declaration of pointers to partition_t
    partition_t p1 = (partition_t) malloc(sizeof(struct partition));
    partition_t p2 = (partition_t) malloc(sizeof(struct partition));
    partition_t p3 = (partition_t) malloc(sizeof(struct partition));
    partition_t p4 = (partition_t) malloc(sizeof(struct partition));
    partition_t p5 = (partition_t) malloc(sizeof(struct partition));
    partition_t p6 = (partition_t) malloc(sizeof(struct partition));

    p1->number = 1; p1->size = 40; p1->program_name = strdup("free");
    p2->number = 2; p2->size = 25; p2->program_name = strdup("free");
    p3->number = 3; p3->size = 15; p3->program_name = strdup("free");
    p4->number = 4; p4->size = 10; p4->program_name = strdup("free");
    p5->number = 5; p5->size = 8;  p5->program_name = strdup("free");
    p6->number = 6; p6->size = 2;  p6->program_name = strdup("Occupied with PCB 0");

    node_t n1 = create_node(p1, PARTITION);
    node_t n2 = create_node(p2, PARTITION);
    node_t n3 = create_node(p3, PARTITION);
    node_t n4 = create_node(p4, PARTITION);
    node_t n5 = create_node(p5, PARTITION);
    node_t n6 = create_node(p6, PARTITION);

    n1->next = n2;
    n2->next = n3;
    n3->next = n4;
    n4->next = n5;
    n5->next = n6;

    return n1; // Return the head of the linked list
}

node_t initialize_PCB(){
        pcb_t p0 = (pcb_t) malloc(sizeof(struct pcb));
        p0->pid = 0; p0->program_name = strdup("init"); p0->partition_number = 6, p0->size = 1;
        node_t pcb0 = create_node(p0, PCB);
        saveToFile(0, pcb0);    
        return pcb0;
}
node_t new_PCB(node_t pcb0) {
    node_t curr = pcb0;
    pcb_t prev_data = pcb0->data;

    while(curr->next != NULL){
        curr = curr->next;
    }
    prev_data = curr->data;
    // Allocate and initialize the new PCB
    pcb_t p = (pcb_t) malloc(sizeof(struct pcb));
    p->pid = prev_data->pid + 1;
    p->program_name = strdup("init");
    p->partition_number = prev_data->partition_number;
    p->size = prev_data->size;
    node_t pcb = create_node(p, PCB);
    curr->next = pcb;
    // Return the head of the list (pcb0)
    return pcb0;
}

partition_t find_best_fit_partition(node_t head, int size) {
    partition_t best_fit = NULL;  // Pointer to store the best fit partition
    int min_diff = INT_MAX;        // Initialize minimum difference to maximum integer value

    // Iterate through the linked list
    node_t current = head;
    while (current != NULL) {
        // Check if the current node's data is of type PARTITION
        if (current->type == PARTITION) {
            partition_t part = (partition_t)current->data;

            // Check if the partition name contains "free" and the size is greater than the input size
            if (strstr(part->program_name, "free") != NULL && part->size >= size) {
                int diff = part->size - size; // Calculate the difference

                // Update best_fit if the current partition is a better fit
                if (diff < min_diff) {
                    min_diff = diff;    // Update the minimum difference
                    best_fit = part;    // Update the best fitting partition
                }
            }
        }
        else{
            printf("\nError at finding best fit partitioning\n");
        }
        current = current->next; // Move to the next node in the list
    }

    return best_fit; // Return the best fitting partition or NULL if not found
}

void read_fork(int *current_time, int fork_time, node_t pcb0) {
    
    int rand_fork_time = generate_random_num(10);
    if (*current_time == 0){
        printf("%d, 1, switch to kernel mode\n", *current_time);
        printf("%d, 3, context saved\n", *current_time + 1);
        *current_time += 4;
    }
    printf("%d, 1, find vector 2 in memory position 0x%s\n", *current_time, dec_to_hex(2));
    printf("%d, 1, load address %s into the PC\n", *current_time + 1, lines[2*2]);
    printf("%d, %d, FORK: copy parent PCB to child PCB\n", *current_time + 2 , rand_fork_time );
    // Call new_PCB to create a new PCB node and link it
    node_t new_pcb = new_PCB(pcb0);

    // Save the current list of PCBs to file
    

    printf("%d, %d, scheduler called \n", *current_time + rand_fork_time + 2, fork_time - rand_fork_time);
    printf("%d, 1, IRET \n", *current_time + fork_time + 2);     
    *current_time += fork_time + 3;
    saveToFile(*current_time-1, pcb0);
    }

void read_exec(int *current_time, node_t partition_head, node_t file_head, ins_t instruction, node_t pcb0) {
    char *program_name = instruction->program_name;
    int exec_time = instruction->time;
    int rand_exec_time = generate_random_num(exec_time/2);
    partition_t best_fit;
    
    
    printf("%d, 1, switch to kernel mode\n", *current_time);
    printf("%d, 3, context saved\n", *current_time + 1);   
    printf("%d, 1, find vector 3 in memory position 0x%s\n", *current_time + 4, dec_to_hex(3));
    printf("%d, 1, load address %s into the PC\n", *current_time + 5, lines[2*2]);
    printf("%d, %d, EXEC: load %s of size %dMb\n", *current_time + 6 , rand_exec_time , program_name, find_file_size(file_head, program_name));

    ////ADD partitioning function later////////////////////////////////////

    best_fit = find_best_fit_partition(partition_head, find_file_size(file_head, program_name));

    //////////////////////////////////////////////////////////////////////////
    if (best_fit != NULL){
        printf("%d, %d, found partition %d with %d Mb of space\n", *current_time + 6 + rand_exec_time, exec_time - 10 - rand_exec_time, best_fit->number, best_fit ->size);
        printf("%d, %d, partition %d marked as occupied\n", *current_time + 6 + exec_time - 10 , 6 , best_fit->number);
        printf("%d, %d, updating PCB with new information\n", *current_time + 2 + exec_time , 2);
        printf("%d, %d, scheduler called \n", *current_time + 4 + exec_time, 2);
        printf("%d, 1, IRET \n", *current_time + 6 + exec_time); 

        free(best_fit->program_name); // Free the existing name to avoid memory leaks
        char new_name[50];
        snprintf(new_name, sizeof(new_name), "Occupied by program %d", best_fit->number);
        best_fit->program_name = strdup(new_name); // Assign a new name
        
        *current_time += 7 + exec_time;
        
        node_t last_node = pcb0;
        while (last_node->next != NULL) {
            last_node = last_node->next;
        }

        // Cast last_node->data to struct pcb* to access pcb fields
        struct pcb *pcb_data = (struct pcb *)last_node->data;
        pcb_data->program_name = strdup(program_name);       // Copy program name
        pcb_data->partition_number = best_fit->number;       // Set partition number
        pcb_data->size = find_file_size(file_head, program_name);                       // Set program size
        saveToFile(*current_time-1, pcb0);
        }
    else{
        printf("%d, %d, No available partition\n", *current_time + 6 + rand_exec_time, abs(exec_time - rand_exec_time));
        printf("%d, %d, scheduler called \n", *current_time + 6 + rand_exec_time + abs(exec_time - rand_exec_time), 2);
        printf("%d, 1, IRET \n",  *current_time + 6 + rand_exec_time + abs(exec_time - rand_exec_time) +2 );     
        *current_time += 6 + rand_exec_time + abs(exec_time - rand_exec_time) +2; 
        }
    }

node_t initialize_file_list() {
    FILE *file = fopen("external_files.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open file external_files.txt for reading.\n");
        return NULL; // Return NULL if the file can't be opened
    }

    char line[128]; // Buffer to hold each line
    node_t head = NULL; // Head of the linked list
    node_t temp; // Temporary node to hold the newly created nodes

    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';

        // Create a new external_file struct and populate it
        external_file_t new_file = (external_file_t)malloc(sizeof(struct external_file));
        if (new_file == NULL) {
            printf("Error: Memory allocation failed.\n");
            fclose(file);
            return NULL; // Return NULL if memory allocation fails
        }

        // Assuming the line format is: <name> <size>
        // You might need to modify this based on your actual file format
        char name[20]; // Buffer to hold the name
        unsigned int size;
        if (sscanf(line, "%19[^,], %u", name, &size) == 2) {
            // Check if the last character is a comma and replace it
            size_t name_len = strlen(name);
            if (name_len > 0 && name[name_len - 1] == ',') {
                name[name_len - 1] = '\0'; // Remove the comma
            }
            strncpy(new_file->name, name, sizeof(new_file->name) - 1); // Copy the name safely
            new_file->name[sizeof(new_file->name) - 1] = '\0'; // Ensure null-termination
            new_file->size = size; // Set the size
        } else {
            free(new_file); // Free allocated memory if parsing fails
            continue; // Skip to the next line if the format is incorrect
        }

        // Create a new node for the linked list
        temp = create_node(new_file, EXTERNAL_FILE); // Use the create_node function
        head = push_node(head, temp); // Append the new node to the list
    }

    fclose(file); // Close the file
    return head; // Return the head of the linked list
}

int find_file_size(node_t file_head, const char *program_name) { 
    node_t current = file_head;
    external_file_t temp;

    while (current != NULL) {
        temp = current->data;
        if (strcmp(temp->name, program_name) == 0){
            return temp->size;
        }
        current = current->next;
    }
    return 200000000; //return file size too big
}

void print_instructions(node_t head, node_t pcb0, node_t partition_head, node_t files_head ) { 
    node_t current = head;
    ins_t p;
    int time = 0;
    int cpu_counter=0;
    int sysc_counter=0;
    char *name;

    while (current != NULL) {
        p = current->data;
        name = p->name;
        if (strcmp(name,"CPU") == 0){
            read_CPU_counter(&time,&cpu_counter, p->time);           
        }
        if (strcmp(name,"SYSCALL") == 0){
            read_sysc_counter(&time,&sysc_counter,p);
        }
        if (strcmp(name, "FORK") == 0){
            read_fork(&time, p->time, pcb0);
        }
        if (strcmp(name, "EXEC") == 0){
            read_exec(&time, partition_head, files_head, p, pcb0);
        }
        else if (strcmp(name, "END_IO") == 0){
            read_end_io(&time, p);
        }
        current = current->next;
    }
}

int main(int argc, char *argv[]) {
    // Check if a filename was provided
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    node_t partition_head = initialize_partition();
    node_t files_head = initialize_file_list();
    node_t pcb0 = initialize_PCB();
    

    // // Open the file
    char *filename = argv[1];

    read_vector_table("vector_table.txt");

   
    // //Parse file
    node_t instruction_list = read_trace(filename, partition_head, files_head);
    print_instructions(instruction_list, pcb0, partition_head, files_head);
    return 0;
}
