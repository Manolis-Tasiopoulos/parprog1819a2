#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define NUM_THREADS 4           //Number of threads on thread pool
#define QUEUE_LENGTH 1000000     //Length of queue
#define ARRAY_LENGTH 100       //Length of array that has to be sorted
#define THRESHOLD 10            //Threshold that choose sorting method


#define WORK 0
#define FINISH 1
#define STOP 2



struct message
{
  int type;
  int start;
  int end;
};


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  msg_in = PTHREAD_COND_INITIALIZER;
pthread_cond_t  msg_out = PTHREAD_COND_INITIALIZER;

struct message message_queue[QUEUE_LENGTH];
int qin = 0, qout = 0;
int msg_count = 0;



void send(int type, int start, int end)
{
  pthread_mutex_lock(&mutex);


  while(msg_count >= QUEUE_LENGTH)
  {
    printf("*Locked send*\n");
    pthread_cond_wait(&msg_out, &mutex);
  }

  message_queue[qin].type = type;
  message_queue[qin].start = start;
  message_queue[qin].end = end;

  qin = (qin + 1) % QUEUE_LENGTH;

  msg_count++;

  pthread_cond_signal(&msg_in);
  pthread_mutex_unlock(&mutex);


}



void receive(int *type, int *start, int *end)
{
  pthread_mutex_lock(&mutex);


  while(msg_count < 1)
  {
    printf("*Locked receive*\n");
    pthread_cond_wait(&msg_in, &mutex);
  }

  *type = message_queue[qout].type;
  *start = message_queue[qout].start;
  *end = message_queue[qout].end;

  qout = (qout + 1) % QUEUE_LENGTH;

  msg_count--;

  pthread_cond_signal(&msg_out);
  pthread_mutex_unlock(&mutex);
}

void swap(double *a, double *b) {
    double tmp = *a;
    *a = *b;
    *b = tmp;
}



int partition(double *array, int size)
{
    int first = 0;
    int middle = size / 2;
    int last = size - 1;

    if (array[first] > array[middle])
    {
        swap(array + first, array + middle);
    }

    if (array[middle] > array[last])
    {
        swap(array + middle, array + last);
    }

    if (array[first] > array[middle])
    {
        swap(array + first, array + middle);
    }

    double pivot = array[middle];
    int i, j;
    for (i = 1, j = size - 2 ;; i++, j--)
    {

        while (array[i] < pivot)
          i++;
        while (array[j] > pivot)
          j--;
        if (i >= j)
          break;

        swap(array + i, array + j);
    }

    return i;
}



void QuickSort(double *array, int start, int end)
{
    int partition_split = partition(array + start , end - start);


    send(WORK, start, start + partition_split);
    send(WORK, start + partition_split, end);
}



void inssort(double *array, int size)
{
    int j;

    for (int i = 1; i < size; i++)
    {
        j=i;

        while (j > 0 && array[j-1] > array[j])
        {
            swap(array + j, array + (j - 1));
            j--;
        }
    }
}



void *thread_func(void *params)
{
    double *array = (double*) params;

    int type, start, end;

    receive(&type, &start, &end);



    while (type != STOP)
    {
        if(type == FINISH)
        {
            printf("Finish\n");
            send(FINISH, start, end);
        }
        else if (type == WORK)
        {

          if ( (end - start) <= THRESHOLD)
          {
              printf("Starting inssort()\n");
              inssort(array + start, end - start);
              send(FINISH, start, end);
          }
          else
          {
            printf("Starting QuicSort()\n");
            QuickSort(array, start, end);
          }

        }

        receive(&type, &start, &end);

    }

    printf("Shutdown\n");
    send(STOP, 0, 0);

    pthread_exit(NULL);
}



int main()
{
    double *array = (double*) malloc(sizeof(double) * ARRAY_LENGTH);    

    if (!array)
    {
        printf("Error while allocation memory\n");
        exit(-1);
    }



    for(int i = 0; i < ARRAY_LENGTH; i++)
    {
        array[i] = (double) rand() / RAND_MAX;
    }



    pthread_t thread_pool[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++)
    {
      pthread_create(&thread_pool[i], NULL, thread_func, array);
    }


    send(WORK, 0, ARRAY_LENGTH);



    int type, start, end;
    int messages = 0;

    receive(&type, &start, &end);

    while (1)
    {
        if (type == FINISH)
        {
            messages += end - start;
            printf("Messages completed: %d / %d\n", messages, ARRAY_LENGTH);

            if(messages == ARRAY_LENGTH)
              break;
        }
        else
        {
            send(type, start, end);
        }

        receive(&type, &start, &end);
    }

    send(STOP, 0, 0);


    bool success_check = true;
    for (int i = 0; i < ARRAY_LENGTH - 1; i++)
    {
        if (array[i] > array[i+1])
        {
            printf("Error these elements are not sorted: [%d] = %f > [%d] = %f\n", i, array[i], i + 1, array[i+1]);
            success_check = false;
            break;
        }
    }

    if(success_check == true)
    {
      printf("SUCCESS array is sorted\n");
    }


    for(int i = 0; i < NUM_THREADS;i++)
    {
       pthread_join(my_thread[i], NULL);
    }

    free(array);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&msg_in);
    pthread_cond_destroy(&msg_out);

    return 0;
}
