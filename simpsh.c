#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

/* Flag set by ‘--verbose’. */
static int verbose_flag;

/* Data/storage for file descriptors */
static int fileIndex;
static int* files;
static int FILE_CAPACITY;

/* Storage for command arguments */
static int CMD_CAPACITY;

/* Storage for I/O/E */
typedef struct 
{
  int fd1, fd2, fd3;
} cmdfds;

/* STD IN/OUT/ERR */
int saved_IN;
int saved_OUT;
int saved_ERR;

/* Final return status */
static int return_value;

void checkmemory()
{
  /* Check memory of allocated file array, 
      add more if at FILE_CAPACITY, 
      exit if allocation error */
  if( fileIndex == FILE_CAPACITY )
  {
    FILE_CAPACITY = FILE_CAPACITY * 2;
    files = realloc(files, FILE_CAPACITY * sizeof(int));
  }
  if( errno == ENOMEM )
    exit(EXIT_FAILURE);
}

void openfile( const char *path, int flag )
{
  /* Open requested file with correct OFLAG */
  int fd = open(path, flag);
  if (fd == -1)
  {
    /* Return error if failure to open file */
    fprintf(stderr, "Error: Failed to open file!\n");
    return_value = 1;
    return;
  }
  //printf("File descriptor old: %d && fileIndex: %d\n", fd, fileIndex);
  files[fileIndex] = fd;
  fileIndex++;
}

int missingfile( const char *arg )
{
  /* Check if --rdonly / --wronly are missing an operand */
  if( optarg == NULL || (optarg[0] == '-' && optarg[1] == '-'))
  {
    fprintf( stderr, "Error: Missing file operand!\n" );
    return 1;
  }
  return 0;
}

void runCommand(cmdfds fds, char *args[])
{
  /* Runs the command as specificied by --command */

  pid_t pid = fork();
  
  if( pid < 0 )
    fprintf( stderr, "Error: Couldn't create a child process!\n" );
  else if (pid == 0)
  {
    /* Child thread to run the bash command */

    /* Change standard input, output, error to user specifications */
    dup2( files[fds.fd1], 0 );
    dup2( files[fds.fd2], 1 );
    dup2( files[fds.fd3], 2 );

    /* By convention, args[0] is the cmd name, args must end with a null ptr */
    if(execvp(args[0], args) == -1)
    {
      fprintf( stderr, "Error: Unable to execute command! Exiting in failure.\n");
      exit(EXIT_FAILURE);
    }
  }
}

int main (int argc, char **argv)
{
  int c;
  verbose_flag = 0;
  return_value = 0;

  int firstVerbose = 1;

  /* Set up array of file descriptors */
  fileIndex = 0;
  FILE_CAPACITY = 10;
  files = malloc(FILE_CAPACITY * sizeof(int));
  checkmemory();
  
  while (1)
    {
      static struct option long_options[] =
        {
          /* This option sets a flag. */
          {"verbose",    no_argument,       &verbose_flag,   1},
          /* These options don’t set a flag. */
          {"rdonly",     required_argument, 0,              'r'},
          {"wronly",     required_argument, 0,              'w'},
          {"command",    required_argument, 0,              'c'},
          {0,            0,                 0,               0}
        };

      /* getopt_long stores the option index here. */
      int option_index = 0;

      /* Suppress getopt_long error messages */
      opterr = 0;
      
      /* Empty string "" since no short options allowed. */
      c = getopt_long (argc, argv, "",
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
      case 0:
        /* This option set a flag, do nothing else now. */
      	if (!firstVerbose)
      		printf("--verbose\n");
      	firstVerbose = 0;
        break;

      case 'r':
        /* Check for missing file operand */
        if(missingfile(optarg))
        {
          optind--;
          return_value = 1;
          if(verbose_flag)
            printf ("--rdonly\n");
          break;
        }

        /* If verbose flag is set */
        if(verbose_flag)
          printf ("--rdonly %s\n", optarg);
        checkmemory();
        openfile(optarg, O_RDONLY);
        break;

      case 'w':
        /* Check for missing file operand */
        if(missingfile(optarg))
        {
          optind--;
          return_value = 1;
          if(verbose_flag)
            printf ("--wronly\n");
          break;
        }

        /* If verbose flag is set */
        if(verbose_flag)
          printf ("--wronly %s\n", optarg);
        checkmemory();
        openfile(optarg, O_WRONLY);
        break;

      case 'c':

        if( optind+2 > argc )
        {
          fprintf( stderr, "Error: Missing additional operands!\n" );
          return_value = 1;
          break;
        }

        if (optarg[0] < '0' || optarg[0] > '9' ||
            argv[optind][0] < '0' || argv[optind][0] > '9' ||
            argv[optind + 1][0] < '0' || argv[optind + 1][0] > '9')
        {
          fprintf( stderr, "Error: First 3 arguments to --command are not digits!\n" );
          return_value = 1;
          break;
        }

    /* Gather file descriptors for i o e, assuming integers */
    cmdfds stdioe = {atoi(optarg), atoi(argv[optind]), atoi(argv[optind+1])};

    if( (stdioe.fd1 >= fileIndex) || (stdioe.fd2 >= fileIndex) || 
        (stdioe.fd3 >= fileIndex) )
    {
      fprintf( stderr, "Error: File descriptors out of range!\n" );
      return_value = 1;
      break;
    }

    /* Collect arguments for the command until another -- is hit.*/
    CMD_CAPACITY = 10;
    char **args = (char**)malloc(CMD_CAPACITY * sizeof(char*));

    int index = optind + 2;
    int argsCounter = 0;
    while( index <= argc )
    {
      /* If found an argument with leading '--', stop collecting */
      if( index == argc || (argv[index][0] == '-' && argv[index][1] == '-'))
      {
        args[argsCounter] = NULL;
        optind = index;
        break;
      }
      if( argsCounter == CMD_CAPACITY )
      {
        /* Reallocate memory */
        CMD_CAPACITY *= 2;
        args = (char**)realloc(args, CMD_CAPACITY * sizeof(char*));
		if( errno == ENOMEM )
    		exit(EXIT_FAILURE);
      }
      args[argsCounter] = argv[index];
      argsCounter++;
      index++;
    }

    /* If verbose flag is set */
    if(verbose_flag)
    {
      printf ("--command %d %d %d ", stdioe.fd1, stdioe.fd2, stdioe.fd3);
      for( int j = 0; j < argsCounter; j++ )
        printf ("%s ", args[j] );
      printf("\n");
    }

    runCommand(stdioe, args);
    free(args);
    break;

        case '?':
          /* getopt_long already printed an error message. */
        default:
          printf ("Error: Unrecognized command!\n");
          return_value = 1;
        }
    }

  /* Close file descriptors and free allocated memory */
  for( int i = fileIndex-1; i >= 0; i-- )
    close(files[i]);
  free(files);

  exit (return_value);
}
