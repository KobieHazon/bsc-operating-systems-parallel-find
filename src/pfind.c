#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/queue.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define NOT_DOT(str) strcmp(str, ".") && strcmp(str, "..") //Ignore . .. files
#define DONE_STRING "Done searching, found %d files\n"
#define SIGINT_STRING "Search stopped, found %d files\n"
#define THREAD_ERROR_STRING "Error: Thread encounterd error, exiting thread.\n"
#define IS_ERROR 1

static pthread_t* thread_ids; //Array for storing thread ids
static pthread_mutex_t queue_lock;
static pthread_cond_t wait_cond;
static TAILQ_HEAD(tailhead, dir_entry) dir_head; //The head of the queue
static char* search_term; //What we search for
static char** file_paths; //Array of different current file_paths
static int thread_num, awake_num, err_num, found_files, is_SIGINT;
static long created; //To use if SIGINT before launching all threads

struct dir_entry {
	char* dir_path; //Data of queue
	TAILQ_ENTRY(dir_entry) entries; //Need because of TAILQ implementation
};

/*
 * int is_empty():
 * 	Returns true iff the directory queue is empty
*/
int is_empty() {
	return dir_head.tqh_first == NULL; 
}

/*
 * char* create_file_path:
 * 	Returns pointer to string with the path of new file (or directory)
 * 	Allocates more memory if needed.
*/
char* create_file_path(char* old_path, const char *dir_path, const char* file_name) {
	if ((old_path = realloc(old_path, strlen(dir_path) + strlen(file_name) + 2)) == NULL) 
		return 0;
	strcpy(old_path, dir_path);
	strcat(old_path, "/");
	strcat(old_path, file_name);
	return old_path;
}

/*
 * char* create_file_path:
 * 	Returns pointer to string with the path of new file (or directory)
 * 	Allocates more memory if needed.
*/
void free_dir_entry(struct dir_entry* entry) {
	if (entry)
		free(entry->dir_path);
	free(entry);
}

/*
 * void handle_sigint(int sig, siginfo_t *siginfo, void *context):
 * 	Signal handler for SIGINT, registerd with sigaction in the main thread.
 * 	It exits inside this function only if it is not the main thread
 *	so the main thread can still join the threads, free memory and print
 * 	what it needs to.
*/
void handle_sigint(int sig, siginfo_t *siginfo, void *context) {
    is_SIGINT = 1;
    pthread_mutex_lock(&queue_lock);
    pthread_cond_broadcast(&wait_cond);
    pthread_mutex_trylock(&queue_lock);
    pthread_mutex_unlock(&queue_lock);
    if (thread_ids[thread_num] != pthread_self())
		pthread_exit(NULL);
	return;
}

/*
 * void exit_thread(void* to_free, int is_error):
 * 	Responsible for exiting thread, thus unlocks the lock incase the
 * 	thread holds it. If a thread error print is needed it prints it 
 * 	and it frees the string of file_path.
*/
void exit_thread(void* to_free, int is_error) {
	pthread_mutex_trylock(&queue_lock);
	pthread_mutex_unlock(&queue_lock);
	if (is_error) {
		fprintf(stderr, THREAD_ERROR_STRING);
		__sync_fetch_and_add(&err_num, 1);
	}
	free(to_free);
	pthread_exit(NULL);
}

/*
 * int add_dir(const char* path):
 * 	Adds the directory specified by path to the global TAILQ queue
 * 	it is also a checking point for SIGINT (and may use pthread_cancel)
 *  and it locks the mutex when adding the directory.
 * 	Return true if succeeded.
*/
int add_dir(const char* path) {
	struct dir_entry *elem;
	if ((pthread_mutex_lock(&queue_lock)))
		return 0;
	if (is_SIGINT)
		exit_thread(NULL, !IS_ERROR);
	if ((elem = malloc(sizeof(struct dir_entry))) == NULL) {
		pthread_mutex_trylock(&queue_lock);
		pthread_mutex_unlock(&queue_lock);
		return 0;
	}
	if ((elem->dir_path = malloc((strlen(path) + 1))) == NULL) {
		free(elem);
		pthread_mutex_trylock(&queue_lock);
		pthread_mutex_unlock(&queue_lock);
		return 0;
	}
	strcpy(elem->dir_path, path);
	TAILQ_INSERT_HEAD(&dir_head, elem, entries);
	pthread_cond_signal(&wait_cond);
	pthread_mutex_trylock(&queue_lock);
	if ((pthread_mutex_unlock(&queue_lock))) {
		free_dir_entry(elem);
		return 0;
	}
	return 1;
}

/*
 * struct dir_entry *pop_dir(int* err_pop):
 * 	Pops a directory entry from the global queue and returns the 
 * 	appropriate dir_entry (the struct the queue is made of).
 * 	It locks the mutex when popping and it invokes wait if the queue is 
 * 	empty.
 * 	It is also a checking point for SIGINT and thus may invoke
 * 	pthread_cancel.
*/
struct dir_entry *pop_dir(int* err_pop) {
	struct dir_entry *ret;
	if (pthread_mutex_lock(&queue_lock))
		return NULL;
	if (is_SIGINT) 
		exit_thread(NULL, !IS_ERROR);
	
	while (is_empty()) {
		if (awake_num == 1) {
 			pthread_cond_broadcast(&wait_cond);
 			pthread_mutex_trylock(&queue_lock);
			pthread_mutex_unlock(&queue_lock);
			return NULL;
		}
		awake_num--;
		pthread_cond_wait(&wait_cond, &queue_lock);
		if (is_SIGINT) 
			exit_thread(NULL, !IS_ERROR);
		if (awake_num == 1) {
			pthread_mutex_trylock(&queue_lock);
			if (pthread_mutex_unlock(&queue_lock))
				*err_pop = 1;
			return NULL;
		}
		awake_num++;
	}
	ret = dir_head.tqh_first;
	TAILQ_REMOVE(&dir_head, dir_head.tqh_first, entries);
	pthread_mutex_trylock(&queue_lock);
	if ((pthread_mutex_unlock(&queue_lock)))
		return NULL;
	return ret;
}

/*
 * void *search_thread(void* input):
 * 	The main function for all searching threads, the input is used 
 * 	to associate a file_path to be the thread's file_path string.
 * 	We never use the return value so always return NULL with pthread_exit.
 * 	The flow of this function is just as specified in the given pdf.
*/
void *search_thread(void* input) {
	struct dir_entry* entry; //for queue pop
    struct dirent *dp; //for iterating through directory
    DIR* search_dir; //for opening directory before iterating
    struct stat dir_stat; //for checking file type
	char* file_path = file_paths[(int)(long)input]; //for calculating new path
	int err_pop = 0;
	while (1) {
		if ((entry = pop_dir(&err_pop)) == NULL) {
			if (err_pop)
				exit_thread(file_path, IS_ERROR);
			else
				exit_thread(file_path, !IS_ERROR);
		}
		if ((search_dir = opendir(entry->dir_path)) == NULL) 
			exit_thread(file_path, IS_ERROR);
		while ((dp = readdir(search_dir)) != NULL) {
			if (NOT_DOT(dp->d_name)) {
				file_path = create_file_path(file_path, 
				entry->dir_path, dp->d_name);
				stat(file_path, &dir_stat);
				if (S_ISDIR(dir_stat.st_mode)) {
					if (!add_dir(file_path))
						exit_thread(file_path, IS_ERROR);
				}
				else if (strstr(dp->d_name, search_term) != NULL) {
					printf("%s\n", file_path);
					__sync_fetch_and_add(&found_files, 1);
				}
			}
		}
		free_dir_entry(entry);
		closedir(search_dir);
	}
	exit_thread(file_path, !IS_ERROR);
	return NULL;
}

/*
 * int main(int argc, char *argv[]):
 * 	The function the main thread runs. This function allocates memory
 * 	for the structures we use in our code and initiallizes them.
 * 	Afterwards, it creates the searching threads and waits for their exit.
 * 	When all launched threads are joined with the main thread, we free
 * 	all the memory that was allocated, print according to every case
 *	and exit safely with the correct return value;
*/
int main(int argc, char *argv[]) {
	struct stat path_dir;
	int error_check;
	long int i;
	struct sigaction act_sigint;
	
	if (argc != 4)
		exit(IS_ERROR);
	if (stat(argv[1], &path_dir) < 0 || !S_ISDIR(path_dir.st_mode))
		exit(IS_ERROR);
	if (argv[3][0] == '-' || (thread_num = atoi(argv[3])) == 0 ||
	thread_num == 0) 
		exit(IS_ERROR);
	
	memset(&act_sigint, '\0', sizeof(act_sigint));
    act_sigint.sa_sigaction = &handle_sigint;
    act_sigint.sa_flags = SA_SIGINFO;
    if (sigaction(SIGINT, &act_sigint, NULL) < 0)
		exit(IS_ERROR);
	TAILQ_INIT(&dir_head);
	if (!add_dir(argv[1]))
		exit(IS_ERROR);
	if ((thread_ids = malloc(sizeof(pthread_t) * (thread_num + 1))) == NULL)
		exit(IS_ERROR);
	thread_ids[thread_num] = pthread_self();
	if ((search_term = strdup(argv[2])) == NULL)
		exit(IS_ERROR);
	if ((file_paths = calloc(thread_num, sizeof(char*))) == NULL)
		exit(IS_ERROR);
	for (i = 0; i < thread_num; i++)
		file_paths[i] = malloc(1);
	awake_num = thread_num;
	if ((error_check = pthread_mutex_init(&queue_lock, NULL)))
		exit(IS_ERROR);
	if (pthread_cond_init(&wait_cond, NULL))
		exit(IS_ERROR);
		
	for(created = 0; created < thread_num; created++) {
		if ((error_check = pthread_create(&thread_ids[created], NULL, search_thread, (void *)created))) 
			exit(IS_ERROR);
		if (is_SIGINT)
			break;
	}	
	for(i = 0;i < created; i++) 
		if ((error_check = pthread_join(thread_ids[i], NULL))) 
			exit(IS_ERROR);
	if (pthread_cond_destroy(&wait_cond)) 
		exit(IS_ERROR);
		
	if (!is_SIGINT)
		printf(DONE_STRING, found_files);
	else
		printf(SIGINT_STRING, found_files);
		
	if (pthread_mutex_destroy(&queue_lock))
		exit(err_num == thread_num);
	free(thread_ids);
	free(search_term);
	free(file_paths);
	exit(err_num == thread_num);
}
