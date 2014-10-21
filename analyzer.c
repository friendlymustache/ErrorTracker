#include <stdio.h>
#include <stdlib.h>
#include <pcre.h>
#include "create_error_commit.h"
const int MAX_BUF_SIZE = 255;
const int STDIN = 0;

int main(int argc, char** argv) {
  srand(time(0));
  char *buf = (char *) calloc(MAX_BUF_SIZE, sizeof(char));
  int error;

  while(read(STDIN, buf, MAX_BUF_SIZE)) {
    error = detect_error(buf);
    if(error) {
      create_error(buf);
      printf("Error detected\n");
    }
    else {
      printf("No error\n");
    }
  }

  return 0;
}


/* Uses the PCRE match function to look for 'error' or 'exception'
 * in the provided input; returns 1 if the input describes an error
 * and 0 otherwise
 */
int parse_stdin(char **input) {
  return rand() % 2 == 0;
}


/* Returns 1 if an error was present in the output and 0 otherwise */
int detect_error(char* terminal_output) {
  pcre *reCompiled;
  pcre_extra *pcreExtra;
  int pcreExecRet;
  int subStrVec[30];
  const char *pcreErrorStr;
  int pcreErrorOffset;
  char *aStrRegex;
  const char *psubStrMatchStr;
  int j;
  char *testStrings[] = { "This should match... hello",
                          "This could match... hello!",
                          "More than one hello.. hello",
                          "No chance of a match...",
                          NULL};


  // aStrRegex = "(.*)(hello)+";  
  aStrRegex = "exception|error";                         

  // First, the regex string must be compiled.
  reCompiled = pcre_compile(aStrRegex, PCRE_CASELESS, &pcreErrorStr, &pcreErrorOffset, NULL);

  // pcre_compile returns NULL on error, and sets pcreErrorOffset & pcreErrorStr
  if(reCompiled == NULL) {
    printf("ERROR: Could not compile '%s': %s\n", aStrRegex, pcreErrorStr);
    exit(1);
  } /* end if */

  // Optimize the regex
  pcreExtra = pcre_study(reCompiled, 0, &pcreErrorStr);

  /* pcre_study() returns NULL for both errors and when it can not optimize the regex.  The last argument is how one checks for
     errors (it is NULL if everything works, and points to an error string otherwise. */
  if(pcreErrorStr != NULL) {
    printf("ERROR: Could not study '%s': %s\n", aStrRegex, pcreErrorStr);
    exit(1);
  } /* end if */


  /* Try to find the regex in terminal_output, and report results. */
  pcreExecRet = pcre_exec(reCompiled,
                          pcreExtra,
                          terminal_output, 
                          strlen(terminal_output),  // length of string
                          0,                      // Start looking at this point
                          0,                      // OPTIONS
                          subStrVec,
                          30);                    // Length of subStrVec

  // Report what happened in the pcre_exec call..
  if(pcreExecRet < 0) { // Something bad happened..
    switch(pcreExecRet) {
    case PCRE_ERROR_NOMATCH      :                                                      return 0;
    case PCRE_ERROR_NULL         : printf("Something was null\n");                      break;
    case PCRE_ERROR_BADOPTION    : printf("A bad option was passed\n");                 break;
    case PCRE_ERROR_BADMAGIC     : printf("Magic number bad (compiled re corrupt?)\n"); break;
    case PCRE_ERROR_UNKNOWN_NODE : printf("Something kooky in the compiled re\n");      break;
    case PCRE_ERROR_NOMEMORY     : printf("Ran out of memory\n");                       break;
    default                      : printf("Unknown error\n");                           break;
    } /* end switch */
  } 

  /* We have a match */
  else {
    // Free up the substring
    pcre_free_substring(psubStrMatchStr);
    pcre_free(reCompiled);
    return 1;

  } 


}

