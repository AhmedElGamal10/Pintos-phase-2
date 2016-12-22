#include "priority_queue.h"
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

//======================== helper functions ===================================

int get_left_child(struct priority_queue* p_queue, int parent);

int get_right_child(struct priority_queue* p_queue, int parent);

int is_leaf(struct priority_queue* p_queue, int node);

void swap(struct priority_queue* p_queue, int node1, int node2);

int get_parent(struct priority_queue* p_queue, int node);

void push_priority_queue_helper(struct priority_queue* p_queue, int node);

void max_heapify(struct priority_queue* p_queue, int node);

//================= end helper functions


struct priority_queue* init_priority_queue(int max_size, compare_func* compare_func)
{
    struct priority_queue* p_queue = (struct priority_queue*) malloc(sizeof(struct priority_queue));

    /*
        priority queue initialization
    */
    p_queue->max_size = max_size;

    //# of elements in queue
    p_queue->count = 0;

    //queue compare function
    p_queue->compare_func = compare_func;

    //allocate heap array
    p_queue->heap_arr = (void*) malloc(sizeof(void*) * max_size);

    return p_queue;
}

void push_priority_queue(struct priority_queue* p_queue, void* element)
{
    if (p_queue->count >= p_queue->max_size) { //max size reached
        return;
    }
    //update heap array
    p_queue->heap_arr[p_queue->count] = element;
    p_queue->count++;

    //traverse the heap upwards
    push_priority_queue_helper(p_queue, p_queue->count - 1);
}

void* pop_priority_queue(struct priority_queue* p_queue)
{
    if (p_queue->count == 0) {
        //heap is empty
        return NULL;
    }
    //value that should be returned
    void* max_value = p_queue->heap_arr[0];

    p_queue->count--;
    //exchange the first and last elements
    p_queue->heap_arr[0] = p_queue->heap_arr[p_queue->count];


    if (p_queue->count > 0) {
        //restore heap property
        max_heapify(p_queue, 0);
    }
    return max_value;
}

void* display_priority_queue(struct priority_queue* p_queue)
{
    if (p_queue->count == 0) { //heap is empty
        return NULL;
    }

    //maximum element in the queue.
    return p_queue->heap_arr[0];
}

int size_priority_queue(struct priority_queue* p_queue)
{
    //number of elements in the queue.
    return p_queue->count;
}

void make_heap(struct priority_queue* p_queue)
{
    int current_node = p_queue->count - 1;
    for (; current_node >= 0; current_node--) {
        max_heapify(p_queue, current_node);
    }
}


//========================= helper functions; implamantation


/*
    returns index of left child
*/
int get_left_child(struct priority_queue* p_queue, int parent)
{
    //formulae for the left child
    int left_child = parent * 2 + 1;
    if (left_child >= p_queue->count) {//formulae for the left child
        return -1;
    }

    //left child index
    return left_child;
}

/*
    returns index of left child
*/
int get_right_child(struct priority_queue* p_queue, int parent)
{
    //formulae for the right child
    int right_child = parent * 2 + 2;
    if (right_child >= p_queue->count) {//no right child
        return -1;
    }

    //right child index
    return right_child;
}

/*
    test if the provided node is a leaf node.
*/
int is_leaf(struct priority_queue* p_queue, int node)
{
    //formulae for the left child
    int left_child = node * 2 + 1;
    if (left_child >= p_queue->count) {//leaf node
        return TRUE;
    }
    return FALSE;
}

/*
    swaps two elements.
*/
void swap(struct priority_queue* p_queue, int node1, int node2)
{
    void* temp = p_queue->heap_arr[node1];
    p_queue->heap_arr[node1] = p_queue->heap_arr[node2];
    p_queue->heap_arr[node2] = temp;
}

int get_parent(struct priority_queue* p_queue, int node)
{
    if (node == 0) {
        //node is the root
        return -1;
    }

    //formulae for the parent node
    int parent = (node - 1) / 2;
    return parent;
}

void max_heapify(struct priority_queue* p_queue, int node)
{
    if (is_leaf(p_queue, node)) {
        //nothing to do.
        return;
    }
    int left_child = get_left_child(p_queue, node);
    int right_child = get_right_child(p_queue, node);

    //get the max of the two children
    int max_child = -1;
    if (right_child != -1
            && p_queue->compare_func(p_queue->heap_arr[right_child],
                                     p_queue->heap_arr[left_child]) > 0) {
        max_child = right_child;
    } else {
        max_child = left_child;
    }

    if (p_queue->compare_func(p_queue->heap_arr[node],
                              p_queue->heap_arr[max_child]) < 0) {
        swap(p_queue, node, max_child);
        max_heapify(p_queue, max_child);
    }

}


/*
    push given node to given queue recursively.
*/
void push_priority_queue_helper(struct priority_queue* p_queue, int node)
{
    int parent = get_parent(p_queue, node);
    if (parent == -1) { // node is root
        return;
    }
    if (p_queue->compare_func(p_queue->heap_arr[parent],
                              p_queue->heap_arr[node]) < 0) {
        //parent is smaller than the node
        swap(p_queue, parent, node);
        push_priority_queue_helper(p_queue, parent);
    }
}


