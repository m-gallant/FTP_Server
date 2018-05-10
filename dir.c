#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/stat.h>
#include "dir.h"

/*
   Arguments:
      fd - a valid open file descriptor. This is not checked for validity
           or for errors with it is used.
      directory - a pointer to a null terminated string that names a
                  directory
   Returns
      -1 the named directory does not exist or you don't have permission
         to read it.
      -2 insufficient resources to perform request

   This function takes the name of a directory and lists all the regular
   files and directoies in the directory.
 */

int listFiles(int fd, char * directory) {

  // Get resources to see if the directory can be opened for reading
  DIR * directoryToPrint = NULL;

  directoryToPrint = opendir(directory);
  if (!directoryToPrint) return -1;

  // print regular files and directories.
  struct dirent *directoryEntry;
  int entriesPrinted = 0;
  for (directoryEntry = readdir(directoryToPrint);
        directoryEntry; directoryEntry = readdir(directoryToPrint)) {
    if (directoryEntry->d_type == DT_REG) {  // Regular file
      struct stat buf;
      // check the return value of stat to ensure not an error
      if (stat(directoryEntry->d_name, &buf) == 0) {
        dprintf(fd, "F    %-20s     %d\r\n", directoryEntry->d_name, buf.st_size);
      } else {
        perror("list error");
        // minus here to account for ++ after
        entriesPrinted--;
      }
    } else if (directoryEntry->d_type == DT_DIR) { // Directory
      dprintf(fd, "D        %s\r\n", directoryEntry->d_name);
    } else {
      dprintf(fd, "U        %s\r\n", directoryEntry->d_name);
    }
    entriesPrinted++;
  }

  // Release resources
  closedir(directoryToPrint);
  return entriesPrinted;
}
