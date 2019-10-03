

#include "threadpool.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>


threadpool* create_threadpool(int num_threads_in_pool)
{
    if (num_threads_in_pool <= 0 || num_threads_in_pool > MAXT_IN_POOL) 
    {
        printf("Usage: threadpool <pool-size> <max-number-of-jobs>\n");
        return NULL;
    }
    
    threadpool* tp = (threadpool*)calloc(1, sizeof(threadpool));
    if (tp == NULL)
    {
        perror("calloc failed");
        return NULL;
    }

    tp->threads = (pthread_t*)calloc(num_threads_in_pool, sizeof(pthread_t));
    if (tp->threads == NULL)
    {
        free(tp);
        perror("calloc failed");
        return NULL;
    }
    
    tp->num_threads = num_threads_in_pool;
    tp->qsize = 0;
    tp->qhead = NULL;
    tp->qtail = NULL;
    pthread_mutex_init(&tp->qlock, NULL);
    pthread_cond_init(&tp->q_empty, NULL);
    pthread_cond_init(&tp->q_not_empty, NULL);
    tp->shutdown = 0;
    tp->dont_accept = 0;
    
    for(int i = 0; i < num_threads_in_pool; i++)
    {
        pthread_create(&tp->threads[i], NULL, do_work, tp);
    }
    
    return tp;
}

void* do_work(void* p)
{
    if (p == NULL)
    {
        return NULL;
    }

    threadpool* tp = (threadpool*)p;
    
    while(1)
    {
        pthread_mutex_lock(&tp->qlock);
        
        if (tp->shutdown == 1)
        {
            pthread_mutex_unlock(&tp->qlock);
            return NULL;
        }

        if (tp->qsize == 0)
        {
            pthread_cond_wait(&tp->q_not_empty, &tp->qlock);
            if (tp->shutdown == 1)
            {
                pthread_mutex_unlock(&tp->qlock);
                return NULL;
            }
        }

        work_t* work = tp->qhead;
        if (work == NULL)
        {
            pthread_mutex_unlock(&tp->qlock);
            continue;
        }

        tp->qsize--;
        tp->qhead = tp->qhead->next;    
        pthread_mutex_unlock(&tp->qlock);
        work->routine(work->arg);
        free(work);
        
        if (tp->qsize == 0 && tp->dont_accept == 1) 
        {
            pthread_cond_signal(&tp->q_empty);
        }     
    }
}

void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg)
{
    pthread_mutex_lock(&from_me->qlock);
    
    if (from_me->dont_accept == 0) 
    {
        work_t* work = (work_t*)calloc(1, sizeof(work_t));
        work->routine = dispatch_to_here;
        work->arg = arg;
        work->next = NULL;
        from_me->qsize++;
        
        if (from_me->qhead == NULL) 
        {
            from_me->qhead = work;
            from_me->qtail = work;
        }
        
        else
        {
            from_me->qtail->next = work;
            from_me->qtail = work;
        } 
        
        pthread_mutex_unlock(&from_me->qlock);
        pthread_cond_signal(&from_me->q_not_empty);
    }

    else
    {
        pthread_mutex_unlock(&from_me->qlock);
        return;
    } 
}

void destroy_threadpool(threadpool* destroyme)
{
    pthread_mutex_lock(&destroyme->qlock);
    destroyme->dont_accept = 1;
    
    if (destroyme->qsize > 0) 
    {
        pthread_cond_wait(&destroyme->q_empty, &destroyme->qlock);
    }

    destroyme->shutdown = 1;

    pthread_mutex_unlock(&destroyme->qlock);
    pthread_cond_broadcast(&destroyme->q_not_empty);
    
    for(int i = 0; i < destroyme->num_threads; i++)
    {
        pthread_join(destroyme->threads[i], NULL);
    }

    pthread_mutex_destroy(&destroyme->qlock);
    pthread_cond_destroy(&destroyme->q_empty);
    pthread_cond_destroy(&destroyme->q_not_empty);

    free(destroyme->threads);
    free(destroyme);    
}