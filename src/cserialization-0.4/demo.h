/*
  demo.h
  C serialization library
  ulf.astrom@gmail.com / happyponyland.net

  This file contains demo code and test
  cases for the C serialization library. 
*/

#include <time.h>
#include <stdbool.h>

typedef struct demo_dir_t demo_dir_t;
typedef struct demo_leaf_t demo_leaf_t;
typedef struct demo_array_t demo_array_t;
typedef struct demo_ragged_t demo_ragged_t;
typedef struct demo_null_t demo_null_t;
typedef struct demo_maxint_t demo_maxint_t;
typedef struct demo_outer_t demo_outer_t;
typedef struct demo_outer2_t demo_outer2_t;
typedef struct demo_outer3_t demo_outer3_t;
typedef struct demo_outer4_t demo_outer4_t;
typedef struct demo_inner_t demo_inner_t;
typedef struct demo_stuff_t demo_stuff_t;
typedef struct demo_overlap_t demo_overlap_t;
typedef struct demo_float_t demo_float_t;
typedef struct demo_subst_t demo_subst_t;

struct demo_subst_t
{
  int array[5];
  int * somewhere;
  int * other_int;
  demo_subst_t * self;
};

struct demo_float_t
{
  float a;
  double b;
  long double c;
};

struct demo_overlap_t
{
  int a;
  demo_overlap_t * next;
  int * ptr;
};

struct demo_dir_t
{
  char * label;
  demo_dir_t * n_dir;
  demo_dir_t * b_dir;
  demo_leaf_t * f_leaf;
};

struct demo_leaf_t
{
  char letter;
  char * label;
  demo_leaf_t * n_leaf;
  int value;
};

struct demo_array_t
{
  int elements;
  short * data;
};

struct demo_ragged_t
{
  int rows;
  int ** data;
};

struct demo_null_t
{
  int * data;
};

struct demo_maxint_t
{
  short a;
  unsigned short b;
  int c;
  unsigned int d;
  long e;
  unsigned long f;
  size_t g;
};

struct demo_inner_t
{
  int c;
  int d;
};

struct demo_outer_t
{
  int a;
  int b;
  demo_inner_t inner[5];
};

struct demo_outer2_t
{
  demo_inner_t * inner[15];
};

struct demo_outer3_t
{
  size_t inner_count;
  demo_inner_t ** inner;
};

struct demo_outer4_t
{
  size_t inner_count;
  demo_inner_t * inner;
};

struct demo_stuff_t
{
  time_t time;
};

#define BSIZE 10
struct ring_buffer {
  int buffer[BSIZE];
  int *head, *tail;
}; 

void demo_del_leaf(demo_leaf_t * leaf);
void demo_del_dir(demo_dir_t * dir);
void demo_print(demo_dir_t * dir, int depth);
void test_static_arrays(void);
void test_dynamic_arrays(void);
void test_demo_tree(ser_tra_t * first_tra);
void test_circular_refs(ser_tra_t * first_tra);
void test_errors(ser_tra_t * first_tra);
void test_strings(void);
void list_type_sizes(void);
void test_subst_ptrs(void);
int char_array_null_term(ser_job_t * job, ser_tra_t * tra, void * thing);
demo_leaf_t * demo_make_leaves(int leaves, int * index);
demo_dir_t * demo_make_tree(int depth, int * index);
