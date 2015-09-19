/*
  C serialization library, example 3
  happyponyland.net

  In this example we will serialize a linked list.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "serialize.h"


typedef struct node_t node_t;

struct node_t
{
  char*   label;
  node_t* next;
};


int main()
{
  /*
    Set up the translator.
    
    Note that the third argument to ser_new_field is 1; this will make
    the translator treat this field as a pointer to another structure.

    When a structure is serialized it will add all its references to
    the list of things to be serialized. We only need to pass the
    first node and the rest will follow.
  */

  ser_tra_t* tra;
  
  tra = ser_new_tra("node", sizeof(node_t), NULL);
  ser_new_field(tra, "string", 0, "label", offsetof(node_t, label));
  ser_new_field(tra, "node",   1, "next",  offsetof(node_t, next));

  /*
    Populate a short list.
    a will point to the start of the list.
  */

  char tmp[50];
  node_t* a;
  node_t* b;
  int i;

  a = NULL;

  for (i = 5; i > 0; i--)
  {
    b = malloc(sizeof(node_t));
    sprintf(tmp, "node %d", i);
    b->label = strdup(tmp);
    b->next = a;
    a = b;
  }

  /*
    Serialize the list.

    This should print the nodes we created above.
    The last should end with "next NULL;".
  */
  
  char* result;

  result = ser_ialize(tra, "node", a, NULL, 0);

  printf("Result:\n%s\n", result);
  
  free(result);

  while (a)
  {
    b = a->next;
    free(a->label);
    free(a);
    a = b;
  }

  /*
    Now we will read a short list. The input intentionally mixes up
    the order of the nodes; the serializer will get this right.
  */

  a = ser_parse(tra, "node",
		"node #2 { label \"third node\";  next NULL; }"
		"node #1 { label \"first node\";  next #3;   }"
		"node #3 { label \"second node\"; next #2;   }",
		NULL);
  
  printf("Let's see what we got...\n");

  while (a)
  {
    printf("%s\n", a->label);
    b = a->next;
    free(a->label);
    free(a);
    a = b;
  }

  ser_del_tra(tra);

  return 0;
}
