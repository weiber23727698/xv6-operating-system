#include "kernel/types.h"
#include "user/setjmp.h"
#include "user/threads.h"
#include "user/user.h"
#define NULL 0

static struct thread* current_thread = NULL;
static struct thread* root_thread = NULL;
static int id = 1;
static jmp_buf env_st;

struct thread* find_last(struct thread* t){
    struct thread* now = t;
    while(!(now->right==NULL && now->left==NULL)){
        if(now->right != NULL)
            now = now->right;
        else if(now->left != NULL)
            now = now->left;
    }
    return now;
}

struct thread *thread_create(void (*f)(void *), void *arg){
    struct thread *t = (struct thread*) malloc(sizeof(struct thread));
    //unsigned long stack_p = 0;
    unsigned long new_stack_p;
    unsigned long new_stack;
    new_stack = (unsigned long) malloc(sizeof(unsigned long)*0x100);
    new_stack_p = new_stack +0x100*8-0x2*8;
    t->fp = f;//it is a function for the created thread
    t->arg = arg;
    t->ID  = id;
    t->buf_set = 0; //1: indicate jmp_buf has been set, 0: indicate jmp_buf not set
    t->stack = (void*) new_stack;
    t->stack_p = (void*) new_stack_p;
    id++;
    return t; //return the initialized structure
}
void thread_add_runqueue(struct thread *t){
    t->right = NULL, t->left = NULL;
    if(current_thread == NULL)
        root_thread = t, current_thread = t;
    else{
        if(current_thread->left == NULL){
            current_thread->left = t;
            t->parent = current_thread;
        }
        else if(current_thread->right == NULL){
            current_thread->right = t;
            t->parent = current_thread;
        }
        else{
            free(t->stack);
            free(t);
        }
    }
}
void thread_yield(void){
    if(setjmp(current_thread->env) == 0){
        schedule();
        dispatch();
    }
}
void dispatch(void){
    if(current_thread->buf_set == 0){// first access
        if(setjmp(current_thread->env) == 0){
            current_thread->env->sp = (unsigned long)current_thread->stack_p;
            longjmp(current_thread->env, 1);
        }
        current_thread->buf_set = 1;
        current_thread->fp(current_thread->arg);// call the task function
        thread_exit();
    }
    longjmp(current_thread->env, 1);
}
void schedule(void){
    if(current_thread->left != NULL)
        current_thread  = current_thread->left;
    else{
        if(current_thread->right != NULL)
            current_thread = current_thread->right;
        else{
            struct thread* now = current_thread;
            while(now != root_thread){
                if(now->parent->left==now && now->parent->right!=NULL){
                    now = now->parent->right;
                    break;
                }
                else
                    now  = now->parent;
            }
            current_thread = now;
        }
    }
}
void thread_exit(void){
    if(current_thread == root_thread && current_thread->left == NULL && current_thread->right == NULL){
        free(current_thread->stack);
        free(current_thread);
        longjmp(env_st, 1);
    }
    else{
        struct thread* t = current_thread;
        if(t->left==NULL && t->right==NULL){
            schedule();
            if(t == t->parent->right)
                t->parent->right = NULL;
            else if(t== t->parent->left)
                t->parent->left = NULL;
        }
        else{
            struct thread* last = find_last(t);
            if(last == last->parent->right)
                last->parent->right = NULL;
            else if(last == last->parent->left)
                last->parent->left = NULL;
            if(t == t->parent->right)
                t->parent->right = last;
            else if(t == t->parent->left)
                t->parent->left = last;
            last->left = t->left, last->right = t->right, last->parent = t->parent;
            if(t->left != NULL)
                t->left->parent = last;
            if(t->right != NULL)
                t->right->parent = last;
            if(t == root_thread)
                root_thread = last;
            current_thread = last;
            schedule();
        }
        free(t->stack);
        free(t);
        dispatch();
    }
    
}
void thread_start_threading(void){
    if(setjmp(env_st) == 0)
        dispatch();
}

