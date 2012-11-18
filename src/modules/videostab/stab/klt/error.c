/*********************************************************************
 * error.c
 *
 * Error and warning messages, and system commands.
 *********************************************************************/


/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <framework/mlt_log.h>

/*********************************************************************
 * KLTError
 * 
 * Prints an error message and dies.
 * 
 * INPUTS
 * exactly like printf
 */

void KLTError(char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  mlt_log_error(NULL, "KLT Error: ");
  mlt_log_error(NULL, fmt, args);
  mlt_log_error(NULL, "\n");
  va_end(args);
}


/*********************************************************************
 * KLTWarning
 * 
 * Prints a warning message.
 * 
 * INPUTS
 * exactly like printf
 */

void KLTWarning(char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  fprintf(stderr, "KLT Warning: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  fflush(stderr);
  va_end(args);
}

