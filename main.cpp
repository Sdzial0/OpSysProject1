/*
  Contributors: Stephen Dzialo (dzials), Yarden Ne'eman (neemay), Tyler Peláez (pelaet)
*/

#include <iostream>
#include <deque>
#include <queue>
#include <fstream>
#include <iomanip>
#include "Parse.h"

using namespace std;

//Class to define custom Priority Queue comparison
class myPQComparator {
    public:
        bool operator() (const Process& p1, const Process& p2) const {
            return p1.remaining > p2.remaining;
        }
};

// GLOBAL VARIABLES
//
// map of processes
map<string,Process> processes;
// Queue of Processes in ready state
deque<Process> readyQueue;
// Priority Queue of Processes in ready state for SRT
priority_queue<Process, vector<Process>, myPQComparator> readyPQ;
// Number of processes
int n = 0;
// Number of processors (cores)
int m = 1;
// Time for context switch
int t_cs = 6;
// Round-robin time slice
int t_slice = 94;
// Time counter
int t = 0;
//Number of context switches
int cs = 0;

//For debugging Parser.h
void print_processes_map() {
    map<string, Process>::iterator itr;
    for(itr = processes.begin(); itr != processes.end(); ++itr) {
        cout << itr->first << " " << itr->second.arrival_time << " " << itr->second.burst_time << " " << itr->second.num_burst << " "  << itr->second.IOtime<< endl;
    }
}

void reset() {
    readyQueue.clear(); //clear the queue (though it should already be empty)
    t = 0; //reset the timer
    cs = 0; //reset the number of context switches

    map<string, Process>::iterator itr;
    for(itr = processes.begin(); itr != processes.end(); ++itr) {
        itr->second.reset();
    }

}

string printQueue() {
    string build = "[Q";
    if (readyQueue.size() == 0) {

        return build += " <empty>]";
    }
    for (deque<Process>::iterator itr = readyQueue.begin(); itr != readyQueue.end(); itr++) {
        build += " " + itr->p_name;
    }
    return build + "]";
}

string printPQueue() {
    string build = "[Q";
    if (readyPQ.size() == 0) {

        return build += " <empty>]";
    }
    priority_queue<Process, vector<Process>, myPQComparator> copyPQ(readyPQ);
    while(!copyPQ.empty()) {
        build += " " + copyPQ.top().p_name;
        copyPQ.pop();
    }
    return build + "]";
}

//To test behavior of Priority Queue
void testPQ () {
    //priority_queue<Process, vector<Process>, myPQComparator> pq;
    map<string, Process>::iterator itr;
    cout << printPQueue() << endl;
    for(itr = processes.begin(); itr != processes.end(); ++itr) {
        readyPQ.push(itr->second);
    }
    cout << printPQueue() << endl;
    for(int i = 0; i < n; i++) {
        cout << readyPQ.top().p_name << endl;
        readyPQ.pop();
    }
}


float calculate_waits() {
    float sum = 0;
    float bursts = 0;
    for (map<string, Process>::iterator itr = processes.begin(); itr != processes.end(); ++itr)
    {
        sum += (itr->second.total_wait);
        bursts += itr->second.total_bursts;
    }
    return sum / bursts;
}

float calculate_avg_CPU() {
    float sum = 0;
    float bursts = 0;
    for (map<string, Process>::iterator itr = processes.begin(); itr != processes.end(); ++itr)
    {
        sum += ((itr->second.burst_time) * itr->second.total_bursts);
        bursts += (itr->second.total_bursts);
    }
    return sum / bursts;
}

float calculate_turnaround() {

    //return calculate_waits() + calculate_avg_CPU() + t_cs;
    float sum = 0;
    float bursts = 0;
    for (map<string, Process>::iterator itr = processes.begin(); itr != processes.end(); ++itr)
    {
        sum += (itr->second.turnaround);
        bursts += itr->second.total_bursts;
    }
    return (sum / bursts) + (t_cs/2);
}

int calculate_pre() {
    int total = 0;
    for (map<string, Process>::iterator itr = processes.begin(); itr != processes.end(); ++itr)
    {
        total += (itr->second.preemption_cnt);
    }
    return total;
}

void print_statistics(ofstream &output, int x) {
    if(x == 1)
        output << "Algorithm FCFS" << endl;
    else if(x == 2)
        output << "Algorithm SRT" << endl;
    else
        output << "Algorithm RR" << endl;

    output << "-- average CPU burst time: " << setprecision(2) << fixed << calculate_avg_CPU() << " ms" << endl;
    output << "-- average wait time: " << setprecision(2) << fixed << calculate_waits() << " ms" << endl;
    output << "-- average turnaround time: " << setprecision(2) << fixed << calculate_turnaround() << " ms" << endl;
    output << "-- total number of context switches: " << cs << endl;
    output << "-- total number of preemptions: " << calculate_pre() << endl;
}

void FCFS() {
    int term_cnt = 0;
    int current_process_burst_time = 0;
    int context_switch_timer = t_cs / 2;
    Process* running = NULL;
    bool tie = false;


    printf("time %dms: Simulator started for FCFS %s\n", t, (printQueue()).c_str());
    fflush (stdout);


    while (term_cnt < n) {

        // Take care of ready processes and currently running process
        if (current_process_burst_time == 0 && context_switch_timer > 0) {
            // In a context switch out of the CPU, decrement timer
            context_switch_timer--;
        } else if (current_process_burst_time == 0 && context_switch_timer == 0){
            // Switch to new process
            if (!readyQueue.empty()) {
                cs++;
                running = &readyQueue.front();
                readyQueue.pop_front();
                processes.find(running->p_name)->second.total_wait += (t - (processes.find(running->p_name)->second.ready_start));

                printf("time %dms: Process %s started using the CPU %s\n", t, running->p_name.c_str(), printQueue().c_str());
                fflush(stdout);

                current_process_burst_time = running->burst_time;
                context_switch_timer = t_cs -1;
            }
        } else {
            current_process_burst_time--;
            if (current_process_burst_time == 0 ) {
                processes.find(running->p_name)->second.endBurst(t, t_cs);
                if (processes.find(running->p_name)->second.num_burst == 0) {
                    printf("time %dms: Process %s terminated %s\n", t, running->p_name.c_str(), printQueue().c_str());
                    fflush(stdout);
                    term_cnt++;
                    if (term_cnt == n) {
                        t += (t_cs / 2) - 1;
                    }

                } else {
                    if (processes.find(running->p_name)->second.num_burst == 1) {
                        printf("time %dms: Process %s completed a CPU burst; %d burst to go %s\n", t, running->p_name.c_str(), processes.find(running->p_name)->second.num_burst,  printQueue().c_str());
                        fflush(stdout);
                    } else {
                        printf("time %dms: Process %s completed a CPU burst; %d bursts to go %s\n", t, running->p_name.c_str(), processes.find(running->p_name)->second.num_burst,  printQueue().c_str());
                        fflush(stdout);
                    }
                    printf("time %dms: Process %s switching out of CPU; will block on I/O until time %dms %s\n", t, running->p_name.c_str(), t + running->IOtime + (t_cs / 2), printQueue().c_str());
                    fflush(stdout);

                }
                running = NULL;
                tie = true;
            }
        }

        // Handle blocking processes
        for (map<string, Process>::iterator itr2 = processes.begin(); itr2 != processes.end(); ++itr2) {
            if (t == itr2->second.end_blocking_time) {
                itr2->second.end_blocking_time = -1;
                readyQueue.push_back(itr2->second);
                itr2->second.ready_start = t + (t_cs/2);
                printf("time %dms: Process %s completed I/O; added to ready queue %s\n", t, itr2->second.p_name.c_str(), printQueue().c_str());
                fflush(stdout);
                (processes.find(itr2->second.p_name))->second.start_burst = t;
                if (running == NULL) {
                    context_switch_timer = (t_cs / 2) - 1;
                }
                if(running == NULL && tie) {
                    context_switch_timer = t_cs - 1;
                }
            }
        }

        // Find any processes that arrive at current time t and push process into queue
        map<string, Process>::iterator itr = processes.begin();
        while (itr->second.arrival_time <= t &&  itr != processes.end()) {
            if (itr->second.arrival_time == t) {
                itr->second.ready_start = t + (t_cs/2);
                readyQueue.push_back(itr->second);
                printf("time %dms: Process %s arrived and added to ready queue %s\n", t, itr->second.p_name.c_str(), (printQueue()).c_str());
                fflush (stdout);
                (processes.find(itr->second.p_name))->second.start_burst = t;
            }
            itr++;
        }
        tie = false;
        t++;
    }
    printf("time %dms: Simulator ended for FCFS\n", t);
    fflush (stdout);

}

void SRT() {
    int term_cnt = 0;
    int current_process_burst_time = 0;
    int context_switch_timer = t_cs / 2;
    const Process* running = NULL;
    Process* copy = new Process();
    bool pre = false;
    bool tie = false;


    printf("time %dms: Simulator started for SRT %s\n", t, (printPQueue()).c_str());
    fflush (stdout);


    while (term_cnt < n) {

        // Take care of ready processes and currently running process
        if ((current_process_burst_time == 0 && context_switch_timer > 0) || (pre && context_switch_timer > 0)) {
            // In a context switch out of the CPU, decrement timer
            context_switch_timer--;
        } else if ((current_process_burst_time == 0 && context_switch_timer == 0) || (pre && context_switch_timer == 0)){
            // Switch to new process
            if (!readyPQ.empty() && !pre) {
                cs++;
                running = &readyPQ.top();
                copy->copy_process(*running);
                readyPQ.pop();
                processes.find(copy->p_name)->second.total_wait += (t - (processes.find(copy->p_name)->second.ready_start));

                if(copy->burst_time == (processes.find(copy->p_name))->second.remaining) {
                    printf("time %dms: Process %s started using the CPU %s\n", t, copy->p_name.c_str(), printPQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = copy->burst_time;
                }
                else {
                    printf("time %dms: Process %s started using the CPU with %dms remaining %s\n", t, copy->p_name.c_str(), (processes.find(copy->p_name))->second.remaining, printPQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = copy->remaining;
                }
                context_switch_timer = t_cs - 1;
            }
            else if(pre && context_switch_timer == 0) {
                cs++;
                copy->copy_process(*running);
                if(copy->burst_time == (processes.find(copy->p_name))->second.remaining) {
                    printf("time %dms: Process %s started using the CPU %s\n", t, copy->p_name.c_str(), printPQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = copy->burst_time;
                }
                else {
                    printf("time %dms: Process %s started using the CPU with %dms remaining %s\n", t, copy->p_name.c_str(), (processes.find(copy->p_name))->second.remaining, printPQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = copy->remaining;
                }
                context_switch_timer = t_cs - 1;
                pre = false;
            }
        } else {
            current_process_burst_time--;
            (processes.find(copy->p_name))->second.remaining--;
            if (current_process_burst_time == 0 ) {
                processes.find(copy->p_name)->second.endBurst(t, t_cs);
                if (processes.find(copy->p_name)->second.num_burst == 0) {
                    printf("time %dms: Process %s terminated %s\n", t, copy->p_name.c_str(), printPQueue().c_str());
                    fflush(stdout);
                    term_cnt++;
                    if (term_cnt == n) {
                        t += (t_cs / 2) - 1;
                    }

                } else {
                    if (processes.find(copy->p_name)->second.num_burst == 1) {
                        printf("time %dms: Process %s completed a CPU burst; %d burst to go %s\n", t, copy->p_name.c_str(), processes.find(copy->p_name)->second.num_burst,  printPQueue().c_str());
                        fflush(stdout);
                    } else {
                        printf("time %dms: Process %s completed a CPU burst; %d bursts to go %s\n", t, copy->p_name.c_str(), processes.find(copy->p_name)->second.num_burst,  printPQueue().c_str());
                        fflush(stdout);
                    }
                    printf("time %dms: Process %s switching out of CPU; will block on I/O until time %dms %s\n", t, copy->p_name.c_str(), t + copy->IOtime + (t_cs / 2), printPQueue().c_str());
                    fflush(stdout);

                }
                running = NULL;
                tie = true;
            }
        }

        // Handle blocking processes
        for (map<string, Process>::iterator itr2 = processes.begin(); itr2 != processes.end(); ++itr2) {
            if (t == itr2->second.end_blocking_time) {
                // A process comes out of blocking at current time
                itr2->second.end_blocking_time = -1;
                if(running != NULL && itr2->second.remaining < (processes.find(copy->p_name))->second.remaining) {
                    // Check if newly arrived process preempts the current process
                    (processes.find(copy->p_name))->second.preemption_cnt++;
                    //(processes.find(copy->p_name))->second.remaining--;
                    printf("time %dms: Process %s completed I/O and will preempt %s %s\n", t, itr2->second.p_name.c_str(), copy->p_name.c_str(), (printPQueue()).c_str());
                    (processes.find(copy->p_name))->second.ready_start = t + (t_cs/2);
                    readyPQ.push((processes.find(copy->p_name))->second);
                    running = &(itr2->second);
                    pre = true;
                    context_switch_timer = t_cs - 1;
                    (processes.find(itr2->second.p_name))->second.start_burst = t;
                }
                else {
                    readyPQ.push(itr2->second);
                    itr2->second.ready_start = t + (t_cs/2);
                    printf("time %dms: Process %s completed I/O; added to ready queue %s\n", t, itr2->second.p_name.c_str(), printPQueue().c_str());
                    fflush(stdout);
                    (processes.find(itr2->second.p_name))->second.start_burst = t;
                }
                if (running == NULL) {
                    context_switch_timer = (t_cs / 2) - 1;
                }
                if(running == NULL && tie) {
                    context_switch_timer = t_cs - 1;
                }
            }
        }

        // Find any processes that arrive at current time t and push process into queue
        map<string, Process>::iterator itr = processes.begin();
        while (itr->second.arrival_time <= t &&  itr != processes.end()) {
            if (itr->second.arrival_time == t) {
                // New process has arrived
                if(running != NULL && itr->second.remaining < (processes.find(copy->p_name))->second.remaining) {
                    // The newly arrived process has shorter remaining time than
                    // the process currently running, preemption occurs
                    (processes.find(copy->p_name))->second.preemption_cnt++;
                    //(processes.find(copy->p_name))->second.remaining--;
                    printf("time %dms: Process %s arrived and will preempt %s %s\n", t, itr->second.p_name.c_str(), copy->p_name.c_str(), (printPQueue()).c_str());
                    (processes.find(copy->p_name))->second.ready_start = t + (t_cs/2);
                    readyPQ.push((processes.find(copy->p_name))->second);
                    running = &(itr->second);
                    pre = true;
                    context_switch_timer = t_cs - 1;
                    (processes.find(itr->second.p_name))->second.start_burst = t;
                }
                else {
                    itr->second.ready_start = t + (t_cs/2);
                    readyPQ.push(itr->second);
                    printf("time %dms: Process %s arrived and added to ready queue %s\n", t, itr->second.p_name.c_str(), (printPQueue()).c_str());
                    (processes.find(itr->second.p_name))->second.start_burst = t;
                }
                fflush (stdout);
            }
            itr++;
        }
        tie = false;
        t++;
    }
    printf("time %dms: Simulator ended for SRT\n", t);
    fflush (stdout);
}

void RR() {
    int term_cnt = 0;
    int current_process_burst_time = 0;
    int context_switch_timer = t_cs / 2;
    int current_t_slice = 0;
    Process* running = NULL;
    bool pre = false;
    bool tie = false;


    printf("time %dms: Simulator started for RR %s\n", t, (printQueue()).c_str());
    fflush (stdout);


    while (term_cnt < n) {

        if (current_t_slice == t_slice && !readyQueue.empty() && current_process_burst_time != 1) {
            // Current process gets preempted
            (processes.find(running->p_name))->second.preemption_cnt++;
            (processes.find(running->p_name))->second.remaining--;

            printf("time %dms: Time slice expired; process %s preempted with %dms to go %s\n", t, running->p_name.c_str(), (processes.find(running->p_name))->second.remaining, printQueue().c_str());



            (processes.find(running->p_name))->second.ready_start = t - (t_cs / 2);
            readyQueue.push_back((processes.find(running->p_name))->second);



            running = &(readyQueue.front());
            readyQueue.pop_front();

            processes.find(running->p_name)->second.total_wait += (t - (processes.find(running->p_name)->second.ready_start));

            pre = true;
            context_switch_timer = t_cs;

            current_t_slice = 0;

        } else if (current_t_slice == t_slice && current_process_burst_time != 1) {
            printf("time %dms: Time slice expired; no preemption because ready queue is empty %s\n", t, printQueue().c_str());
            current_t_slice = 0;
        }

        // Take care of ready processes and currently running process
        if ((current_process_burst_time == 0 && context_switch_timer > 0) || (pre && context_switch_timer > 0)) {
            // In a context switch out of the CPU, decrement timer
            context_switch_timer--;
        } else if ((current_process_burst_time == 0 && context_switch_timer == 0) || (pre && context_switch_timer == 0)) {
            // Switch to new process
            if (!readyQueue.empty() && !pre) {
                cs++;
                running = &readyQueue.front();
                readyQueue.pop_front();
                processes.find(running->p_name)->second.total_wait += (t - (processes.find(running->p_name)->second.ready_start));

                if(running->burst_time == running->remaining) {
                    printf("time %dms: Process %s started using the CPU %s\n", t, running->p_name.c_str(), printQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = running->burst_time;
                }
                else {
                    printf("time %dms: Process %s started using the CPU with %dms remaining %s\n", t, running->p_name.c_str(), running->remaining, printQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = running->remaining;
                }

                context_switch_timer = t_cs -1;
                current_t_slice = 1;
            } else if(pre && context_switch_timer == 0) {
                cs++;

                if(processes.find(running->p_name)->second.burst_time == processes.find(running->p_name)->second.remaining) {
                    printf("time %dms: Process %s started using the CPU %s\n", t, running->p_name.c_str(), printQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = running->burst_time;
                }
                else {
                    printf("time %dms: Process %s started using the CPU with %dms remaining %s\n", t, running->p_name.c_str(), running->remaining, printQueue().c_str());
                    fflush(stdout);
                    current_process_burst_time = running->remaining;
                }
                context_switch_timer = t_cs - 1;
                current_t_slice = 1;
                pre = false;
            }
        } else {
            current_process_burst_time--;
            (processes.find(running->p_name))->second.remaining--;
            current_t_slice++;

            if (current_process_burst_time == 0 ) {
                current_t_slice = 0;
                processes.find(running->p_name)->second.endBurst(t, t_cs);
                if (processes.find(running->p_name)->second.num_burst == 0) {
                    printf("time %dms: Process %s terminated %s\n", t, running->p_name.c_str(), printQueue().c_str());
                    fflush(stdout);
                    term_cnt++;
                    if (term_cnt == n) {
                        t += (t_cs / 2) - 1;
                    }

                } else {
                    if (processes.find(running->p_name)->second.num_burst == 1) {
                        printf("time %dms: Process %s completed a CPU burst; %d burst to go %s\n", t, running->p_name.c_str(), processes.find(running->p_name)->second.num_burst,  printQueue().c_str());
                        fflush(stdout);
                    } else {
                        printf("time %dms: Process %s completed a CPU burst; %d bursts to go %s\n", t, running->p_name.c_str(), processes.find(running->p_name)->second.num_burst,  printQueue().c_str());
                        fflush(stdout);
                    }
                    printf("time %dms: Process %s switching out of CPU; will block on I/O until time %dms %s\n", t, running->p_name.c_str(), t + running->IOtime + (t_cs / 2), printQueue().c_str());
                    fflush(stdout);

                }
                running = NULL;
                tie = true;
            }
        }

        // Handle blocking processes
        for (map<string, Process>::iterator itr2 = processes.begin(); itr2 != processes.end(); ++itr2) {
            if (t == itr2->second.end_blocking_time) {
                itr2->second.end_blocking_time = -1;
                readyQueue.push_back(itr2->second);
                itr2->second.ready_start = t + (t_cs/2);
                printf("time %dms: Process %s completed I/O; added to ready queue %s\n", t, itr2->second.p_name.c_str(), printQueue().c_str());
                fflush(stdout);
                (processes.find(itr2->second.p_name))->second.start_burst = t;
                if (running == NULL) {
                    context_switch_timer = (t_cs / 2) - 1;
                }
                if(running == NULL && tie) {
                    context_switch_timer = t_cs - 1;
                }
            }
        }

        // Find any processes that arrive at current time t and push process into queue
        map<string, Process>::iterator itr = processes.begin();
        while (itr->second.arrival_time <= t &&  itr != processes.end()) {
            if (itr->second.arrival_time == t) {
                itr->second.ready_start = t + (t_cs/2);
                readyQueue.push_back(itr->second);
                printf("time %dms: Process %s arrived and added to ready queue %s\n", t, itr->second.p_name.c_str(), (printQueue()).c_str());
                fflush (stdout);
                (processes.find(itr->second.p_name))->second.start_burst = t;
            }
            itr++;
        }
        tie = false;
        t++;
    }
    printf("time %dms: Simulator ended for RR\n", t);
    fflush (stdout);

}

int main(int argc, char const *argv[]) {
    if(argc == 1){
        cerr << "ERROR: Invalid arguments" << endl;
        cerr << "USAGE: ./a.out <input-file> <stats-output-file>" << endl;
        return 0;
    }
    processes = Parse::read((string)argv[1]);
    n = processes.size();

    ofstream output;
    output.open(argv[2]);

    reset();
    FCFS();
    print_statistics(output, 1);
    cout << endl;
    reset();
    SRT();
    print_statistics(output, 2);
    cout << endl;
    reset();
    RR();
    print_statistics(output, 3);


    output.close();
    return 0;
}
