// Justine Mangaliman - 30164741 - TUT T02
// Abdullah Nawaz Sheikh - 30180348 - TUT T02

// IMPORTANT
// IMPORTANT

// Before running the code, please make sure to change these two constants:

//  MAX_INPUT_SIZE: how many processes there are in the input file (<= 1000)
//  PROCESS_COUNT: how many process #s (pids) will appear in the input file (<= 50)

// They are located in lines 9 and 12.

// Thank you !

// IMPORTANT
// IMPORTANT

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT_SIZE 1000 // how many processes there are in the .csv file you're using. do not count empty lines at eof.
#define ORIGINAL_DUPLICATES_SIZE 3
#define ORIGINAL_READY_QUEUE_SIZE 1
#define PROCESS_COUNT 50

struct Process {
  int pid;
  int arrival_time;
  int time_until_first_response;
  int burst_length;
  float priority;
};

// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
// Declare a heap structure
struct Heap {
	struct Process* arr;
	int size;
	int capacity;
};

typedef struct Heap heap; // modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/

char *cmd_args_list[] = {
  "fcfs",
  "sjf",
  "priority",
  "rr",
  "srt"
};

struct Process all_processes[MAX_INPUT_SIZE]; // array to store processes
struct Process processes_output[PROCESS_COUNT];

int processes_start_times[PROCESS_COUNT];
int processes_finish_times[PROCESS_COUNT];
int processes_wait_times[PROCESS_COUNT];
int processes_response_times[PROCESS_COUNT];
float predicted_burst_times[PROCESS_COUNT]; // To store the predicted burst times for each process


void first_come_first_served();
void shortest_job_first();
void priority_scheduling();
void round_robin();
void shortest_remaining_time(float alpha);
void write_results_to_file(const char *filename, int avg_turnaround_time, int avg_wait_time, int avg_response_time);
void print_table(int avg_turnaround_time, int avg_wait_time, int avg_response_time);
void print_table_rrsrt(int avg_turnaround_time, int avg_wait_time, int avg_response_time);
void to_lowercase(char *str); // "FCfS" -> "fcfs", "Priority" -> "priority"... etc.
int compare_arrival_time(const void* a, const void* b);
void csv_inputs_to_processes_array();

// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
heap* createHeap(int capacity);
void insertHelper(heap* h, int index, char* sort_by);
void heapify(heap* h, int index, char* sort_by);
struct Process extractMin(heap* h, char* sort_by);
void insert(heap* h, struct Process data, char* sort_by);
void printHeap(heap* h);

int main(int argc, char *argv[]) {
  if (argc > 1) {
    to_lowercase(argv[1]);

    csv_inputs_to_processes_array();

    for (int i = 0; i < PROCESS_COUNT; i++) {
      struct Process process;
      process.arrival_time = -1;
      process.pid = i + 1;
      process.burst_length = 0;
      process.time_until_first_response = 0;
      processes_output[i] = process;
    }

    if (strcmp(argv[1], cmd_args_list[3]) == 0 || strcmp(argv[1], cmd_args_list[4]) == 0) {
      for (int i = 0; i < PROCESS_COUNT; i++) {
        processes_start_times[i] = 0;
        processes_wait_times[i] = 0;
        processes_finish_times[i] = 0;
        processes_response_times[i] = -1;
      }

      for (int i = 0; i < PROCESS_COUNT; i++) {
        predicted_burst_times[i] = 10.0; // Initial predicted burst time τ0 = 10
      }
    } else {
      for (int i = 0; i < PROCESS_COUNT; i++) {
        processes_start_times[i] = -1;
        processes_wait_times[i] = -1;
        processes_response_times[i] = -1;
      }
    }

    if (strcmp(argv[1], cmd_args_list[0]) == 0) {
      first_come_first_served();
    } else if (strcmp(argv[1], cmd_args_list[1]) == 0) {
      shortest_job_first();
    } else if (strcmp(argv[1], cmd_args_list[2]) == 0) {
      priority_scheduling();
    } else if (strcmp(argv[1], cmd_args_list[3]) == 0) {
      int time_quantum = (argc > 2) ? atoi(argv[2]) : 1; // Default quantum is 1 if not provided
      round_robin(time_quantum);;
    } else if (strcmp(argv[1], cmd_args_list[4]) == 0) {
      float alpha = (argc > 2) ? atof(argv[2]) : 1.0; // Default α = 0.5 if not provided
      shortest_remaining_time(alpha);
    } else {
      printf("incorrect input please rerun program\n");
    }
  }
  return 0;
}

void first_come_first_served() {
  int duplicates_size = ORIGINAL_DUPLICATES_SIZE;
  struct Process* duplicates = (struct Process*)malloc(ORIGINAL_DUPLICATES_SIZE * sizeof(struct Process));
  int duplicates_index = 0;
  int jump; // if 3 consecutive processes a b c have same arrival time, check ab and ac but don't need to check bc
  int row_type = 0;
  int total_turnaround_time = 0;
  int avg_turnaround_time = 0;
  int avg_wait_time = 0;
  int avg_response_time = 0;
  int turnaround_time;
  int num_of_processes = 0;
  int start = 0;
  int finish = 0;
  int wait = 0;
  int response_time = 0;

  for (int i = 0; i < MAX_INPUT_SIZE; i++) { // outer for loop goes through all items in the array
    jump = 0;

    for (int j = i + 1; j < MAX_INPUT_SIZE; j++) { // inner for loop goes from index i, terminates if index i + 1 is not same arrival time (checking duplicates)
      if (all_processes[i].arrival_time != all_processes[j].arrival_time) { // two consecutive processes have different arrival times
        if (jump != 0) { // meaning we already have two processes w/ same arrival time and need to check 3rd, 4th, 5th...
          if (duplicates_index == duplicates_size) {
            duplicates_size += 1;
            duplicates = (struct Process*)realloc(duplicates, duplicates_size * sizeof(struct Process));
          }

          duplicates[duplicates_index] = all_processes[i];
        }

        i += jump; // jump to the next process which isn't a duplicate
        qsort(duplicates, duplicates_size, sizeof(struct Process), compare_arrival_time); // sort the duplicates from lowest to highest pid

        if(jump != 0) { // print only if there is something in the duplicates array
          for (int k = 0; k < duplicates_size; k++) {
            //printf("\n| %2d | %7d | %5d | %5d | %6d | %5d | %11d | %13d |", duplicates[k].pid, duplicates[k].arrival_time, duplicates[k].burst_length, duplicates[k].arrival_time, (duplicates[k].burst_length + duplicates[k].arrival_time), 0, duplicates[k].burst_length, 0);
            processes_output[duplicates[k].pid - 1].burst_length += duplicates[k].burst_length;
            if (processes_output[duplicates[k].pid - 1].arrival_time == -1) { // if -1, replace with arrival time. since csv is sorted by arrival time, it will be replaced by earliest value
              processes_output[duplicates[k].pid - 1].arrival_time = duplicates[k].arrival_time;
            }
            num_of_processes += 1;
          }
            duplicates_size = ORIGINAL_DUPLICATES_SIZE;
            duplicates = (struct Process*)realloc(duplicates, duplicates_size * sizeof(struct Process));
            duplicates_index = 0;
        }
        break; // don't need to check for consecutive duplicates anymore because next process is different

      } else { // two consecutive processes have the same arrival time
        if (jump == 0) {
          jump +=2;
        } else {
          jump += 1;
        }

        if (duplicates_index == duplicates_size) { // checking for out of bounds
          duplicates_size += 1;
          duplicates = (struct Process*)realloc(duplicates, duplicates_size * sizeof(struct Process));
        }

        duplicates[duplicates_index] = all_processes[j];
        duplicates_index += 1;
      }
    }

    turnaround_time = all_processes[i].burst_length + all_processes[i].arrival_time;
    total_turnaround_time += turnaround_time;
    processes_output[all_processes[i].pid - 1].burst_length += all_processes[i].burst_length;
    
    if (processes_output[all_processes[i].pid - 1].arrival_time == -1) { // if -1, replace with arrival time. since csv is sorted by arrival time, it will be replaced by earliest value
      processes_output[all_processes[i].pid - 1].arrival_time = all_processes[i].arrival_time;
    }

    // get start and finish times of all processes before process i
    for(int j = 1; j <= i; j++) {
      finish += all_processes[j].burst_length;
      wait += all_processes[j].burst_length;

      if (j != i) {
        response_time += all_processes[j].burst_length;
      }

      if (j == i) {
        wait -= all_processes[j].burst_length - all_processes[j].arrival_time;
        response_time += all_processes[j].time_until_first_response - all_processes[j].arrival_time;
      }

      if (j == 1) {
        start = all_processes[j].arrival_time;
      } else if (j != 1 && j == i) {
        start += all_processes[j-1].burst_length;
      } else {
        start += all_processes[j-1].burst_length;
      }
    }

    if (start < processes_start_times[all_processes[i].pid - 1] || processes_start_times[all_processes[i].pid - 1] == -1) {
      processes_start_times[all_processes[i].pid - 1] = start;
    }

    if (finish > processes_finish_times[all_processes[i].pid - 1] || processes_finish_times[all_processes[i].pid - 1] == 0) {
      processes_finish_times[all_processes[i].pid - 1] = finish;
    }

    if (processes_response_times[all_processes[i].pid - 1] == -1) {
      processes_response_times[all_processes[i].pid - 1] = response_time;
    }

    processes_wait_times[all_processes[i].pid - 1] += wait;

    start = 0;
    finish = 0;
    wait = 0;
    response_time = 0;

    num_of_processes += 1;
  }

  print_table(avg_turnaround_time, avg_wait_time, avg_response_time);
  free(duplicates);
}

void shortest_job_first() {
  int run_simulation = 1;
  int time = 0;
  int sum_of_burst_times = all_processes[0].burst_length;
  int index_next_process_arrival = 1;
  int avg_turnaround_time = 0;
  int avg_wait_time = 0;
  int avg_response_time = 0;
  struct Process current_process = all_processes[0];

  struct Process queue_array[MAX_INPUT_SIZE];
  heap* queue = createHeap(MAX_INPUT_SIZE);

  processes_start_times[all_processes[0].pid - 1] = current_process.arrival_time;

  processes_output[all_processes[0].pid - 1].arrival_time = all_processes[0].arrival_time;
  processes_output[all_processes[0].pid - 1].burst_length = all_processes[0].burst_length;
  processes_output[all_processes[0].pid - 1].arrival_time = all_processes[0].arrival_time;


  while (run_simulation == 1) {
    time++;

    // when time reaches arrival of next process, put process in job queue
    if (time == all_processes[index_next_process_arrival].arrival_time && index_next_process_arrival < MAX_INPUT_SIZE) {
      if (current_process.pid != all_processes[index_next_process_arrival].pid) { // don't put process in job queue if pid == current running pid
        insert(queue, all_processes[index_next_process_arrival], "burst");
        printf("inserted process %d/%d into job queue\nthis will take a while :)\nprobably shouldn't have used a min-heap :)\n\n", index_next_process_arrival, MAX_INPUT_SIZE);
      }
      index_next_process_arrival++; // check for next process
    }

    // when current process finishes running
    if (time == sum_of_burst_times) {
      current_process = extractMin(queue, "burst"); // take process with shortest burst and make it the current running process
      sum_of_burst_times += current_process.burst_length; // time at which process stops running

      processes_output[current_process.pid - 1].burst_length += current_process.burst_length;

      if (processes_output[current_process.pid - 1].arrival_time == -1) {
        processes_output[current_process.pid - 1].arrival_time = current_process.arrival_time;
      }

      if (processes_start_times[current_process.pid - 1] == -1) {
        processes_start_times[current_process.pid - 1] = time;
      }

      processes_wait_times[current_process.pid - 1] += time - current_process.arrival_time;

      if (processes_response_times[current_process.pid - 1] == -1) {
        processes_response_times[current_process.pid - 1] = time + current_process.time_until_first_response - current_process.arrival_time;
      }

      if (processes_finish_times[current_process.pid - 1] < time) {
        processes_finish_times[current_process.pid - 1] = time + current_process.burst_length;
      }
    }

    if (index_next_process_arrival >= MAX_INPUT_SIZE) {
      run_simulation = 0;
    }
  }

  print_table(avg_turnaround_time, avg_wait_time, avg_response_time);
}

void priority_scheduling() {
  int run_simulation = 1;
  int time = 0;
  int sum_of_burst_times = all_processes[0].burst_length;
  int index_next_process_arrival = 1;
  int avg_turnaround_time = 0;
  int avg_wait_time = 0;
  int avg_response_time = 0;
  struct Process current_process = all_processes[0];

  struct Process queue_array[MAX_INPUT_SIZE];
  heap* queue = createHeap(MAX_INPUT_SIZE);

  processes_start_times[all_processes[0].pid - 1] = current_process.arrival_time;

  processes_output[all_processes[0].pid - 1].arrival_time = all_processes[0].arrival_time;
  processes_output[all_processes[0].pid - 1].burst_length = all_processes[0].burst_length;
  processes_output[all_processes[0].pid - 1].arrival_time = all_processes[0].arrival_time;


  while (run_simulation == 1) {
    time++;

    // when time reaches arrival of next process, put process in job queue
    if (time == all_processes[index_next_process_arrival].arrival_time && index_next_process_arrival < MAX_INPUT_SIZE) {
      if (current_process.pid != all_processes[index_next_process_arrival].pid) { // don't put process in job queue if pid == current running pid
        insert(queue, all_processes[index_next_process_arrival], "priority");
        printf("inserted process %d/%d into job queue\nthis will take a while :)\nprobably shouldn't have used a min-heap :)\n\n", index_next_process_arrival, MAX_INPUT_SIZE);
      }
      index_next_process_arrival++; // check for next process
    }

    // when current process finishes running
    if (time == sum_of_burst_times) {
      current_process = extractMin(queue, "priority"); // take process with lowest prio and make it the current running process
      sum_of_burst_times += current_process.burst_length; // time at which process stops running

      processes_output[current_process.pid - 1].burst_length += current_process.burst_length;

      if (processes_output[current_process.pid - 1].arrival_time == -1) {
        processes_output[current_process.pid - 1].arrival_time = current_process.arrival_time;
      }

      if (processes_start_times[current_process.pid - 1] == -1) {
        processes_start_times[current_process.pid - 1] = time;
      }

      processes_wait_times[current_process.pid - 1] += time - current_process.arrival_time;

      if (processes_response_times[current_process.pid - 1] == -1) {
        processes_response_times[current_process.pid - 1] = time + current_process.time_until_first_response - current_process.arrival_time;
      }

      if (processes_finish_times[current_process.pid - 1] < time) {
        processes_finish_times[current_process.pid - 1] = time + current_process.burst_length;
      }
    }

    if (index_next_process_arrival >= MAX_INPUT_SIZE) {
      run_simulation = 0;
    }
  }

  print_table(avg_turnaround_time, avg_wait_time, avg_response_time);
}

void round_robin(int time_quantum) {
    int time = 0;
    int completed_processes = 0;
    int index_next_process_arrival = 0;
    int avg_turnaround_time = 0;
    int avg_wait_time = 0;
    int avg_response_time = 0;
    struct Process queue[MAX_INPUT_SIZE]; // Ready queue for RR
    int front = 0, rear = 0;
    int remaining_burst_times[PROCESS_COUNT]; // Stores remaining burst time for each process

    // Initialize the remaining burst times and the process queue
    for (int i = 0; i < PROCESS_COUNT; i++) {
        remaining_burst_times[i] = all_processes[i].burst_length;
        processes_start_times[i] = -1; // Initialize to -1 to signify that the process hasn't started yet
    }

    while (completed_processes < PROCESS_COUNT) {
        // Add processes to the ready queue based on their arrival time
        while (index_next_process_arrival < PROCESS_COUNT && all_processes[index_next_process_arrival].arrival_time <= time) {
            queue[rear] = all_processes[index_next_process_arrival];
            rear = (rear + 1) % MAX_INPUT_SIZE;
            index_next_process_arrival++;
        }

        // If the ready queue is not empty, process the front process
        if (front != rear) {
            struct Process current_process = queue[front];
            front = (front + 1) % MAX_INPUT_SIZE;

            int pid = current_process.pid - 1;

            // Set the start time for the process if it's being scheduled for the first time
            if (processes_start_times[pid] == -1) {
                processes_start_times[pid] = time;  // Set start time to current time
            }

            // Set the response time for the process if it's being scheduled for the first time
            if (processes_response_times[pid] == -1) {
                processes_response_times[pid] = time - current_process.arrival_time;
            }

            // Run the process for the time quantum or until it finishes
            int run_time = (remaining_burst_times[pid] > time_quantum) ? time_quantum : remaining_burst_times[pid];
            remaining_burst_times[pid] -= run_time;
            time += run_time;

            // Update the output structure after running the process
            if (remaining_burst_times[pid] == 0) {
                completed_processes++;
                processes_finish_times[pid] = time;
                processes_output[pid] = all_processes[pid]; // Update output data
            } else {
                // If the process is not done, reinsert into the ready queue
                queue[rear] = current_process;
                rear = (rear + 1) % MAX_INPUT_SIZE;
            }

            // Update the wait time
            processes_wait_times[pid] = time - processes_output[pid].arrival_time - (current_process.burst_length - remaining_burst_times[pid]);
        } else {
            // If no process is ready, increment time
            time++;
        }
    }

    // Compute averages and print the result table
    avg_turnaround_time /= PROCESS_COUNT;
    avg_wait_time /= PROCESS_COUNT;
    avg_response_time /= PROCESS_COUNT;

    write_results_to_file("rr_results.csv", avg_turnaround_time, avg_wait_time, avg_response_time);
    print_table_rrsrt(avg_turnaround_time, avg_wait_time, avg_response_time);
}

void shortest_remaining_time(float alpha) {
    int time = 0;
    int completed_processes = 0;
    int index_next_process_arrival = 0;
    int avg_turnaround_time = 0;
    int avg_wait_time = 0;
    int avg_response_time = 0;
    heap* queue = createHeap(MAX_INPUT_SIZE);
    
    // Initialize the remaining burst times for each process
    int remaining_burst_times[PROCESS_COUNT];
    for (int i = 0; i < PROCESS_COUNT; i++) {
        remaining_burst_times[i] = all_processes[i].burst_length;
    }

    while (completed_processes < PROCESS_COUNT) {
        // Add processes to the ready queue based on their arrival time
        while (index_next_process_arrival < PROCESS_COUNT && all_processes[index_next_process_arrival].arrival_time <= time) {
            // Insert using the predicted burst time as the sorting key
            insert(queue, all_processes[index_next_process_arrival], "predicted");
            index_next_process_arrival++;
        }

        if (queue->size > 0) {
            struct Process current_process = extractMin(queue, "predicted");
            int pid = current_process.pid - 1;

            // If it's the first time this process is scheduled
            if (processes_start_times[pid] == 0 && remaining_burst_times[pid] == all_processes[pid].burst_length) {
                processes_start_times[pid] = time;
            }

            // Set the response time if it's the first time it's being scheduled
            if (processes_response_times[pid] == -1) {
                processes_response_times[pid] = time - current_process.arrival_time;
            }

            // Calculate the run time; either the remaining burst time or the time until the next process arrives
            int run_time = remaining_burst_times[pid];
            if (index_next_process_arrival < PROCESS_COUNT && all_processes[index_next_process_arrival].arrival_time < time + run_time) {
                run_time = all_processes[index_next_process_arrival].arrival_time - time;
            }

            // Update the remaining burst time for the current process
            remaining_burst_times[pid] -= run_time;
            time += run_time;

            // If the process is finished, update the finish time and other stats
            if (remaining_burst_times[pid] == 0) {
                processes_finish_times[pid] = time;
                processes_output[pid] = current_process;
                completed_processes++;

                // Calculate wait time
                processes_wait_times[pid] = processes_finish_times[pid] - current_process.arrival_time - current_process.burst_length;

                // Calculate turnaround time
                int turnaround_time = processes_finish_times[pid] - current_process.arrival_time;
                avg_turnaround_time += turnaround_time;
            } else {
                // If the process is not done, reinsert it into the queue
                insert(queue, current_process, "predicted");
            }
        } else {
            // If no process is ready, increment time
            time++;
        }
    }

    // Compute averages
    for (int i = 0; i < PROCESS_COUNT; i++) {
        avg_wait_time += processes_wait_times[i];
        avg_response_time += (processes_response_times[i] != -1) ? processes_response_times[i] : 0;
    }

    avg_turnaround_time /= PROCESS_COUNT;
    avg_wait_time /= PROCESS_COUNT;
    avg_response_time /= PROCESS_COUNT;

    write_results_to_file("srt_results.csv", avg_turnaround_time, avg_wait_time, avg_response_time);
    // Print the result table
    print_table_rrsrt(avg_turnaround_time, avg_wait_time, avg_response_time);
}

void write_results_to_file(const char *filename, int avg_turnaround_time, int avg_wait_time, int avg_response_time) {
    FILE *fptr = fopen(filename, "w");
    
    if (fptr == NULL) {
        printf("Error opening file for writing!\n");
        return;
    }
    
    // Write the CSV header
    fprintf(fptr, "Process ID, Arrival Time, Burst Length, Start Time, Finish Time, Wait Time, Turnaround Time, Response Time\n");

    for (int i = 0; i < PROCESS_COUNT; i++) {
        if (processes_output[i].arrival_time == -1) {
            continue; // Skip processes with no valid arrival time
        }

        int turnaround_time = (processes_finish_times[i] == -1) ? 0 : processes_finish_times[i] - processes_output[i].arrival_time;

        fprintf(fptr, "%d, %d, %d, %d, %d, %d, %d, %d\n",
                processes_output[i].pid,
                processes_output[i].arrival_time,
                processes_output[i].burst_length,
                (processes_start_times[i] == -1) ? 0 : processes_start_times[i],
                (processes_finish_times[i] == -1) ? 0 : processes_finish_times[i],
                processes_wait_times[i],
                turnaround_time,
                (processes_response_times[i] == -1) ? 0 : processes_response_times[i]);
    }

    // Write the averages at the end
    fprintf(fptr, "\nAverage Turnaround Time, %d\n", avg_turnaround_time);
    fprintf(fptr, "Average Waiting Time, %d\n", avg_wait_time);
    fprintf(fptr, "Average Response Time, %d\n", avg_response_time);

    fclose(fptr);
}

void print_table(int avg_turnaround_time, int avg_wait_time, int avg_response_time) {
  printf("\n+----+---------+-------+-------+--------+--------------+-------------+---------------+");
  printf("\n| Id | Arrival | Burst | Start | Finish |     Wait     | Turnaround  | Response Time |");
  printf("\n+----+---------+-------+-------+--------+--------------+-------------+---------------+");

  for (int i = 0; i < PROCESS_COUNT; i++) {
    avg_turnaround_time += processes_finish_times[i] - processes_output[i].arrival_time;
    avg_wait_time += processes_wait_times[i];
    avg_response_time += processes_response_times[i];

    printf("\n| %2d | %7d | %5d | %5d | %6d | %12d | %11d | %13d |", processes_output[i].pid, processes_output[i].arrival_time, processes_output[i].burst_length, processes_start_times[i], processes_finish_times[i], processes_wait_times[i], processes_finish_times[i] - processes_output[i].arrival_time, processes_response_times[i]);
  }

  avg_turnaround_time /= PROCESS_COUNT;
  avg_wait_time /= PROCESS_COUNT;
  avg_response_time /= PROCESS_COUNT;

  printf("\n+----+---------+-------+-------+--------+--------------+-------------+---------------+");
  printf("\nAverage waiting time: %d ms", avg_wait_time);
  printf("\nAverage turnaround time: %d ms", avg_turnaround_time);
  printf("\nAverage response time: %d ms\n", avg_response_time);
}

void print_table_rrsrt(int avg_turnaround_time, int avg_wait_time, int avg_response_time) {
    printf("\n+----+---------+-------+-------+--------+--------------+-------------+---------------+");
    printf("\n| Id | Arrival | Burst | Start | Finish |     Wait     | Turnaround  | Response Time |");
    printf("\n+----+---------+-------+-------+--------+--------------+-------------+---------------+");

    for (int i = 0; i < PROCESS_COUNT; i++) {
        if (processes_output[i].arrival_time == -1) {
            continue; // Skip processes with no valid arrival time
        }

        int turnaround_time = (processes_finish_times[i] == -1) ? 0 : processes_finish_times[i] - processes_output[i].arrival_time;
        avg_turnaround_time += turnaround_time;

        avg_wait_time += processes_wait_times[i];
        avg_response_time += (processes_response_times[i] == -1) ? 0 : processes_response_times[i];

        printf("\n| %2d | %7d | %5d | %5d | %6d | %12d | %11d | %13d |",
               processes_output[i].pid,
               processes_output[i].arrival_time,
               processes_output[i].burst_length,
               (processes_start_times[i] == -1) ? 0 : processes_start_times[i],
               (processes_finish_times[i] == -1) ? 0 : processes_finish_times[i],
               processes_wait_times[i],
               turnaround_time,
               (processes_response_times[i] == -1) ? 0 : processes_response_times[i]);
    }

    avg_turnaround_time /= PROCESS_COUNT;
    avg_wait_time /= PROCESS_COUNT;
    avg_response_time /= PROCESS_COUNT;

    printf("\n+----+---------+-------+-------+--------+--------------+-------------+---------------+");
    printf("\nAverage waiting time: %d ms", avg_wait_time);
    printf("\nAverage turnaround time: %d ms", avg_turnaround_time);
    printf("\nAverage response time: %d ms\n", avg_response_time);
}


void to_lowercase(char *str) {
  while (*str) {
      *str = tolower((unsigned char)*str);
      str++;
  }
}

int compare_arrival_time(const void* a, const void* b) {
  return (*(int*)a - *(int*)b);
}

// some modifications but mostly from: https://stdin.top/posts/csv-in-c/
void csv_inputs_to_processes_array() {
    char row[1024];
    char *column;
    int column_number = 0;
    int i = 0; // Properly initialize i to store processes correctly

    while (fgets(row, 1024, stdin) != NULL) {
        struct Process process;
        column = strtok(row, ",");

        while (column != NULL) {
            switch (column_number) {
                case 0:
                    process.pid = atoi(column);
                    break;
                case 1:
                    process.arrival_time = atoi(column);
                    break;
                case 2:
                    process.time_until_first_response = atoi(column);
                    break;
                case 3:
                    process.burst_length = atoi(column);
                    break;
            }
            column_number += 1;
            column = strtok(NULL, ",");
        }
        column_number = 0;

        process.priority = (float)1 / process.pid;
        all_processes[i] = process;
        i += 1;
    }
}



// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
// Define a createHeap function
heap* createHeap(int capacity)
{
	// Allocating memory to heap h
	heap* h = (heap*)malloc(sizeof(heap));

	// Checking if memory is allocated to h or not
	if (h == NULL) {
		printf("Memory error");
		return NULL;
	}
	// set the values to size and capacity
	h->size = 0;
	h->capacity = capacity;

	// Allocating memory to array
	h->arr = (struct Process*)malloc(capacity * sizeof(struct Process));

	// Checking if memory is allocated to h or not
	if (h->arr == NULL) {
		printf("Memory error");
		return NULL;
	}

	return h;
}

// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
// Defining insertHelper function
void insertHelper(heap* h, int index, char* sort_by) {
    int parent = (index - 1) / 2;

    if (strcmp(sort_by, "burst") == 0) {
        if (index > 0 && h->arr[parent].burst_length > h->arr[index].burst_length) {
            struct Process temp = h->arr[parent];
            h->arr[parent] = h->arr[index];
            h->arr[index] = temp;
            insertHelper(h, parent, sort_by);
        }
    } else if (strcmp(sort_by, "predicted") == 0) {
        int pid1 = h->arr[parent].pid - 1;
        int pid2 = h->arr[index].pid - 1;
        if (index > 0 && predicted_burst_times[pid1] > predicted_burst_times[pid2]) {
            struct Process temp = h->arr[parent];
            h->arr[parent] = h->arr[index];
            h->arr[index] = temp;
            insertHelper(h, parent, sort_by);
        }
    }
}


// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
void heapify(heap* h, int index, char* sort_by) {
    int left = index * 2 + 1;
    int right = index * 2 + 2;
    int min = index;

    if (strcmp(sort_by, "burst") == 0) {
        if (left < h->size && h->arr[left].burst_length < h->arr[min].burst_length)
            min = left;
        if (right < h->size && h->arr[right].burst_length < h->arr[min].burst_length)
            min = right;
    } else if (strcmp(sort_by, "predicted") == 0) {
        int pid_min = h->arr[min].pid - 1;
        if (left < h->size) {
            int pid_left = h->arr[left].pid - 1;
            if (predicted_burst_times[pid_left] < predicted_burst_times[pid_min]) {
                min = left;
                pid_min = pid_left;
            }
        }
        if (right < h->size) {
            int pid_right = h->arr[right].pid - 1;
            if (predicted_burst_times[pid_right] < predicted_burst_times[pid_min]) {
                min = right;
            }
        }
    }

    if (min != index) {
        struct Process temp = h->arr[min];
        h->arr[min] = h->arr[index];
        h->arr[index] = temp;
        heapify(h, min, sort_by);
    }
}


// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
struct Process extractMin(heap* h, char* sort_by)
{
	struct Process deleteItem = {-1, -1, -1, -1};

	// Checking if the heap is empty or not
	if (h->size == 0) {
		printf("\nPlease wait for simulation to stop running :)");
    return deleteItem;
	}

	// Store the node in deleteItem that
	// is to be deleted.
	deleteItem = h->arr[0];

	// Replace the deleted node with the last node
	h->arr[0] = h->arr[h->size - 1];
	// Decrement the size of heap
	h->size--;

	// Call minheapify_top_down for 0th index
	// to maintain the heap property
	heapify(h, 0, sort_by);
	return deleteItem;
}

// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
// Define a insert function
void insert(heap* h, struct Process data, char* sort_by)
{
  if (h->size >= h->capacity) {
      printf("Heap is full\n");
      return;
  }

  h->arr[h->size] = data;
  insertHelper(h, h->size, sort_by);
  h->size++;
}

// modified to handle processes, but from: https://www.geeksforgeeks.org/c-program-to-implement-min-heap/
void printHeap(heap* h)
{

	for (int i = 0; i < h->size; i++) {
		printf("Process ID: %d, Burst Length: %d\n", h->arr[i].pid, h->arr[i].burst_length);
	}
	printf("\n");
}
