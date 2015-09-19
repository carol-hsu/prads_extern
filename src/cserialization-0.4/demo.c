/*
  demo.c
  C serialization library
  ulf.astrom@gmail.com / happyponyland.net

  This file contains demo code and test
  cases for the C serialization library. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>

#include "serialize.h"
#include "demo.h"



char result[10000];



void demo_del_leaf(demo_leaf_t * leaf)
{
  if (!leaf)
    return;

  demo_del_leaf(leaf->n_leaf);
  free(leaf->label);
  free(leaf);
  return;
}


void demo_del_dir(demo_dir_t * dir)
{
  if (!dir)
    return;

  demo_del_dir(dir->n_dir);
  demo_del_leaf(dir->f_leaf);
  free(dir->label);
  free(dir);
  return;
}


void demo_print(demo_dir_t * dir, int depth)
{
  demo_leaf_t * leaf;
  int i;
  
  for (i = 0; i < depth; i++)
    printf("    ");

  printf("+ \"%s\"\n", dir->label);

  for (leaf = dir->f_leaf; leaf; leaf = leaf->n_leaf)
  {
    for (i = 0; i < depth + 1; i++)
      printf("    ");
    
    printf("* \"%s\" (value: %d, letter \'%c\')\n",
	   leaf->label, leaf->value, leaf->letter);
  }  

  if (dir->b_dir)
    demo_print(dir->b_dir, depth + 1);
  
  if (dir->n_dir)
    demo_print(dir->n_dir, depth);

  return;
}


demo_leaf_t * demo_make_leaves(int leaves, int * index)
{
  demo_leaf_t * ret;
  char tmp[100];

  if (leaves < 1)
    return NULL;

  ret = malloc(sizeof(demo_leaf_t));

  sprintf(tmp, "leaf %d", (*index)++);
  ret->label = strdup(tmp);
  ret->value = (*index) * 47;
  ret->letter = 'A' + (*index % 25);
  ret->n_leaf = demo_make_leaves(leaves - 1, index);

  return ret;
}



demo_dir_t * demo_make_tree(int depth, int * index)
{
  demo_dir_t * ret;
  char tmp[100];

  ret = malloc(sizeof(demo_dir_t));

  sprintf(tmp, "directory \"%d\"", (*index)++);
  ret->label = strdup(tmp);

  ret->n_dir = NULL;
  ret->f_leaf = NULL;
  ret->b_dir = NULL;

  if (depth - 1 > 0)
  {
    ret->f_leaf = demo_make_leaves(4 - depth, index);

    ret->n_dir = demo_make_tree(depth - 1, index);
    ret->b_dir = demo_make_tree(depth - 1, index);
  }

  return ret;
}



void test_static_arrays()
{
  unsigned int * shit;
  unsigned int * ret;
  int i;
  ser_tra_t * first_tra;
  ser_field_t * field;
  char * a;

  printf("\n== Static array serialization\nOriginal values: ");

  shit = malloc(sizeof(unsigned int) * 8);

  for (i = 0; i < 8; i++)
  {
    shit[i] = i * 2;
    printf("%d ", shit[i]);
  }

  printf("\n");

  first_tra = ser_new_tra("uint_array", sizeof(unsigned int) * 8, NULL);
  field = ser_new_field(first_tra, "uint", 0, "value", 0);
  field->repeat = 8;

  a = ser_ialize(first_tra, "uint_array", shit, NULL, SER_OPT_DEFAULT);
  printf("After ser_ialize():\n%s", a);

  ret = ser_parse(first_tra, "uint_array", a, &ser_def_log);
  printf("Values read back with ser_parse(): ");

  if (ret)
  {
    for (i = 0; i < field->repeat; i++)
      printf("%d ", ret[i]);
    
    printf("\n\n");
    free(ret);
  }
  else
  {
    printf("ERROR: Something broke. Sorry.\n");
  }

  ser_del_tra(first_tra);

  free(shit);

  return;
}


void test_dynamic_pointer_array()
{
  demo_inner_t ** orig;
  demo_inner_t ** ret;

  size_t i;

  ser_tra_t * first_tra;
  ser_tra_t * tra;
  ser_field_t * field;
  char * a;

  int elements;

  elements = 5;

  printf("\n== Dynamic pointer arrays\n");

  tra = first_tra = ser_new_tra("pointer_array", sizeof(demo_inner_t *) * elements, NULL);
  field = ser_new_field(tra, "inner_t", 1, "ptrs", 0);
  field->repeat = elements;

  tra = ser_new_tra("inner_t", sizeof(demo_inner_t), first_tra);
  ser_new_field(tra, "int", 0, "c", offsetof(demo_inner_t, c));
  ser_new_field(tra, "int", 0, "d", offsetof(demo_inner_t, c));

  orig = malloc(sizeof(demo_inner_t *) * elements);

  for (i = 0; i < elements; i++)
  {
    orig[i] = malloc(sizeof(demo_inner_t));
    orig[i]->c = i * 10;
    orig[i]->d = i * 10 + 5;
  }

  a = ser_ialize(first_tra, "pointer_array", orig, NULL, SER_OPT_COMPACT);

  printf("Serialized output:\n%s\n", a);

  for (i = 0; i < elements; i++)
    free(orig[i]);
  
  free(orig);

  ret = ser_parse(first_tra, "pointer_array", a, NULL);

  if (ret)
  {
    printf("Inflated again\n");
    
    for (i = 0; i < elements; i++)
      free(ret[i]);
    
    free(ret);
  }
  else
  {
    printf("ERROR: Something broke, sorry.\n");
  }

  free(a);

  ser_del_tra(first_tra);

  return;
}



void test_maxints()
{
  demo_maxint_t * ret;
  demo_maxint_t data;
  ser_tra_t * tra;
  char * a;
  bool success = false;

  printf("== Max int values\n"
	 "Serializes integer types with their largest possible values and verifies that "
	 "they are read back properly.\n");
  
  tra = ser_new_tra("maxint_test", sizeof(demo_maxint_t), NULL);
  ser_new_field(tra, "short",  0, "a", offsetof(demo_maxint_t, a));
  ser_new_field(tra, "ushort", 0, "b", offsetof(demo_maxint_t, b));
  ser_new_field(tra, "int",    0, "c", offsetof(demo_maxint_t, c));
  ser_new_field(tra, "uint",   0, "d", offsetof(demo_maxint_t, d));
  ser_new_field(tra, "long",   0, "e", offsetof(demo_maxint_t, e));
  ser_new_field(tra, "ulong",  0, "f", offsetof(demo_maxint_t, f));
  ser_new_field(tra, "size_t", 0, "g", offsetof(demo_maxint_t, g));

  data.a = SHRT_MIN;
  data.b = USHRT_MAX;
  data.c = INT_MIN;
  data.d = UINT_MAX;
  data.e = LONG_MIN;
  data.f = ULONG_MAX;
  data.g = -1;

  printf("a = %d\nb = %u\nc = %d\nd = %u\ne = %ld\nf = %lu\ng = %lu\n",
	 data.a, data.b, data.c, data.d, data.e, data.f, data.g);

  a = ser_ialize(tra, "maxint_test", &data, NULL, 0);

  printf("\nSerialized output:\n%s\n", a);

  ret = ser_parse(tra, "maxint_test", a, NULL);

  if (ret)
  {
    printf("Restored:\na = %d\nb = %u\nc = %d\nd = %u\ne = %ld\nf = %lu\ng = %lu\n\n",
	   ret->a, ret->b, ret->c, ret->d, ret->e, ret->f, ret->g);
    
    if (ret->a == data.a &&
	ret->b == data.b &&
	ret->c == data.c &&
	ret->d == data.d &&
	ret->e == data.e &&
	ret->f == data.f &&
	ret->g == data.g)
    {
      success = true;
    }

    free(ret);
  }

  free(a);

  ser_del_tra(tra);

  if (success)
    strcat(result, "PASSED: ");
  else
    strcat(result, "FAILED: ");

  strcat(result, "test_maxints\n");

  return;
}



void test_dynamic_arrays()
{
  demo_array_t * ret;
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  char * a;
  size_t i;

  bool success = false;

  printf("\n== Dynamic arrays\n"
	 "This will inflate a dynamic size array of short ints. "
	 "We will check that the number of values and the value themselves meet our expectations.\n");

  first_tra = ser_new_dyn_array("short*", "short", 0, sizeof(short), NULL);

  tra = ser_new_tra("dynamic_array", sizeof(demo_array_t), first_tra);
  ser_new_field(tra, "short", 0, "@data", offsetof(demo_array_t, elements));
  ser_new_field(tra, "short*", 1, "data", offsetof(demo_array_t, data));

  a = strdup("dynamic_array #1 { data #2; } short* #2 { 98,99,100,101 }");
  printf("Input data:\n  %s\n", a);

  ret = ser_parse(first_tra, "dynamic_array", a, &ser_def_log);
  free(a);
  
  printf("Values read back with ser_parse(): \n");
  printf("Got %d elements. \n", ret->elements);

  for (i = 0; i < ret->elements; i++)
    printf("%d ", ret->data[i]);

  if (ret->elements == 4 &&
      ret->data[0] == 98 &&
      ret->data[1] == 99 &&
      ret->data[2] == 100 &&
      ret->data[3] == 101)
  {
    success = true;
  }

  printf("\n\n");

  a = ser_ialize(first_tra, "dynamic_array", ret, NULL, SER_OPT_COMPACT);

  printf("Serialized again:\n%s\n\n", a);

  free(a);
  free(ret);

  ser_del_tra(first_tra);

  if (success)
    strcat(result, "PASSED: ");
  else
    strcat(result, "FAILED: ");

  strcat(result, "test_dynamic_arrays\n");

  return;
}


void test_ragged_arrays()
{
  demo_ragged_t * ret;
  int i;
  int j;
  ser_tra_t * first_tra;
  char * a;

  printf("== Dynamic arrays: ragged 2D\n");

  first_tra = ser_new_tra("ragged_int_array", sizeof(demo_ragged_t), NULL);
  ser_new_field(first_tra, "int",   0, "@rows", offsetof(demo_ragged_t, rows));
  ser_new_field(first_tra, "int**", 1, "rows",  offsetof(demo_ragged_t, data));

  ser_new_dyn_array("int**", "int*", 1, sizeof(int*), first_tra);
  ser_new_nullterm_array("int*", "int", 0, first_tra);

  a = strdup("  ragged_int_array #1 { rows #2; }\n"
	     "  int** #2 { #3,#4,#5 }\n"
	     "  int* #3 { 5,7,9,11,0 }\n"
	     "  int* #4 { 1,1,0 }\n"
	     "  int* #5 { 7,8,9,0 }\n");

  printf("Input data:\n%s\n", a);

  ret = ser_parse(first_tra, "ragged_int_array", a, &ser_def_log);
  free(a);
  
  printf("Values read back with ser_parse(): \n");
  printf("Got %d rows. \n", ret->rows);

  for (i = 0; i < ret->rows; i++)
  {
    for (j = 0; /* in loop */; j++)
    {
      printf("%2d ", ret->data[i][j]);

      if (ret->data[i][j] == 0)
	break;
    }

    printf("\n");
  }

  printf("\n");

  a = ser_ialize(first_tra, "ragged_int_array", ret, &ser_def_log, SER_OPT_COMPACT);

  for (i = 0; i < ret->rows; i++)
    free(ret->data[i]);
  free(ret->data);
  free(ret);

  printf("Serialized again:\n%s\n", a);

  free(a);

  ser_del_tra(first_tra);

  return;
}


void test_demo_tree(ser_tra_t * first_tra)
{
  demo_dir_t * demo;

  char * a;
//  char * b;

  int tree_number;

  tree_number = 0;
  demo = demo_make_tree(2, &tree_number);

  printf("== Input structure\n\n");
  demo_print(demo, 0);

  printf("\n");

  a = ser_ialize(first_tra, "dir", demo, NULL, SER_OPT_DEFAULT);
  printf("== ser_ialize()\n%s\n", a);

  printf("== ser_parse() - inflate what we serialized earlier\n");
  demo = ser_parse(first_tra, "dir", a, &ser_def_log);
  free(a);

  if (!demo)
  {
    printf("Got NULL; something broke. Sorry.\n");
    return;
  }

  demo_print(demo, 0);

  return;
}



void test_circular_refs(ser_tra_t * first_tra)
{
  demo_leaf_t * leaf;
  demo_leaf_t * temp_leaf;

  char * a;

  int i;

  printf("\n== Circular references\n");
  a = strdup("leaf #1 { label \"first leaf\"; n_leaf #2; }\n"
	     "leaf #2 { label \"second leaf\"; n_leaf #3; }\n"
	     "leaf #3 { label \"third leaf\"; n_leaf #1; }\n");

  printf("Parsing:\n%s\n", a);

  leaf = ser_parse(first_tra, "leaf", a, &ser_def_log);

  printf("Iterating:\n");

  temp_leaf = leaf;

  for (i = 0; i < 10; i++)
  {
    printf("%s\n", temp_leaf->label);
    temp_leaf = temp_leaf->n_leaf;
  }

  free(a);

  free(leaf->n_leaf->n_leaf);
  free(leaf->n_leaf);
  free(leaf);
  leaf = NULL;

  return;
}



void test_type_mismatch(ser_tra_t * first_tra)
{
  char * a;
  demo_dir_t * demo;

  a = strdup("dir #1 { n_dir #2; } leaf #2 { letter 's'; }");
  printf("\n== ser_parse(), type mismatch\n"
	 "This should fail with a type mismatch error:\n  %s\n", a);
  demo = ser_parse(first_tra, "dir", a, &ser_def_log);

  if (demo)
    printf("%p %p\n", demo, demo->n_dir);

  free(a);

  return;
}



void test_errors(ser_tra_t * first_tra)
{
  char * a;
  demo_dir_t * demo;

  a = strdup("leaf #1 { label \"");
  printf("\n== ser_ntok()\n"
	 "This should exit with \"premature end of string\":\n  %s\n", a);
  ser_parse(first_tra, "leaf", a, &ser_def_log);
  free(a);

  a = strdup("leaf #1 /* this is some text");
  printf("\n== ser_ntok()\n"
	 "This should exit with \"unterminated comment\":\n  %s\n", a);
  ser_parse(first_tra, "leaf", a, &ser_def_log);
  free(a);

  a = strdup("leaf #aaaargh { }");
  printf("\n== ser_parse()\n"
	 "This should exit with \"bad reference\":\n  %s\n", a);
  ser_parse(first_tra, "leaf", a, &ser_def_log);
  free(a);

  a = strdup("{ }");
  printf("\n== ser_parse()\n"
	 "This should exit with \"expecting type\":\n  %s\n", a);
  ser_parse(first_tra, "leaf", a, &ser_def_log);
  free(a);
  
  a = strdup("badtype #1 { }");
  printf("\n== ser_parse()\n"
	 "This should exit with \"missing translator\":\n  %s\n", a);
  ser_parse(first_tra, "badtype", a, &ser_def_log);
  free(a);
  
  a = strdup("dir #1 { } dir #1 { }");
  printf("\n== ser_parse(), duplicate check"
	 "\nThis should exit with \"already assigned\":\n  %s\n", a);
  ser_parse(first_tra, "dir", a, &ser_def_log);
  free(a);
  
  a = strdup("dir #1 { }");
  printf("\n== ser_parse(), type mismatch\n"
	 "This should say \"does not match expected type\":\n  %s\n", a);
  demo = ser_parse(first_tra, "leaf", a, &ser_def_log);
  free(a);

  a = strdup("dir #1 { n_dir #2; }");
  printf("\n== ser_parse()\n"
	 "This should say \"unable to resolve substitution pointer\":\n  %s\n", a);
  demo = ser_parse(first_tra, "dir", a, &ser_def_log);
  if (demo)
  {
    printf("demo != NULL ...?\n");
  }
  free(a);

  a = strdup("dir #1 { }");
  printf("\n== ser_parse()\n"
	 "This should make an empty dir and rely on ser_blank_struct() to set null values:\n  %s\n", a);
  demo = ser_parse(first_tra, "dir", a, &ser_def_log);
  demo_print(demo, 0);
  free(a);

  demo_del_dir(demo);

  return;
}





void test_strings(void)
{
  char * a;
  char * b;

  printf("\n== ser_escape_quotes()\nThis should put an extra \\ before each \" and \\:\n");
  a = strdup("Let's test \" some \"\" quotes \\\\\"");
  b = ser_escape_str(a);
  printf("%s\n%s\n", a, b);
  free(a);

  printf("\n== ser_unescape_quotes()\nThis should strip the extra \\ before \\ and \":\n");
  a = ser_unescape_str(b);
  printf("%s\n%s\n", b, a);
  free(b);
  free(a);

  printf("\n== ser_unescape_quotes()\nThis should leave other \\ sequences intact:\n");
  a = strdup("Let\\'s try \\some \"other\" \\escape\\sequences \\\" \\");
  printf("%s\n", a);
  b = ser_unescape_str(a);
  printf("%s\n", b);
  free(a);
  free(b);
  
  printf("\n== ser_unescape_quotes()\nThis should convert non-printable characters::\n");
  a = strdup("\x8\x10\x12\x0A");
  printf("%d %d %d %d\n", a[0], a[1], a[2], a[3]);
  b = ser_escape_str(a);
  printf("%s\n", b);
  free(a);
  a = ser_unescape_str(b);
  printf("%d %d %d %d\n", a[0], a[1], a[2], a[3]);
  free(a);
  free(b);
  
  printf("\n== ser_preformat()\n");
  a = strdup("abc;,\";,;\\,\";\";abc,,\"/* comment (padded) */; ;\"/*unterminated-comment;abc\"");
  b = ser_preformat(a);
  printf("Commas, semicolons and comments outside quotes should be padded with spaces.\n"
	 "%s\n%s\n\n", a, b);
  free(a);
  free(b);

  return;
}




void test_nullterm_array()
{
  demo_null_t input;
  demo_null_t * ret;
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  ser_field_t * field;
  char * a;

  printf("\n== Null-terminated array\n");
  
  tra = first_tra = ser_new_tra("some_struct", sizeof(demo_null_t), NULL);
  ser_new_field(tra, "nullterm_array", 1, "data", offsetof(demo_null_t, data));

  tra = ser_new_tra("nullterm_array", sizeof(int), first_tra);
  field = ser_new_field(tra, "int", 0, "stuff", 0);
  field->repeat = 0;
  tra->atype = ser_array_nullterm;



  int stuff[] = {100, 200, 300, 0};

  a = ser_ialize(first_tra, "nullterm_array", &stuff, NULL, 0);

  printf("Serialized output:\n%s\n", a);
  free(a);



  input.data = malloc(sizeof(int) * 6);
  input.data[0] = 15;
  input.data[1] = 17;
  input.data[2] = 19;
  input.data[3] = 21;
  input.data[4] = 23;
  input.data[5] = 0;

  a = ser_ialize(first_tra, "some_struct", &input, NULL, 0);

  free(input.data);

  printf("Serialized output:\n%s\n", a);

  ret = ser_parse(first_tra, "some_struct", a, NULL);

  if (ret)
  {
    printf("Inflated again:");

    int i;
    i = 0;

    while (1)
    {
      printf(" %d", ret->data[i]);
      
      if (ret->data[i] == 0)
	break;

      i++;
    } 

    printf("\n");

    free(ret->data);
    free(ret);
  }
  else
    printf("Something broke; sorry.\n");

  free(a);

  ser_del_tra(first_tra);

  return;
}



void test_embedded()
{
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  ser_field_t * field;
  char * a;
  char * b;
  demo_outer_t * ret;
  int i;
  int j;
  int c;

  demo_outer_t outer[10];

  printf("\n== Embedded structs\n");
  
  tra = first_tra = ser_new_tra("outer_t", sizeof(demo_outer_t), NULL);
  ser_new_field(tra, "int", 0, "a", offsetof(demo_outer_t, a));
  ser_new_field(tra, "int", 0, "b", offsetof(demo_outer_t, b));
  field = ser_new_field(tra, "inner_t", 0, "inner", offsetof(demo_outer_t, inner));
  field->repeat = 5;

  tra = ser_new_tra("inner_t", sizeof(demo_inner_t), first_tra);
  ser_new_field(tra, "int", 0, "c", offsetof(demo_inner_t, c));
  ser_new_field(tra, "int", 0, "d", offsetof(demo_inner_t, d));

  tra = ser_new_tra("outer_t[10]", sizeof(demo_outer_t) * 10, first_tra);
  field = ser_new_field(tra, "outer_t", 0, "___", 0);
  field->repeat = 10;

  c = 0;
  for (i = 0; i < 10; i++)
  {
    outer[i].a = c++;
    outer[i].b = c++;
    
    for (j = 0; j < 5; j++)
    {
      outer[i].inner[j].c = c++;
      outer[i].inner[j].d = c++;
    }
  }

  a = ser_ialize(first_tra, "outer_t[10]", &outer, &ser_def_log, 0);

  printf("Serialized output:\n%s\n", a);

  ret = ser_parse(first_tra, "outer_t[10]", a, &ser_def_log);
  free(a);

  if (ret)
  {
    b = ser_ialize(first_tra, "outer_t[10]", ret, &ser_def_log, 0);
    printf("Serialized again:\n%s\n", b);
    free(ret);
    free(b);
  }
  else
    printf("Something broke; sorry.\n");

  ser_del_tra(first_tra);

  return;
}




int char_array_null_term(ser_job_t * job, ser_tra_t * tra, void * thing)
{
  char tmp[200];
  char * s;

  if (!job || !tra || !thing)
    return 0;

  s = thing;

  ser_job_cat(job, "/* this is output from the custom handler */\n");
  ser_job_cat(job, "stuff");
  ser_job_cat(job, " { ");

  while (*s != '\0')
  {
    if (isprint(*s))
      sprintf(tmp, "'%c', ", *s);
    else
      sprintf(tmp, "'\\x%02X', ", *s);

    ser_job_cat(job, tmp);
    s++;
  }

  ser_job_cat(job, "'\\x00' };\n");
  ser_job_cat(job, "/* end of custom output */\n");

  return 1;
}



void test_time_t()
{
  demo_stuff_t data;
  ser_tra_t * tra;

  printf("== time_t\n");
  
  data.time = time(NULL);

  tra = ser_new_tra("stuff", sizeof(demo_stuff_t), NULL);
  ser_new_field(tra, "time_t", 0, "time", offsetof(demo_stuff_t, time));

  char * a;
  demo_stuff_t *ret;

  a = ser_ialize(tra, "stuff", &data, &ser_def_log, 0);

  printf("Serialized output:\n%s\n", a);

  ret = ser_parse(tra, "stuff", a, &ser_def_log);

  if (ret)
  {
    printf("Inflated again, time: %ld\n", ret->time);
    free(ret);
  }

  free(a);

  ser_del_tra(tra);

  return;

}




void test_embedded_pointers()
{
  demo_outer2_t outer;

  ser_tra_t * first_tra;
  ser_tra_t * tra;
  ser_field_t * field;

  char * a;
  char * b;

  demo_outer2_t * ret;

  int i;

  printf("\n== Pointer array in struct\n");
  
  tra = first_tra = ser_new_tra("outer2_t", sizeof(demo_outer2_t), NULL);
  field = ser_new_field(tra, "inner_t", 1, "inner", offsetof(demo_outer2_t, inner));
  field->repeat = 15;

  tra = ser_new_tra("inner_t", sizeof(demo_inner_t), first_tra);
  ser_new_field(tra, "int", 0, "c", offsetof(demo_inner_t, c));
  ser_new_field(tra, "int", 0, "d", offsetof(demo_inner_t, d));

  for (i = 0; i < 15; i++)
  {
    outer.inner[i] = malloc(sizeof(demo_inner_t));
    outer.inner[i]->c = i * 10;
    outer.inner[i]->d = i * 10 + 1;
  }

  a = ser_ialize(first_tra, "outer2_t", &outer, &ser_def_log, 0);

  printf("Serialized output:\n%s\n", a);

  ret = ser_parse(first_tra, "outer2_t", a, &ser_def_log);
  free(a);

  if (ret)
  {
    for (i = 0; i < 15; i++)
    {
      printf("ret->inner[%d]: %p\n", i, ret->inner[i]);
    }
  
    b = ser_ialize(first_tra, "outer2_t", ret, &ser_def_log, 0);
    printf("\nSerialized again:\n%s\n", b);

    for (i = 0; i < 15; i++)
      free(ret->inner[i]);

    free(ret);
    free(b);
  }
  else
    printf("ERROR: Something broke. Sorry.\n");

  for (i = 0; i < 15; i++)
    free(outer.inner[i]);

  ser_del_tra(first_tra);

  return;
}



void test_more_pointers()
{
  demo_outer3_t outer;
  demo_outer3_t * ret;
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  size_t elm;
  size_t i;
  char * a;

  printf("== More pointers!!!\n");
  
  first_tra = tra = ser_new_tra("outer3_t", sizeof(demo_outer3_t), NULL);
  ser_new_field(tra, "size_t",      0, "@inner", offsetof(demo_outer3_t, inner_count));
  ser_new_field(tra, "inner_array", 1, "inner",  offsetof(demo_outer3_t, inner));

  tra = ser_new_dyn_array("inner_array", "inner_t", 1, sizeof(demo_inner_t*), first_tra);

  tra = ser_new_tra("inner_t", sizeof(demo_inner_t), first_tra);
  ser_new_field(tra, "int", 0, "c", offsetof(demo_inner_t, c));
  ser_new_field(tra, "int", 0, "d", offsetof(demo_inner_t, d));

  elm = 20;

  outer.inner = malloc(sizeof(demo_inner_t *) * elm);
  outer.inner_count = elm;

  for (i = 0; i < elm; i++)
  {
    outer.inner[i] = malloc(sizeof(demo_inner_t));
    outer.inner[i]->c = i * 10;
    outer.inner[i]->d = i * 10 + 1;

    printf("[%2ld]->c = %d\n", i, outer.inner[i]->c);
  }

  a = ser_ialize(first_tra, "outer3_t", &outer, &ser_def_log, 0);

  printf("\nSerialized output:\n%s\n", a);

  ret = ser_parse(first_tra, "outer3_t", a, &ser_def_log);
  free(a);

  for (i = 0; i < elm; i++)
  {
    printf("[%2ld]->c = %d\n", i, ret->inner[i]->c);
  }

  ser_del_tra(first_tra);

  return;
}



void test_structs()
{
  demo_outer4_t outer;
  demo_outer4_t * ret;
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  size_t i;
  size_t elm;
  char * a;

  printf("== Let's write some structs\n");
  
  printf("sizeof(demo_inner_t): %lu\n", sizeof(demo_inner_t));

  first_tra = tra = ser_new_tra("outer4_t", sizeof(demo_outer4_t), NULL);
  ser_new_field(tra, "size_t",      0, "@inner", offsetof(demo_outer4_t, inner_count));
  ser_new_field(tra, "inner_array", 1, "inner",  offsetof(demo_outer4_t, inner));

  tra = ser_new_dyn_array("inner_array", "inner_t", 0, sizeof(demo_inner_t), first_tra);

  tra = ser_new_tra("inner_t", sizeof(demo_inner_t), first_tra);
  ser_new_field(tra, "int", 0, "c", offsetof(demo_inner_t, c));
  ser_new_field(tra, "int", 0, "d", offsetof(demo_inner_t, d));

  elm = 20;

  outer.inner = malloc(sizeof(demo_inner_t) * elm);
  outer.inner_count = elm;

  for (i = 0; i < elm; i++)
  {
    outer.inner[i].c = i * 10;
    outer.inner[i].d = i * 10 + 1;

    printf("[%2ld]->c = %d\n", i, outer.inner[i].c);
  }

  a = ser_ialize(first_tra, "outer4_t", &outer, &ser_def_log, 0);

  printf("\nSerialized output:\n%s\n", a);

  ret = ser_parse(first_tra, "outer4_t", a, &ser_def_log);
  free(a);

  for (i = 0; i < elm; i++)
  {
    printf("[%2ld]->c = %d\n", i, ret->inner[i].c);
  }

  ser_del_tra(first_tra);

  printf("\n");

  return;
}



void test_static_char_array()
{
  ser_tra_t * tra;
  ser_field_t * field;

  char local[30];

  char * ret;
  size_t i;

  printf("\n== Fixed-size char array\n");

  strcpy(local, "test string abc 123");

  for (i = 0; i < 30; i++)
  {
    if (isprint(local[i]))
      printf("local[%2ld]: '%c'\n", i, local[i]);
    else
      printf("local[%2ld]: (non-printable)\n", i);
  }

  tra = ser_new_tra("char[30]", sizeof(char) * 30, NULL);
  field = ser_new_field(tra, "char", 0, "text", 0);
  field->repeat = 30;

  char * a;

  a = ser_ialize(tra, "char[30]", &local, &ser_def_log, 0);

  printf("\nSerialized output:\n%s\n", a);

  ret = ser_parse(tra, "char[30]", a, &ser_def_log);

  if (ret)
  {
    printf("In memory again:\n");
    
    for (i = 0; i < 30; i++)
    {
      if (isprint(ret[i]))
	printf("local[%2ld]: '%c'\n", i, ret[i]);
      else
	printf("local[%2ld]: (non-printable)\n", i);
    }

    free(ret);
  }

  free(a);

  return;
}



void test_single_ptr()
{
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  ser_field_t * field;
  int a[4] = {14, 15, 16, 17};
  char * c;
  int * b;
  int * ret1;
  int ** ret2;

  printf("== int array\n");
  printf("This test serializes an array of ints.\n\n");

  tra = ser_new_tra("_int", sizeof(int) * 4, NULL);
  field = ser_new_field(tra, "int", 0, "value", 0);
  field->repeat = 4;
  
  c = ser_ialize(tra, "_int", &a, &ser_def_log, 0);

  if (c)
  {
    printf("%s\n", c);

    ret1 = ser_parse(tra, "_int", c, &ser_def_log);
    free(c);

    if (ret1)
    {
      printf("Restored values: %d %d %d %d\n",
	     ret1[0], ret1[1], ret1[2], ret1[3]);
      free(ret1);
    }
  }
  
  ser_del_tra(tra);



  printf("== int pointer\n");
  printf("This test serializes a pointer to a single int.\n\n");

  b = &a[0];

  first_tra = tra = ser_new_tra("intptr", sizeof(int*), NULL);
  ser_new_field(tra, "_int", 1, "ptr", 0);

  tra = ser_new_tra("_int", sizeof(int), first_tra);
  ser_new_field(tra, "int", 0, "value", 0);

  c = ser_ialize(first_tra, "intptr", &b, &ser_def_log, 0);

  if (c)
  {
    printf("%s\n", c);

    ret2 = ser_parse(first_tra, "intptr", c, &ser_def_log);

    if (ret2)
    {
      printf("(The return value is really a pointer to a pointer.)\n");
      printf("Restored pointer: %p\n", *ret2);
      printf("The value the pointer is pointing to: %d\n\n", **ret2);
      free(*ret2);
      free(ret2);
    }

    free(c);
  }
  
  ser_del_tra(tra);

  return;
}



void test_meta_serialize(ser_tra_t * tra)
{
  char * a;

  printf("== Meta translators\n");

  a = ser_meta_tra(tra);
  printf("%s\n", a);
  free(a);

  return;
}




void test_overlap()
{
  demo_overlap_t * demo;
  demo_overlap_t * ret;
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  char * sd;

  printf("== Overlapping pointers\n");

  printf("This test sets up two structures, one with a pointer to a specific field in the other. The serializer should detect this and code it as #n.fieldname, then redirect it correctly from the holder list.\n\n");
  
  demo = malloc(sizeof(demo_overlap_t));
  demo->a = 5;
  demo->ptr = NULL;
  demo->next = malloc(sizeof(demo_overlap_t));
  demo->next->a = 7;
  demo->next->next = NULL;
  demo->next->ptr = &demo->a;
  demo->ptr = &demo->next->a;

  first_tra = tra = ser_new_tra("overlap", sizeof(demo_overlap_t), NULL);
  ser_new_field(tra, "int",     0, "a",    offsetof(demo_overlap_t, a));
  ser_new_field(tra, "overlap", 1, "next", offsetof(demo_overlap_t, next));
  ser_new_field(tra, "int",     1, "ptr",  offsetof(demo_overlap_t, ptr));

  printf("Input:\n"
	 "demo: %p\n&demo->a: %p\ndemo->next: %p\n"
	 "demo->next->ptr: %p\ndemo->a: %d\n*demo->next->ptr: %d\n\n",
	 demo, &demo->a, demo->next,
	 demo->next->ptr, demo->a, *demo->next->ptr);

  sd = ser_ialize(first_tra, "overlap", demo, &ser_def_log, 0);

  if (sd)
  {
    printf("Output:\n%s\n", sd);

    ret = ser_parse(first_tra, "overlap", sd, &ser_def_log);

    if (ret)
    {
      printf("Inflated again:\n"
	     "ret: %p\n&ret->a: %p\nret->next: %p\n"
	     "ret->next->ptr: %p\nret->a: %d\n*ret->next->ptr: %d\n\n",
	     ret, &ret->a, ret->next,
	     ret->next->ptr, ret->a, *ret->next->ptr);

      free(ret->next);
      free(ret);
    }

    free(sd);
  }

  /* */
  printf("Now let's provoke an alignment warning.\n");

  demo->next->ptr = (void*)(((void*)&demo->a) + 1);

  sd = ser_ialize(first_tra, "overlap", demo, &ser_def_log, 0);

  if (sd)
  {
//    printf("%s\n", sd);
    free(sd);
  }


  printf("\nNow let's provoke a type mismatch warning.\n");

  demo->next->ptr = (void*)&demo->next;

  sd = ser_ialize(first_tra, "overlap", demo, &ser_def_log, 0);

  if (sd)
  {
//    printf("%s\n", sd);
    free(sd);
  }

  printf("\n");

  free(demo->next);
  free(demo);

  return;
}



void list_type_sizes()
{
#define PRINT_SIZE(n) printf("sizeof(" #n "): %ld\n", sizeof(n))

  printf("== Data type sizes\n");
  PRINT_SIZE(char);
  PRINT_SIZE(short);
  PRINT_SIZE(int);
  PRINT_SIZE(long);
  PRINT_SIZE(float);
  PRINT_SIZE(double);
  PRINT_SIZE(long double);
  PRINT_SIZE(char *);
  PRINT_SIZE(void *);
  PRINT_SIZE(size_t);
  PRINT_SIZE(demo_dir_t);
  PRINT_SIZE(demo_leaf_t);
  PRINT_SIZE(time_t);

  printf("\n");

  return;
}



void test_floats()
{
  demo_float_t f_test;
  demo_float_t * ret;
  ser_tra_t * tra;
  char * a;

  f_test.a = FLT_MAX;
  f_test.b = DBL_MAX;
  f_test.c = LDBL_MAX;
  
  printf("== Serializing floating-point numbers\n\n");
  
  printf("      float a: %e\n" "     double b: %le\n" "long double c: %Le\n",
	 f_test.a, f_test.b, f_test.c);
  
  tra = ser_new_tra("float_test", sizeof(demo_float_t), NULL);
  ser_new_field(tra, "float",   0, "a", offsetof(demo_float_t, a));
  ser_new_field(tra, "double",  0, "b", offsetof(demo_float_t, b));
  ser_new_field(tra, "ldouble", 0, "c", offsetof(demo_float_t, c));
  
  size_t i;
  
  a = ser_ialize(tra, "float_test", &f_test, &ser_def_log, 0);
  
  if (a)
  {
    for (i = 0; i < 100; i++)
    {
      ret = ser_parse(tra, "float_test", a, &ser_def_log);

      free(a);
 
      if (ret == NULL || i == 99)
	break;

      a = ser_ialize(tra, "float_test", ret, &ser_def_log, 0);
      
      free(ret);

      if (a == NULL)
	break;
    }
    
    if (ret)
    {
      printf("ret->a: %e\n"
	     "ret->b: %le\n"
	     "ret->c: %Le\n",
	     ret->a, ret->b, ret->c);
    }
  }

  return;
}


void test_ring()
{
  struct ring_buffer rbuf;
  ser_tra_t * tra;
  ser_field_t * field;

  tra = ser_new_tra("ring_buffer", sizeof(struct ring_buffer), NULL);
  field = ser_new_field(tra, "int", 0, "buffer", offsetof(struct ring_buffer, buffer));
  field->repeat = BSIZE;
  ser_new_field(tra, "int", 1, "head", offsetof(struct ring_buffer, head));
  ser_new_field(tra, "int", 1, "tail", offsetof(struct ring_buffer, tail));

  size_t i;

  for (i = 0; i < BSIZE; i++)
    rbuf.buffer[i] = i;

  rbuf.head = &rbuf.buffer[0];
  rbuf.tail = &rbuf.buffer[0];

  char * a;

  a = ser_ialize(tra, "ring_buffer", &rbuf, &ser_def_log, 0);

  printf("Result:\n%s\n", a);

  free(a);



  printf("== subst ptrs\n");

  struct ring_buffer * ret;

  a = strdup("ring_buffer #1 { buffer { 1,2,3,4,5,6,7,8,9,10 }; head #1.buffer; tail #1.buffer[5]; }");
  
  printf("Input:\n%s\n", a);

  ret = ser_parse(tra, "ring_buffer", a, &ser_def_log);

  free(a);

  if (ret)
  {
    printf("Inflated:\n  ring_buffer at %p\n  head at %p\n   tail at %p\n",
	   ret, ret->head, ret->tail);
    free(ret);
  }

  return;
}




void test_subst_ptrs(void)
{
  demo_subst_t subst;
  demo_subst_t * ret;
  
  char * a;

  ser_tra_t * tra;
  ser_field_t * field;

  int something;

  bool success = false;

  printf("== Pointer substitution\n"
	 "This will set up a structure with pointers to itself (base address) and specific fields within. "
	 "On inflation these should be repointed to the right location.\n");

  tra = ser_new_tra("demo_struct", sizeof(struct demo_subst_t), NULL);
  field = ser_new_field(tra, "int", 0, "array", offsetof(demo_subst_t, array));
  field->repeat = 5;
  field = ser_new_field(tra, "demo_struct", 1, "self", offsetof(demo_subst_t, self));
  field = ser_new_field(tra, "int", 1, "somewhere", offsetof(demo_subst_t, somewhere));
  field = ser_new_field(tra, "int", 1, "other_int", offsetof(demo_subst_t, other_int));

  something = 12345;

  subst.self = &subst;
  subst.array[0] = 100;
  subst.array[1] = 200;
  subst.array[2] = 300;
  subst.array[3] = 400;
  subst.array[4] = 500;
  subst.somewhere = &subst.array[2];
  subst.other_int = &something;

  printf("&subst: %p\nsubst.self: %p\nsubst.somewhere: %p (%d)\nsubst.other_int: %p (%d)\n",
	 &subst, subst.self,
	 subst.somewhere, *subst.somewhere,
	 subst.other_int, *subst.other_int);

  a = ser_ialize(tra, "demo_struct", &subst, &ser_def_log, 0);

  if (a)
  {
    printf("Output:\n%s\n", a);

    ret = ser_parse(tra, "demo_struct", a, &ser_def_log);

    if (ret)
    {
      printf("ret: %p\nret->self: %p\n"
	     "ret->somewhere: %p (%d)\n"
	     "ret->other_int: %p (%d)\n",
	     ret, ret->self,
	     ret->somewhere, *ret->somewhere,
	     ret->other_int, *ret->other_int);

      if (ret->somewhere == &ret->array[2] &&
	  *ret->other_int == 12345)
      {
	success = true;
      }
	
      free(ret->other_int);
      free(ret);
    }
    else
    {
      printf("Error: Inflation failed :(\n");
    }

    free(a);
  }
  else
  {
    printf("Error: Serialization failed :(\n");
  }



  printf("This should fail:\n");

  ret = ser_parse(tra, "demo_struct",
		  "demo_struct #1 { somewhere #2; other_int #1.self; } "
		  "int #2 { 12345 }",
		  &ser_def_log);

  if (ret)
  {
    printf("It worked!?\n");

    printf("ret: %p\nret->self: %p\n"
	   "ret->somewhere: %p (%d)\n"
	   "ret->other_int: %p (%d)\n",
	   ret, ret->self,
	   ret->somewhere, *ret->somewhere,
	   ret->other_int, *ret->other_int);

    free(ret);
    free(ret->somewhere);

    success = false;
  }


  if (success)
    strcat(result, "PASSED: test_subst_ptrs\n");
  else
    strcat(result, "FAILED: test_subst_ptrs\n");
    
  ser_del_tra(tra);

  return;
}




void test_ser_prim()
{
  int * ret;
  char * a;

  printf("== Serialize primitive\n"
	 "This will inflate and return a pointer to a single int.\n");

  ret = ser_parse(NULL, "int", "int #1 { 12345 }", &ser_def_log);

  if (ret)
  {
    printf("Got an int: %d\n", *ret);

    if (*ret != 12345)
      goto it_failed;

    printf("Trying to serialize again...\n");

    a = ser_ialize(NULL, "int", ret, &ser_def_log, 0);

    if (!a)
      goto it_failed;

    printf("Result:\n%s\n", a);

    free(a);
    free(ret);
    strcat(result, "PASSED: test_ser_prim\n");
  }
  else
  {
  it_failed:
    printf("It didn't work :(\n");
    strcat(result, "FAILED: test_ser_prim\n");
  }

  return;
}



void test_bools()
{
  bool value = true;
  bool * ret;
  char * a;

  printf("== bool serialization\n");
  
  a = ser_ialize(NULL, "bool", &value, &ser_def_log, SER_OPT_COMPACT);

  if (a)
  {
    printf("Serialized: %s\n", a);
    free(a);
  }
  else
    goto failure;

  ret = ser_parse(NULL, "bool", "bool #1 { true }", &ser_def_log);

  if (!ret || *ret != true)
    goto failure;

  free(ret);

  ret = ser_parse(NULL, "bool", "bool #1 { false }", &ser_def_log);

  if (!ret || *ret != false)
    goto failure;

  free(ret);

  printf("Seems ok.\n");
  strcat(result, "PASSED: test_bools\n");
  return;

failure:
  printf("FAILURE!!!\n");
  strcat(result, "FAILED: test_bools\n");
  return;
}



void test_dissection()
{
  int tests = 0;
  char * input;

  size_t holder;
  char * tag;
  size_t index;

  printf("== Reference dissection\n");

  input = strdup("#123");
  printf("%-30s: ", input);
  if (ser_dissect_reference(input, &holder, &tag, &index))
  {
    printf("%ld | %s | %ld\n", holder, tag, index);

    if (holder == 123 && tag == NULL && index == 0)
      tests++;
  }
  else
    printf("Failed\n");

  free(input);

  input = strdup("#456.something");
  printf("%-30s: ", input);
  if (ser_dissect_reference(input, &holder, &tag, &index))
  {
    printf("%ld | %s | %ld\n", holder, tag, index);

    if (holder == 456 && strcmp(tag, "something") == 0 && index == 0)
      tests++;
  }
  else
    printf("Failed\n");

  free(input);

  input = strdup("#789.something[5]");
  printf("%-30s: ", input);
  if (ser_dissect_reference(input, &holder, &tag, &index))
  {
    printf("%ld | %s | %ld\n", holder, tag, index);

    if (holder == 789 && strcmp(tag, "something") == 0 && index == 5)
      tests++;
  }
  else
    printf("Failed\n");

  free(input);

  input = strdup("#987.[111]");
  printf("%-30s: ", input);
  if (ser_dissect_reference(input, &holder, &tag, &index))
  {
    printf("%ld | %s | %ld\n", holder, tag, index);

    if (holder == 987 && tag == NULL  && index == 111)
      tests++;
  }
  else
    printf("Failed\n");

  free(input);

  input = strdup("#654[222]");
  printf("%-30s: ", input);
  if (ser_dissect_reference(input, &holder, &tag, &index))
  {
    printf("%ld | %s | %ld\n", holder, tag, index);

    if (holder == 654 && tag == NULL  && index == 222)
      tests++;
  }
  else
    printf("Failed\n");

  free(input);

  input = strdup("#999.");
  printf("%-30s: ", input);
  if (ser_dissect_reference(input, &holder, &tag, &index))
  {
    printf("%ld | %s | %ld\n", holder, tag, index);

    if (holder == 999 && tag == NULL && index == 0)
      tests++;
  }
  else
    printf("Failed\n");

  free(input);
  
  /* Try to parse gibberish; this should fail. */
  input = strdup("#hsoaetug");
  printf("%-30s: ", input);
  if (ser_dissect_reference(input, &holder, &tag, &index) == false)
  {
    printf("Failed (expected)\n");
    tests++;
  }
  else
    printf("%ld | %s | %ld (unexpected; should have failed)\n", holder, tag, index);

  free(input);

  /* Broken arrays */
  input = strdup("#1[");
  printf("%-30s: ", input);
 
  if (ser_dissect_reference(input, &holder, &tag, &index) == false)
  {
    printf("Failed (expected)\n");
    tests++;
  }
  else
    printf("%ld | %s | %ld (unexpected; should have failed)\n", holder, tag, index);

  /* Gibberish in array */
  input = strdup("#1[nseuhtau]");
  printf("%-30s: ", input);
  
  if (ser_dissect_reference(input, &holder, &tag, &index) == false)
  {
    printf("Failed (expected)\n");
    tests++;
  }
  else
    printf("%ld | %s | %ld (unexpected; should have failed)\n", holder, tag, index);

  free(input);

  /* Missing holder */
  input = strdup("#[555]");
  printf("%-30s: ", input);
  
  if (ser_dissect_reference(input, &holder, &tag, &index) == false)
  {
    printf("Failed (expected)\n");
    tests++;
  }
  else
    printf("%ld | %s | %ld (unexpected; should have failed)\n", holder, tag, index);

  free(input);
  
  /* Empty array */
  input = strdup("#1[]");
  printf("%-30s: ", input);
  
  if (ser_dissect_reference(input, &holder, &tag, &index) == false)
  {
    printf("Failed (expected)\n");
    tests++;
  }
  else
    printf("%ld | %s | %ld (unexpected; should have failed)\n", holder, tag, index);

  free(input);

  printf("\n");

  printf("Test summary: %d\n", tests);

  if (tests == 11)
    strcat(result, "PASSED: ");
  else 
    strcat(result, "FAILED: ");

  strcat(result, "test_dissection\n");

  return;
}



int main()
{
  ser_tra_t * first_tra;
  ser_tra_t * tra;
  
  SER_DPRINT("Debug output enabled\n");

  result[0] = '\0';

//  goto summary;  
  test_subst_ptrs();

//    goto summary;  


  test_dynamic_arrays();
  test_maxints();
  test_ser_prim();
  test_dissection();

  test_bools();

  //  goto summary;  

  test_more_pointers();

  test_ring();
  test_dynamic_pointer_array();
  
  test_overlap();


  test_embedded_pointers();
  test_static_arrays();
  test_ragged_arrays();
  test_nullterm_array();
 
  list_type_sizes();
  
  test_floats();

  test_single_ptr();

  
  test_structs();
  
  test_static_char_array();
  test_time_t();
  test_embedded();
  test_strings();
  
  printf("== Building translators (should be silent unless SER_DEBUG)\n");
  
  first_tra = tra = ser_new_tra("dir", sizeof(demo_dir_t), NULL);
  ser_new_field(tra, "string", 0, "label",  offsetof(demo_dir_t, label));
  ser_new_field(tra, "dir",    1, "n_dir",  offsetof(demo_dir_t, n_dir));
  ser_new_field(tra, "dir",    1, "b_dir",  offsetof(demo_dir_t, b_dir));
  ser_new_field(tra, "leaf",   1, "f_leaf", offsetof(demo_dir_t, f_leaf));
  
  tra = ser_new_tra("leaf", sizeof(demo_leaf_t), first_tra);
  ser_new_field(tra, "int",    0, "value",  offsetof(demo_leaf_t, value));
  ser_new_field(tra, "string", 0, "label",  offsetof(demo_leaf_t, label));
  ser_new_field(tra, "leaf",   1, "n_leaf", offsetof(demo_leaf_t, n_leaf));
  ser_new_field(tra, "char",   0, "letter", offsetof(demo_leaf_t, letter));
  
  printf("\n");
  
  printf("== ser_tra_list() - describe translators\n");
  ser_tra_list(first_tra);
  printf("\n");
  
  test_demo_tree(first_tra);
  test_circular_refs(first_tra);
  test_meta_serialize(first_tra);
  test_errors(first_tra);
  test_type_mismatch(first_tra);
  
  printf("\n== ser_del_tra() - delete translators (silent unless CSERI_DEBUG)\n");
  ser_del_tra(first_tra);

  goto summary;

summary:
  printf("\n== Summary\n%s\n", result);

  return 0;
}
