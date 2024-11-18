// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS 4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */

pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_mutex_t mutex3;

/* TODO: Define graph task argument. */
typedef struct {
	os_graph_t *graph;
	unsigned int node_idx;
} graph_task_t;

static void process_node(unsigned int idx)
{
	os_node_t *node = graph->nodes[idx];

	pthread_mutex_lock(&mutex);
	if (graph->visited[idx] != DONE) {
		sum += node->info;
		graph->visited[idx] = DONE;
	}
	pthread_mutex_unlock(&mutex);

	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		pthread_mutex_lock(&mutex);
		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			struct dataArgs data;

			data.nodeIdx = node->neighbours[idx];
			struct dataArgs *data_ptr = malloc(sizeof(struct dataArgs));

			*data_ptr = data;
			graph->visited[node->neighbours[i]] = PROCESSING;
			os_task_t *task = create_task((void *)process_node, data_ptr, free);

			enqueue_task(tp, task);
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);
	}
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	pthread_mutex_init(&mutex, NULL);
	tp = create_threadpool(NUM_THREADS);
	for (int i = 0; i < graph->num_nodes; i++) {
		if (graph->visited[i] != DONE)
			process_node(i);
	}

	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	return 0;
}
