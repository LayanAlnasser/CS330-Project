#define _TIMESPEC_DEFINED

#include <stdint.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <pthread.h>

#include <time.h>

 

#define MAX_TITLE   128

#define MAX_SUMMARY 256

#define MAX_BOOKS   1000

#define MAX_OPS     1000

 

typedef struct {

    int id;

    char title[MAX_TITLE];

    char summary[MAX_SUMMARY];

    int availability;

    int read_count;

    int update_count;

    pthread_rwlock_t rwlock;

    pthread_mutex_t count_mutex;

} Book;

 

typedef enum {

    OP_READ,

    OP_UPDATE,

    OP_ADD,

    OP_REMOVE

} OperationType;

 

typedef struct {

    OperationType type;

    int book_id;

} Operation;

 

Book catalog[MAX_BOOKS];

int book_count = 0;

 

pthread_rwlock_t catalog_rwlock = PTHREAD_RWLOCK_INITIALIZER;

 

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

 

int num_students = 0;

int num_librarians = 0;

 

Operation student_ops[MAX_OPS];

int student_op_count = 0;

int next_student_op = 0;

pthread_mutex_t student_op_mutex = PTHREAD_MUTEX_INITIALIZER;

 

Operation librarian_ops[MAX_OPS];

int librarian_op_count = 0;

int next_librarian_op = 0;

pthread_mutex_t librarian_op_mutex = PTHREAD_MUTEX_INITIALIZER;

 

void log_event(const char *thread_name, const char *action) {

    pthread_mutex_lock(&log_mutex);

 

    time_t now = time(NULL);

    struct tm *t = localtime(&now);

    char time_buf[32];

    if (t != NULL) {

        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", t);

    } else {

        snprintf(time_buf, sizeof(time_buf), "00:00:00");

    }

 

    printf("[%s] %s %s\n", time_buf, thread_name, action);

    fflush(stdout);

 

    pthread_mutex_unlock(&log_mutex);

}

 

Book *find_book_nolock(int id) {

    for (int i = 0; i < book_count; ++i) {

        if (catalog[i].id == id) {

            return &catalog[i];

        }

    }

    return NULL;

}

 

void create_book_internal(int id, const char *title, const char *summary, int available) {

    if (book_count >= MAX_BOOKS) {

        return;

    }

    Book *b = &catalog[book_count++];

    b->id = id;

    strncpy(b->title, title, MAX_TITLE - 1);

    b->title[MAX_TITLE - 1] = '\0';

    strncpy(b->summary, summary, MAX_SUMMARY - 1);

    b->summary[MAX_SUMMARY - 1] = '\0';

    b->availability = available;

    b->read_count = 0;

    b->update_count = 0;

    pthread_rwlock_init(&b->rwlock, NULL);

    pthread_mutex_init(&b->count_mutex, NULL);

}

 

void init_catalog() {

    pthread_rwlock_wrlock(&catalog_rwlock);

 

    book_count = 0;

    create_book_internal(1, "Operating Systems", "Intro to OS concepts", 1);

    create_book_internal(2, "Concurrent Programming", "Threads and synchronization", 1);

    create_book_internal(3, "Database Systems", "Relational databases", 1);

 

    pthread_rwlock_unlock(&catalog_rwlock);

}

 

void execute_read(int thread_id, int book_id) {

    char name[32];

    snprintf(name, sizeof(name), "Student-%d", thread_id);

 

    pthread_rwlock_rdlock(&catalog_rwlock);

    Book *b = find_book_nolock(book_id);

    if (!b) {

        pthread_rwlock_unlock(&catalog_rwlock);

        char msg[128];

        snprintf(msg, sizeof(msg), "failed to read Book-%d (not found)", book_id);

        log_event(name, msg);

        return;

    }

 

    char msg[128];

    snprintf(msg, sizeof(msg), "wants to read Book-%d", b->id);

    log_event(name, msg);

 

    if (pthread_rwlock_tryrdlock(&b->rwlock) != 0) {

        snprintf(msg, sizeof(msg), "waiting to read Book-%d (lock busy)", b->id);

        log_event(name, msg);

        pthread_rwlock_rdlock(&b->rwlock);

    } else {

        snprintf(msg, sizeof(msg), "acquired read lock for Book-%d", b->id);

        log_event(name, msg);

    }

 

    snprintf(msg, sizeof(msg), "started reading Book-%d", b->id);

    log_event(name, msg);

 

    pthread_mutex_lock(&b->count_mutex);

    b->read_count++;

    pthread_mutex_unlock(&b->count_mutex);

 

    snprintf(msg, sizeof(msg), "finished reading Book-%d", b->id);

    log_event(name, msg);

 

    pthread_rwlock_unlock(&b->rwlock);

    pthread_rwlock_unlock(&catalog_rwlock);

}

 

void execute_update(int thread_id, int book_id) {

    char name[32];

    snprintf(name, sizeof(name), "Librarian-%d", thread_id);

 

    pthread_rwlock_rdlock(&catalog_rwlock);

    Book *b = find_book_nolock(book_id);

    if (!b) {

        pthread_rwlock_unlock(&catalog_rwlock);

        char msg[128];

        snprintf(msg, sizeof(msg), "failed to update Book-%d (not found)", book_id);

        log_event(name, msg);

        return;

    }

 

    char msg[128];

    snprintf(msg, sizeof(msg), "wants to update Book-%d", b->id);

    log_event(name, msg);

 

    if (pthread_rwlock_trywrlock(&b->rwlock) != 0) {

        snprintf(msg, sizeof(msg), "waiting to update Book-%d (lock busy)", b->id);

        log_event(name, msg);

        pthread_rwlock_wrlock(&b->rwlock);

    } else {

        snprintf(msg, sizeof(msg), "acquired write lock for Book-%d", b->id);

        log_event(name, msg);

    }

 

    snprintf(b->summary, MAX_SUMMARY, "Updated summary by librarian %d", thread_id);

    b->availability = 1;

 

    pthread_mutex_lock(&b->count_mutex);

    b->update_count++;

    pthread_mutex_unlock(&b->count_mutex);

 

    snprintf(msg, sizeof(msg), "finished updating Book-%d", b->id);

    log_event(name, msg);

 

    pthread_rwlock_unlock(&b->rwlock);

    pthread_rwlock_unlock(&catalog_rwlock);

}

 

void execute_add(int thread_id) {

    char name[32];

    snprintf(name, sizeof(name), "Librarian-%d", thread_id);

 

    char msg[128];

    snprintf(msg, sizeof(msg), "wants to add a new book");

    log_event(name, msg);

 

    if (pthread_rwlock_trywrlock(&catalog_rwlock) != 0) {

        log_event(name, "waiting to add book (catalog lock busy)");

        pthread_rwlock_wrlock(&catalog_rwlock);

    } else {

        log_event(name, "acquired catalog write lock for add");

    }

 

    int new_id = (book_count == 0) ? 1 : (catalog[book_count - 1].id + 1);

    char title[MAX_TITLE];

    char summary[MAX_SUMMARY];

    snprintf(title, sizeof(title), "New Book %d", new_id);

    snprintf(summary, sizeof(summary), "Auto-generated book %d", new_id);

 

    create_book_internal(new_id, title, summary, 1);

 

    pthread_rwlock_unlock(&catalog_rwlock);

 

    snprintf(msg, sizeof(msg), "added Book-%d", new_id);

    log_event(name, msg);

}

 

void execute_remove(int thread_id, int book_id) {

    char name[32];

    snprintf(name, sizeof(name), "Librarian-%d", thread_id);

 

    char msg[128];

    snprintf(msg, sizeof(msg), "wants to remove Book-%d", book_id);

    log_event(name, msg);

 

    if (pthread_rwlock_trywrlock(&catalog_rwlock) != 0) {

        log_event(name, "waiting to remove book (catalog lock busy)");

        pthread_rwlock_wrlock(&catalog_rwlock);

    } else {

        log_event(name, "acquired catalog write lock for remove");

    }

 

    int index = -1;

    for (int i = 0; i < book_count; ++i) {

        if (catalog[i].id == book_id) {

            index = i;

            break;

        }

    }

 

    if (index == -1) {

        pthread_rwlock_unlock(&catalog_rwlock);

        snprintf(msg, sizeof(msg), "failed to remove Book-%d (not found)", book_id);

        log_event(name, msg);

        return;

    }

 

    pthread_rwlock_destroy(&catalog[index].rwlock);

    pthread_mutex_destroy(&catalog[index].count_mutex);

 

    for (int i = index; i < book_count - 1; ++i) {

        catalog[i] = catalog[i + 1];

    }

    book_count--;

 

    pthread_rwlock_unlock(&catalog_rwlock);

 

    snprintf(msg, sizeof(msg), "removed Book-%d", book_id);

    log_event(name, msg);

}

 

void *student_thread(void *arg) {

    int id = (int)(intptr_t)arg;

 

    while (1) {

        pthread_mutex_lock(&student_op_mutex);

        if (next_student_op >= student_op_count) {

            pthread_mutex_unlock(&student_op_mutex);

            break;

        }

        int index = next_student_op++;

        Operation op = student_ops[index];

        pthread_mutex_unlock(&student_op_mutex);

 

        execute_read(id, op.book_id);

    }

 

    return NULL;

}

 

void *librarian_thread(void *arg) {

    int id = (int)(intptr_t)arg;

 

    while (1) {

        pthread_mutex_lock(&librarian_op_mutex);

        if (next_librarian_op >= librarian_op_count) {

            pthread_mutex_unlock(&librarian_op_mutex);

            break;

        }

        int index = next_librarian_op++;

        Operation op = librarian_ops[index];

        pthread_mutex_unlock(&librarian_op_mutex);

 

        if (op.type == OP_UPDATE) {

            execute_update(id, op.book_id);

        } else if (op.type == OP_ADD) {

            execute_add(id);

        } else if (op.type == OP_REMOVE) {

            execute_remove(id, op.book_id);

        }

    }

 

    return NULL;

}

 

int load_config(const char *filename) {

    FILE *f = fopen(filename, "r");

    if (!f) {

        perror("Failed to open config file");

        return 0;

    }

 

    char line[256];

 

    while (fgets(line, sizeof(line), f)) {

        if (line[0] == '#' || strlen(line) < 2) {

            continue;

        }

 

        if (line[0] == 'S') {

            sscanf(line, "S %d", &num_students);

            continue;

        }

        if (line[0] == 'L') {

            sscanf(line, "L %d", &num_librarians);

            continue;

        }

 

        char op[16];

        int id;

 

        if (sscanf(line, "%15s %d", op, &id) == 2) {

            if (strcmp(op, "read") == 0) {

                if (student_op_count < MAX_OPS) {

                    student_ops[student_op_count].type = OP_READ;

                    student_ops[student_op_count].book_id = id;

                    student_op_count++;

                }

            } else if (strcmp(op, "update") == 0) {

                if (librarian_op_count < MAX_OPS) {

                    librarian_ops[librarian_op_count].type = OP_UPDATE;

                    librarian_ops[librarian_op_count].book_id = id;

                    librarian_op_count++;

                }

            } else if (strcmp(op, "remove") == 0) {

                if (librarian_op_count < MAX_OPS) {

                    librarian_ops[librarian_op_count].type = OP_REMOVE;

                    librarian_ops[librarian_op_count].book_id = id;

                    librarian_op_count++;

                }

            }

        } else if (sscanf(line, "%15s", op) == 1) {

            if (strcmp(op, "add") == 0) {

                if (librarian_op_count < MAX_OPS) {

                    librarian_ops[librarian_op_count].type = OP_ADD;

                    librarian_ops[librarian_op_count].book_id = -1;

                    librarian_op_count++;

                }

            }

        }

    }

 

    fclose(f);

    return 1;

}

 

int main(int argc, char *argv[]) {

    if (argc < 2) {

        fprintf(stderr, "Usage: %s config.txt\n", argv[0]);

        return 1;

    }

 

    if (!load_config(argv[1])) {

        return 1;

    }

 

    init_catalog();

 

    printf("Config: %d students, %d librarians, %d student ops, %d librarian ops\n",

           num_students, num_librarians, student_op_count, librarian_op_count);

 

    if (num_students > 100 || num_librarians > 100) {

        fprintf(stderr, "Too many threads requested.\n");

        return 1;

    }

 

    pthread_t student_threads[100];

    pthread_t librarian_threads[100];

 

    for (int i = 0; i < num_students; ++i) {

        pthread_create(&student_threads[i], NULL, student_thread, (void *)(intptr_t)(i + 1));

    }

 

    for (int i = 0; i < num_librarians; ++i) {

        pthread_create(&librarian_threads[i], NULL, librarian_thread, (void *)(intptr_t)(i + 1));

    }

 

    for (int i = 0; i < num_students; ++i) {

        if (num_students > 0) {

            pthread_join(student_threads[i], NULL);

        }

    }

 

    for (int i = 0; i < num_librarians; ++i) {

        if (num_librarians > 0) {

            pthread_join(librarian_threads[i], NULL);

        }

    }

 

    pthread_rwlock_rdlock(&catalog_rwlock);

    printf("\nFinal catalog state:\n");

    for (int i = 0; i < book_count; ++i) {

        Book *b = &catalog[i];

        printf("Book-%d: title=\"%s\", reads=%d, updates=%d\n",

               b->id, b->title, b->read_count, b->update_count);

    }

    pthread_rwlock_unlock(&catalog_rwlock);

 

    return 0;

}