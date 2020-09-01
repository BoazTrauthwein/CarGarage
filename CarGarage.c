#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

typedef enum
{
    false,
    true
} bool; //Making boolean variables avalible.

typedef struct Resource
{
    int type;
    char *name;
    int amount;
    sem_t sem;
} Resource;

typedef struct Repair
{
    int type;
    char *name;
    int timeInHour;
    int numOfResources;
    int *resourceTypes;
} Repair;

typedef struct Request
{
    char *carLicense;
    int arrivalTime;
    int numOfRepairs;
    int *repairTypes;
    bool absorbed;
} Request;

void RetrieveResources(FILE *fdResources);
void RetrieveRepairs(FILE *fdRepairs);
void RetrieveRequests(FILE *fdRequests);
void *MyClock();
bool FinishedAllRequest(Request *requests);
void* WorkOnTheCar(void *carRequest);
Repair *GetRepair(int repairType);
Resource *GetResource(int resType);

int sizeResources, sizeRepairs, sizeRequests, simClock;
Resource *resources;
Repair *repairs;
Request *requests;

int main(int argc, char *argv[])
{
    int i = 0, w = 0;
    pthread_t *workers;
    pthread_t timeThread;
    FILE *fdResources, *fdRepairs, *fdRequests; 
    
    //Opening all the files.
    if ((fdResources = fopen(argv[1], "r")) == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    if ((fdRepairs = fopen(argv[2], "r")) == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    if ((fdRequests = fopen(argv[3], "r")) == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    //Initializing simulation clock to zero.
    simClock = 0;

    //Creating thread for clock
    if (pthread_create(&timeThread, NULL, MyClock, NULL))
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    //Retriving all the data from the files.
    RetrieveResources(fdResources);
    RetrieveRepairs(fdRepairs);
    RetrieveRequests(fdRequests);

    //Allocating an array of workers of type pthread_t.
    workers = (pthread_t *)malloc((sizeRequests) * sizeof(pthread_t));
    if (workers == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    //While requests are not absorbed 
    while (FinishedAllRequest(requests) == false)
    {
        for (i = 0; i < sizeRequests; i++)
            //Send the worker(pthread) to work on the car if the cars arrival time equals to the clock time and the request is not absorbed.
            if (requests[i].arrivalTime == simClock && requests[i].absorbed == false)
            {
                requests[i].absorbed = true;
                if (pthread_create(&workers[w++], NULL, WorkOnTheCar, (void *)&requests[i]) != 0)
                {
                    perror("pthread_create");
                    exit(EXIT_FAILURE);
                }
            }
    }
    //Send the workers home :)
    for (i = 0; i < w; i++)
        pthread_join(workers[i], NULL);

    //Cancel the clock thread
    pthread_cancel(timeThread);

    fclose(fdResources);
    fclose(fdRepairs);
    fclose(fdRequests);

    //free all allocated memory
    for (i = 0; i < sizeResources; i++)
        free(resources[i].name);
    free(resources);

    for (i = 0; i < sizeRepairs; i++)
    {
        free(repairs[i].name);
        free(repairs[i].resourceTypes);
    }
    free(repairs);

    for (i = 0; i < sizeRequests; i++)
    {
        free(requests[i].carLicense);
        free(requests[i].repairTypes);
    }
    free(requests);

    free(workers);

    return 1;
}

void RetrieveResources(FILE *fdResources)
//Input: File Handler for resources.txt.
//The function retrives each row to variables: type and name  and amount of rescourse of the garage.
{
    int i = 0;
    char buff[1000];
    char *str;

    resources = calloc(0, sizeof(Resource));
    if (resources == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    while (fscanf(fdResources, "%[^\n]%*c", buff) == 1)
    {
        resources = (Resource *)realloc(resources, (i + 1) * sizeof(Resource));
        if (resources == NULL)
        {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        str = strtok(buff, "\t");
        resources[i].type = atoi(str);
        str = strtok(NULL, "\t");
        resources[i].name = (char *)malloc((strlen(str) + 1) * sizeof(char));
        if (resources[i].name == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(resources[i].name, str);
        str = strtok(NULL, "\t");
        resources[i].amount = atoi(str);
        sem_init(&resources[i].sem, 0, resources[i].amount);
        i++;
    }
    sizeResources = i;
}

void RetrieveRepairs(FILE *fdRepairs)
//Input: File Handler for repairs.txt.
//The function retrives each row to variables: type and name of repairs, repair time in hour,
//num of rescourse to use and all the rescourse types that is needed for the repair.
{
    int i = 0, y;
    char buff[1000], *str;

    repairs = calloc(0, sizeof(Repair));
    if (repairs == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    i = 0;
    while (fscanf(fdRepairs, "%[^\n]%*c", buff) == 1)
    {
        repairs = (Repair *)realloc(repairs, (i + 1) * sizeof(Repair));
        if (repairs == NULL)
        {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        str = strtok(buff, "\t");
        repairs[i].type = atoi(str);
        str = strtok(NULL, "\t");
        repairs[i].name = (char *)malloc((strlen(str) + 1) * sizeof(char));
        if (repairs[i].name == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(repairs[i].name, str);
        str = strtok(NULL, "\t");
        repairs[i].timeInHour = atoi(str);
        str = strtok(NULL, "\t");
        repairs[i].numOfResources = atoi(str);
        if (repairs[i].numOfResources != 0)
        {
            repairs[i].resourceTypes = (int *)malloc(repairs[i].numOfResources * sizeof(int));
            if (repairs[i].resourceTypes == NULL)
            {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            for (y = 0; y < repairs[i].numOfResources; y++)
            {
                str = strtok(NULL, "\t");
                repairs[i].resourceTypes[y] = atoi(str);
            }
        }
        i++;
    }
    sizeRepairs = i;
}

void RetrieveRequests(FILE *fdRequests)
//Input: File Handler for request.txt.
//The function retrives each row to variables: Car license, arrival time, number of repairs and all the repair types.
{
    int i = 0, y;
    char buff[1000], *str;

    requests = calloc(0, sizeof(Request));
    if (requests == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    i = 0;
    while (fscanf(fdRequests, "%[^\n]%*c", buff) == 1)
    {
        requests = (Request *)realloc(requests, (i + 1) * sizeof(Request));
        if (requests == NULL)
        {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        str = strtok(buff, "\t");
        requests[i].carLicense = (char *)malloc((strlen(str) + 1) * sizeof(char));
        if (requests[i].carLicense == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(requests[i].carLicense, str);
        str = strtok(NULL, "\t");
        requests[i].arrivalTime = atoi(str);
        str = strtok(NULL, "\t");
        requests[i].numOfRepairs = atoi(str);
        if (requests[i].numOfRepairs != 0)
        {
            requests[i].repairTypes = (int *)malloc(requests[i].numOfRepairs * sizeof(int));
            if (requests[i].repairTypes == NULL)
            {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            for (y = 0; y < requests[i].numOfRepairs; y++)
            {
                str = strtok(NULL, "\t");
                requests[i].repairTypes[y] = atoi(str);
            }
        }
        //Setting absorbed to false.
        requests[i].absorbed = false;
        i++;
    }
    sizeRequests = i;
}

void *MyClock()
//Managing a clock that every second is an hour.
{
    simClock = 0;
    while (1)
    {
        sleep(1);
        simClock++;
    }
}

bool FinishedAllRequest(Request *requests)
//Input: All the requests.
//Checking if all the cars are absorbed or not.
{
    int i;
    for (i = 0; i < sizeRequests; i++)
        if (requests[i].absorbed == false)
            return false;
    return true;
}

void* WorkOnTheCar(void *carRequest)
//Input: a car request
//The worker(thread) repairs the car.
{
    int i, y;
    Request *doRequest;
    Repair *doRepair;
    Resource *useResource;

    doRequest = (Request*)carRequest;

    printf("car %s time: %d arrived garage\n", doRequest->carLicense, simClock);

    //Going over the list of all repairs 
    for (i = 0; i < doRequest->numOfRepairs; i++)
    {
        //The required repairs of the car.
        doRepair = GetRepair(doRequest->repairTypes[i]);
        printf("car %s time: %d needs %s\n", doRequest->carLicense, simClock, doRepair->name);
        //Taking all the rescourses for repair.
        for (y = 0; y < doRepair->numOfResources; y++)
        {
            useResource = GetResource(doRepair->resourceTypes[y]);
            sem_wait(&useResource->sem);
        }
        printf("car %s time: %d started %s\n", doRequest->carLicense, simClock, doRepair->name);
        //Executing repair.
        sleep(doRepair->timeInHour);
        //Returning/freeing the rescourses.
        for (y = 0; y < doRepair->numOfResources; y++)
        {
            useResource = GetResource(doRepair->resourceTypes[y]);
            sem_post(&useResource->sem);
        }
        printf("car %s time: %d completed %s\n", doRequest->carLicense, simClock, doRepair->name);
    }
    printf("car %s time: %d service complete\n", doRequest->carLicense, simClock);
    return NULL;
}

Repair *GetRepair(int repType)
//Input: repair type.
//Returning the repair that matches required repair type.
{
    int i;
    for (i = 0; i < sizeRepairs; i++)
        if (repairs[i].type == repType)
            return &repairs[i];
    return NULL;
}

Resource *GetResource(int resType)
//Input: rescourse type.
//Returning the rescourse that matches required rescourse type.
{
    int i;
    for (i = 0; i < sizeRepairs; i++)
        if (resources[i].type == resType)
            return &resources[i];
    return NULL;
}
