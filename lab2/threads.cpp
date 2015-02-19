
int THREADS = 10;
sem_w work_mutex;

struct thread_info {
  int thread_id;
}

void* serve(void* in_data){
  struct thread_info* t_info = (struct thread_info*) in_data;
  int tid = t_info->thread_id;

  while(1){
    sem_wait(&work_mutex);
    cout << "I am thread: " << tid << "\n";
    sem_post(&work_mutex);
  }
}

int main(int argc, argv[])
{
  sem_init(&work_mutex, 0, 1);

  pthread_t threads [THREADS];

  struct thread_info all_thread_info[THREADS];

  for(int i=0; i < THREADS; i++)
    {
      sem_wait( &work_mutex) ;

      all_thread_ifno[i].thread_id = i; // setting the id
      pthread_create( &threads[i], NULL, serve, (void*) &all_thread_info[i]);
    }
}
