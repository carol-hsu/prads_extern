/*
  C serialization library, example 1
  happyponyland.net

  In this example we will set up a translator
  for a simple structure and study its output.
*/

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "serialize.h"


typedef struct
{
  int a;
  int b;
} thing_t;


int main()
{
  /*
    Set up a translator for thing_t. Each kind of structure we wish to
    export needs its own translator. The translator wants to know how
    large its structure will be.

    We will add two fields to the translator; a and b of type int. As
    the last parameter ser_new_field expects the offset of the member
    variable within the structure.
    
    In this example thing_t.a will be at offset 0 and thing_t.b will
    end up 2 to 8 bytes from the base pointer, depending on the
    computer architecture. We will use the offsetof() macro and rely
    on the compiler to tell us where they are. Hardcoding the values
    is not recommended.

    The third parameter to ser_new_tra should be another translator
    (they're really linked lists), but for now we only need one so
    this can be omitted.
  */

  ser_tra_t* tra;
  
  tra = ser_new_tra("thing", sizeof(thing_t), NULL);
  ser_new_field(tra, "int", 0, "a", offsetof(thing_t, a));
  ser_new_field(tra, "int", 0, "b", offsetof(thing_t, b));


  /* Prepare some sample data. */

  thing_t my_thing;

  my_thing.a = 5;
  my_thing.b = 20;


  /*
    Start the serialization process.

    ser_ialize wants to know the location in memory of the first
    structure to be serialized (&my_thing) and what to make of it
    ("thing"). The type must be present in the list of translators.

    Let's ignore the last two parameters for now.
  */

  char * result;
  
  result = ser_ialize(tra, "thing", &my_thing, NULL, 0);

  printf("Result:\n%s\n", result);


  /* Clean up */

  free(result);

  ser_del_tra(tra);

  return 0;
}
