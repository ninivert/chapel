#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "chpllaunch.h"
#include "chplmem.h"
#include "chpltypes.h"
#include "error.h"

#define baseLLFilename ".chpl-ll-qsub-"
#define baseExpectFilename ".chpl-expect-"

char llFilename[FILENAME_MAX];
char expectFilename[FILENAME_MAX];

/* copies of binary to run per node */
#define procsPerNode 1  
#define versionBuffLen 80

#define launcherAccountEnvvar "CHPL_LAUNCHER_ACCOUNT"

char* chpl_launch_create_command(int argc, char* argv[], int32_t numLocales) {
  int i;
  int size;
  char baseCommand[256];
  char* command;
  FILE* llFile, *expectFile;
  char* projectString = getenv(launcherAccountEnvvar);
  char* basenamePtr = strrchr(argv[0], '/');
  pid_t mypid;

  if (basenamePtr == NULL) {
      basenamePtr = argv[0];
  } else {
      basenamePtr++;
  }

#ifndef DEBUG_LAUNCH
  mypid = getpid();
#else
  mypid = 0;
#endif
  sprintf(expectFilename, "%s%d", baseExpectFilename, (int)mypid);
  sprintf(llFilename, "%s%d", baseLLFilename, (int)mypid);

  llFile = fopen(llFilename, "w");
  fprintf(llFile, "# @ wall_clock_limit = 00:10:00\n");
  fprintf(llFile, "# @ job_type = parallel\n");
  fprintf(llFile, "# @ node = %d\n", numLocales);
  fprintf(llFile, "# @ tasks_per_node = 1\n");
  if (projectString && strlen(projectString) > 0)
      fprintf(llFile, "# @ class = %s\n", projectString);
  fprintf(llFile, "# @ output = out.$(jobid)\n");
  fprintf(llFile, "# @ error = err.$(jobid)\n");
  fprintf(llFile, "# @ queue\n");
  fprintf(llFile, "\n");
  fprintf(llFile, "%s_real", argv[0]);
  for (i=1; i<argc; i++) {
      fprintf(llFile, " '%s'", argv[i]);
  }
  fprintf(llFile, "\n");
  fclose(llFile);

  sprintf(baseCommand, "llsubmit %s", llFilename);

  size = strlen(baseCommand) + 1;

  command = chpl_malloc(size, sizeof(char), "command buffer", -1, "");
  
  sprintf(command, "%s", baseCommand);

  if (strlen(command)+1 > size) {
    chpl_internal_error("buffer overflow");
  }

  return command;
}

void chpl_launch_sanity_checks(int argc, char* argv[], const char* cmd) {
  // Do sanity checks just before launching.
  struct stat statBuf;
  char realName[256];
  int retVal;

  retVal = snprintf(realName, 256, "%s_real", argv[0]);
  if (retVal < 0 || retVal >= 256) {
    chpl_internal_error("error generating back-end filename");
  }

  // Make sure the _real binary exists
  if (stat(realName, &statBuf) != 0) {
    char errorMsg[256];
    sprintf(errorMsg, "unable to locate file: %s", realName);
    chpl_error(errorMsg, -1, "<internal>");
  }
}

void chpl_launch_cleanup(void) {
#ifndef DEBUG_LAUNCH
  char command[1024];

  sprintf(command, "rm %s", llFilename);
  system(command);

#endif
}
