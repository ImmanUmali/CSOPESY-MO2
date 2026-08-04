# MO2 - Multitasking OS
This project is an implementation of a Multitasking OS in accordance to the CSOPESY MCO2 Project for Term 3 AY 2025-2026. 

### S09 Group 8:
- UMALI, Immanuel
- LAZARO, Heisel Janine
- TRIA, Chynna Mae
  
### To run the program:
- Step 0: Clone the main branch of the repository to your Visual Studio environment.
- Step 1: Locate the `config.txt` file to modify the range of instruction length per process if needed.
- Step 2: Run using `Local Windows Debugger`.
- Step 3: Type `initialize` to start the processor configuration of the application.
- Step 4: Use commands as you see fit.

#### List of Commands available (Case Sensitive):
- **Console Commands**
1. `initialize` - initialize the processor configuration of the application
2. `exit` - exit the main console (terminates the console)
3. `screen -s` - adds one process
4. `screen -ls` - lists CPU utilization of all the processes
5. `screen -r` - reattach a running or finished process
6.  `screen -c` - creates a process that contains custom instructions
7. `scheduler-start` - continuously generates a batch of dummy processes for the CPU scheduler
8. `scheduler-stop` - stops generating dummy processes
9. `report-util` - generates CPU utilization report
10. `vmstat` - detailed system statistics of memory, CPU ticks, and page counts
11. `process-smi` - overview of memory usage and memory footprints
12. `exit`- exit the process console
