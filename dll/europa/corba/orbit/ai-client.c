#include <stdio.h>
#include <orb/orbit.h>

#include "europa.h"

EuropaAI_Europa ai_client;

int main(int argc, char *argv[]) {
  CORBA_Environment ev;
  CORBA_ORB orb;
  FILE *ifp;
  char *ior;
  char filebuffer[1024];

  /*
   * Standard initalisation of the orb. Notice that
   * ORB_init 'eats' stuff off the command line
   */
  
  CORBA_exception_init(&ev);
  orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ev);

  /*
   * Get the IOR (object reference). It should be written out
   * by the echo-server into the file echo.ior. So - if you
   * are running the server in the same place as the client,
   * this should be fine!
   */

    ifp = fopen("ai.ior","r");
    if( ifp == NULL ) {
      g_error("No ai.ior file!");
      exit(-1);
    }

    fgets(filebuffer,1024,ifp);
    ior = g_strdup(filebuffer);

    fclose(ifp);

    /*
     * Actually get the object. So easy!
     */
    ai_client = CORBA_ORB_string_to_object(orb, ior, &ev);
    if(!ai_client) {
      printf("Cannot bind to %s\n", ior);
      return 1;
    }

    /*
     * Ok. Now we use the echo object...
     */
    printf("Type messages to the server\n. as the only thing on the line stops\n
");
    while( fgets(filebuffer,1024,stdin) ) {
      if( filebuffer[0] == '.' && filebuffer[1] == '\n' ) 
        break;
      
      /* chop the newline off */
      filebuffer[strlen(filebuffer)-1] = '\0';
      
      /* using the echoString method in the Echo object               */
      /* this is defined in the echo.h header, compiled from echo.idl */

      EuropaAI_Europa_inputChat(ai_client, filebuffer, &ev);

      /* catch any exceptions (eg, network is down) */

      if(ev._major != CORBA_NO_EXCEPTION) {
        printf("we got exception %d from inputChat!\n", ev._major);
        return 1;
      }
    }

     
    /* Clean up */
    CORBA_Object_release(ai_client, &ev);
    CORBA_Object_release((CORBA_Object)orb, &ev);

    return 0;
}
