#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "proclist.h"

// Maximum size of a command line to keep in the process list
#define MAX_NAME_SIZE 30

proc_t *initProcList() {
    proc_t *head = safe_malloc(sizeof(head));
    *head = NULL;
    return head;
}

int addProcess(proc_t *head, int pid, state status, char **commandName) {
    DEBUG_PRINTF("Adding process %d to the list\n", pid);
    if (*head == NULL) { // The list is empty
        DEBUG_PRINT("List initialized\n");
        *head = createProcess(1, pid, status, commandName);
        return 1;
    }
    else {
        proc_t current = *head;
        // Reach the end of the list
        while (current->next != NULL) {
            current = current->next;
        }
        int newID = current->id + 1;
        current->next = createProcess(newID, pid, status, commandName);
        return newID;
    }
}

proc_t createProcess(int id, int pid, state status, char **commandName) {
    proc_t newProc;
    DEBUG_PRINT("Allocating new process\n");
    newProc = safe_malloc(sizeof(struct procList));

    // Copy the command line
    char *name = safe_malloc(MAX_NAME_SIZE);
    strcpy(name, *commandName);
    char **ptr = commandName;
    for (char *c = *++ptr; c != NULL; c = *++ptr) {
        if (strlen(name) + strlen(c) + 4 < MAX_NAME_SIZE) {
            strcat(name, " ");
            strcat(name, c);
        }
        else {
            DEBUG_PRINT("Command name too long\n");
            break;
        }
    }
    strcat(name, " &");

    newProc->id = id;
    newProc->pid = pid;
    newProc->state = status;
    newProc->commandName = name;
    gettimeofday(&(newProc->time), NULL);
    newProc->next = NULL;
    return newProc;
}

int lengthProcList(proc_t *head) {
    proc_t current = *head;
    int n = 0;
    while (current != NULL) {
        n++;
        current = current->next;
    }
    return n;
}

void removeProcessByID(proc_t *head, int id) {
    if (*head == NULL) {
        return; // Don't do anything if the list is empty
    }

    if ((*head)->id == id) {
        // Remove the first process of the list
        proc_t next = (*head)->next;
        free((*head)->commandName);
        free(*head);
        *head = next;
        DEBUG_PRINTF("Process %d removed\n", id);
        return;
    }

    // Remove a process that is not the first in the list
    proc_t current = *head;
    while (current->next != NULL && current->next->id != id) {
        current = current->next;
    }
    if (current->next != NULL) {
        proc_t tmp = current->next;
        current->next = tmp->next;
        free(tmp->commandName);
        free(tmp);
        DEBUG_PRINTF("Process %d removed\n", id);
        return;
    }
    DEBUG_PRINTF("Process %d not found\n", id);
}

void removeProcessByPID(proc_t *head, int pid) {
    int id = getID(head, pid);
    removeProcessByID(head, id);
}

void printProcess(proc_t proc, int lastID, int previousID) {
    // Print the ID of the process
    printf("[%d]", proc->id);
    // Print a special character if the process is the last or second-to-last modified
    if (proc->id == lastID)
        printf("+  ");
    else if (proc->id == previousID)
        printf("-  ");
    else
        printf("   ");
    // Print the state of the process
    if (proc->state == SUSPENDED)
        printf("Stopped\t\t      ");
    else if (proc->state == ACTIVE)
        printf("Running\t\t      ");
    else if (proc->state == DONE)
        printf("Done\t\t      ");
    // Print the command executed by the process
    printf("%s\n", proc->commandName);
}

void printProcessByID(proc_t *head, int id) {
    int lastID, previousID;
    getLastTwoProcesses(head, &lastID, &previousID);
    proc_t current = *head;
    while (current != NULL) {
        // Print the information about the process
        if (current->id == id) {
            printProcess(current, lastID, previousID);
        }
        current = current->next;
    }
}

void printProcessByPID(proc_t *head, int pid) { printProcessByID(head, getID(head, pid)); }

void printProcList(proc_t *head) {
    if (*head == NULL) { // Empty list
        printf("\n");
        return;
    }

    // Get the last two processes
    int lastID, previousID;
    getLastTwoProcesses(head, &lastID, &previousID);
    DEBUG_PRINTF("Last two processes: last=%d and previous=%d\n", lastID, previousID);

    // Loop trough each process in the list
    proc_t current = *head;
    while (current != NULL) {
        // Print the information about the process
        printProcess(current, lastID, previousID);
        current = current->next;
    }
}

void getLastTwoProcesses(proc_t *head, int *lastID, int *previousID) {
    // Initialize previous and last to minimum time
    *lastID = 0;
    *previousID = 0;
    struct timeval lastTime;
    timerclear(&lastTime);
    struct timeval previousTime;
    timerclear(&previousTime);

    proc_t current = *head;

    while (current != NULL) {
        // current time > last time
        if (timercmp(&(current->time), &lastTime, >)) {
            // Update previous and last
            previousTime = lastTime;
            *previousID = *lastID;
            lastTime = current->time;
            *lastID = current->id;
        }
        // current time > previous time
        else if (current->id != *lastID && timercmp(&(current->time), &previousTime, >)) {
            // Update previous
            previousTime = current->time;
            *previousID = current->id;
        }
        current = current->next;
    }
}

void setProcessStatusByPID(proc_t *head, int pid, state status) {
    proc_t current = *head;
    while (current != NULL) {
        if (current->pid == pid) {
            current->state = status;
            gettimeofday(&(current->time), NULL);
            DEBUG_PRINTF("[%d] Status changed to %d\n", pid, current->state);
            return;
        }
        current = current->next;
    }
    DEBUG_PRINTF("[%d] Process not found in the list\n", pid);
}

void setProcessStatusByID(proc_t *head, int id, state status) {
    int pid = getPID(head, id);
    setProcessStatusByPID(head, pid, status);
}

void updateProcList(proc_t *head) {
    proc_t current = *head;
    proc_t next;

    // Get the last two processes
    int lastID, previousID;
    getLastTwoProcesses(head, &lastID, &previousID);

    while (current != NULL) {
        next = current->next;
        if (current->state == DONE) {
            printProcess(current, lastID, previousID);
            removeProcessByID(head, current->id);
        }
        current = next;
    }
}

state getProcessStatusByPID(proc_t *head, int pid) {
    proc_t current = *head;
    while (current != NULL) {
        if (current->pid == pid) {
            DEBUG_PRINTF("[%d] Status found = %d\n", pid, current->state);
            return current->state;
        }
        current = current->next;
    }
    DEBUG_PRINTF("[%d] Process not found in the list\n", pid);
    return UNDEFINED;
}

int getID(proc_t *head, int pid) {
    proc_t current = *head;
    while (current != NULL) {
        if (current->pid == pid) {
            return current->id;
        }
        current = current->next;
    }
    return 0;
}

int getPID(proc_t *head, int id) {
    proc_t current = *head;
    while (current != NULL) {
        if (current->id == id) {
            return current->pid;
        }
        current = current->next;
    }
    return 0;
}

void deleteProcList(proc_t *head) {
    proc_t current = *head;
    proc_t tmp = NULL;
    while (current != NULL) {
        tmp = current;
        current = current->next;
        free(tmp->commandName);
        free(tmp);
    }
    free(head);
}