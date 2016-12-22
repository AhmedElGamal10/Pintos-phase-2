#ifndef PRIORITY_QUEUE_H_INCLUDED
#define PRIORITY_QUEUE_H_INCLUDED

typedef int compare_func (void* element1, void* element2);

/**
    |----------|
    |          |
    | MAX HEAP |
    |          |
    |----------|
*/

struct priority_queue
{
    int max_size;                /** maximum size the queue can hold. */
    int count;                   /** # of elements in the queue. */
    void** heap_arr;             /** heap array. */

    /**
        takes two arguments from the type that the queue holds, as void pointers.

        return value is +ve : element1 > element2
                        -ve : element1 < element2
                        zero: element1 == element2
    */
    compare_func* compare_func;
};

/**
    priority queue initialization.

    @param max_size : max # of elements the queue can hold.
    @param compare_func : compare function.

    @return pointer to the generated priority queue.
*/
struct priority_queue* init_priority_queue(int max_size, compare_func* compare_func);


/**
    @param p_queue : priority queue pointer returned by init_priority_queue.
    @param element : element to push to that priority queue.

    pushes the given element to the qiven priority queue and maintains the heap property.
*/
void push_priority_queue(struct priority_queue* p_queue, void* element);


/**
    @param p_queue : priority queue pointer returned by init_priority_queue.

    @return maximum element in the queue.

    pops the maximum element in the queue and returns it.
    it maintains the heap property.
*/
void* pop_priority_queue(struct priority_queue* p_queue);


/**
    @param p_queue : priority queue pointer returned by init_priority_queue.

    @return maximum element in the priority queue.

    The given queue is not changed.
*/
void* display_priority_queue(struct priority_queue* p_queue);


/**
    @param p_queue : priority queue pointer returned by init_priority_queue.

    @return number of elements in the queue.
*/
int size_priority_queue(struct priority_queue* p_queue);

/**
    form a heap from the array */
void make_heap(struct priority_queue* p_queue);

#endif // PRIORITY_QUEUE_H_INCLUDED
