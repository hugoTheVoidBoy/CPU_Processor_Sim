**This program only runs on Linux/Unix**
This is the first project I did on Linux. Ubuntu and Linux VSCode were implemented by WSL to easily access Linux coding on my laptop.

This project simulates Unix OS when a system call (software interrupt) happens. Inputting bash commands or running test.bash in the prompt will compile the program to read the trace.txt, which involves forking new child processes or executing them. 

From here, all the CPU processes will be recorded into execution_prompt.txt and memory status is recorded in system_status.txt. It logs when the OS enters kernel mode, performs a context switch, calls the scheduler and runs CPU execution.

●	Vector_table.txt has a simulation of hex numbers in CPU memory location.
●	Interrupts.c compiles the file based on test bash commands, reads the Trace file and makes instructions using nodes and malloc, then pushes it into a list. This list is then read and print out a CPU process simulator and output the results manually into execution_prompt.txt

Bash
```
git clone https://github.com/hugoTheVoidBoy/CPU_Processor_Sim
cd CPU_Processor_Sim
gcc -o main interrupts.c
./main trace.txt
 
```
