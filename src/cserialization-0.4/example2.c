/*
  C serialization library, example 2
  happyponyland.net

  In this example we introduce the string and char types. char
  corresponds closely to C's own char type and string is a shorthand
  notation for "pointer to char array".

  We will also deserialize data and read it back into a structure.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "serialize.h"


typedef struct
{
  char* first_name;
  char  initial;
  char* last_name;
} person_t;


int main()
{
  /*
    Note that the field tag need not match the member variable. To
    the translator they are just memory locations; it doesn't care
    what the variable is called in the source code.
  */

  ser_tra_t* tra;
  
  tra = ser_new_tra("person", sizeof(person_t), NULL);
  ser_new_field(tra, "string", 0, "first",   offsetof(person_t, first_name));
  ser_new_field(tra, "char",   0, "initial", offsetof(person_t, initial));
  ser_new_field(tra, "string", 0, "last",    offsetof(person_t, last_name));


  /*
    Set up some sample data and dump it. The serializer will
    follow string pointers and output them in a format that
    is convenient to edit.
  */

  person_t my_person;
  char* result;

  my_person.first_name = strdup("Ulf");
  my_person.initial    = 'J';
  my_person.last_name  = strdup("Astrom");

  result = ser_ialize(tra, "person", &my_person, NULL, 0);
  printf("Result:\n%s\n", result);

  free(result);
  free(my_person.first_name);
  free(my_person.last_name);


  /*
    Now let's try reading data back into a structure.

    Note that new_person is a pointer. The translator will allocate
    the amount of memory passed as second argument to ser_new_tra.
    It will also allocate memory for the strings read.
  */

  person_t* new_person;

  /* Sorry about the mess of quotes; you know how it is. */
  new_person = ser_parse(tra, "person",
			 "person #1 { first \"Ulysses\"; "
			 "initial 'S'; "
			 "last \"Grant\"; }",
			 NULL);

  printf("We got a name: %s %c %s\n",
	 new_person->first_name,
	 new_person->initial,
	 new_person->last_name);
  
  free(new_person->first_name);
  free(new_person->last_name);
  free(new_person);

  ser_del_tra(tra);

  return 0;
}
