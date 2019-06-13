/* 
 * tsh - A tiny shell program with job control
 * 
 * <Jiacheng Sun jzs375@psu.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
  pid_t pid;              /* job PID */
  int jid;                /* job ID [1, 2, ...] */
  int state;              /* UNDEF, BG, FG, or ST */
  char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
  char c;
  char cmdline[MAXLINE];
  int emit_prompt = 1; /* emit prompt (default) */

  /* Redirect stderr to stdout (so that driver will get all output
   * on the pipe connected to stdout) */
  dup2(1, 2);

  /* Parse the command line */
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
      case 'h':             /* print help message */
        usage();
        break;
      case 'v':             /* emit additional diagnostic info */
        verbose = 1;
        break;
      case 'p':             /* don't print a prompt */
        emit_prompt = 0;  /* handy for automatic testing */
        break;
      default:
        usage();
    }
  }

  /* Install the signal handlers */

  /* These are the ones you will need to implement */
  Signal(SIGINT,  sigint_handler);   /* ctrl-c */
  Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
  Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

  /* This one provides a clean way to kill the shell */
  Signal(SIGQUIT, sigquit_handler); 

  /* Initialize the job list */
  initjobs(jobs);

  /* Execute the shell's read/eval loop */
  while (1) {

    /* Read command line */
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }
    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
      app_error("fgets error");
    if (feof(stdin)) { /* End of file (ctrl-d) */
      fflush(stdout);
      exit(0);
    }

    /* Evaluate the command line */
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); /* control never reaches here */
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 */
void eval(char *cmdline)
{
  char *argv[MAXARGS]; /* hold the buffer for the cmdline */
  char buf[MAXLINE];   /* background job if or not */
  int bg;              /* background job if or not */
  pid_t pid;           /* initialize pid */
  sigset_t mask;       /* initialize mask for set sig */

  sigemptyset(&mask);        /* create the empty sig set */
  sigaddset(&mask, SIGCHLD); /* add SIGCHLD to the set */
  //sigprocmask (SIG_BLOCK, &mask, NULL);
  strcpy(buf, cmdline);
  bg = parseline(buf, argv); /* parse the cmdline and build the argv array */
                             /* if bg return 1, fg return 0 */
  if (argv[0] == NULL){
    //sigprocmask (SIG_UNBLOCK, &mask, NULL);
    return;
  }
  /* condition of not buildin cmd */
  if (!builtin_cmd (argv)){

    /*block the sig */
    sigprocmask (SIG_BLOCK, &mask, NULL);

    /* error handling using unix error
     * and child run the fork() */
    if ((pid = fork()) < 0) {
      unix_error("fork(): forking error\n");
      //printf("fork(): forking error\n");
      return;
  }

  /* give the group id for child process */
    if (pid == 0) {
      setpgid(0,0);
      /* condition of no such command */
      sigprocmask(SIG_UNBLOCK, &mask, NULL);
      if(execvp(argv[0], argv) < 0) {
        printf("%s: Command not found \n", argv[0]);
        exit(0);
      }
    }

    else {
      /* condition of bg job, which runs after fg to terminate */
      if(bg == 1) {
        addjob(jobs, pid, BG, cmdline);
        printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
      }
      /* add fg job */
      else {
        addjob(jobs, pid, FG, cmdline);
      }

      /* unblock the sig */
      sigprocmask(SIG_UNBLOCK, &mask, NULL);
      waitfg(pid);
    }
  } 
  return;
}


/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
  static char array[MAXLINE]; /* holds local copy of command line */
  char *buf = array;          /* ptr that traverses command line */
  char *delim;                /* points to first space delimiter */
  int argc;                   /* number of args */
  int bg;                     /* background job? */

  strcpy(buf, cmdline);
  buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) /* ignore leading spaces */
    buf++;

  /* Build the argv list */
  argc = 0;
  if (*buf == '\'') {
    buf++;
    delim = strchr(buf, '\'');
  }
  else {
    delim = strchr(buf, ' ');
  }

  while (argc < MAXARGS-1 && delim) {
    argv[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* ignore spaces */
      buf++;

    if (*buf == '\'') {
      buf++;
      delim = strchr(buf, '\'');
    }
    else {
      delim = strchr(buf, ' ');
    }
  }
  if (delim) {
    fprintf(stderr, "Too many arguments.\n");
    argc = 0; //treat it as an empty line.
  }
  argv[argc] = NULL;

  if (argc == 0)  /* ignore blank line */
    return 1;

  /* should the job run in the background? */
  if ((bg = (*argv[argc-1] == '&')) != 0) {
    argv[--argc] = NULL;
  }
  return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
  /* quit command*/
  if(!strcmp(argv[0], "quit")){
    int i;
    int stopped = 0;
    
    /* quit command of that report unterminated jobs first
     * and exit after all jobs are terminated*/
    for(i = 0; i < 16; i++){
      if(jobs[i].state == ST)
        stopped = 1;
    }

    if (stopped == 1) {
      printf("There are stopped jobs\n");
      return 1;
    }
    else{
      exit(0);
    }
  }

  /* buildin commands of jobs*/
  if(!strcmp(argv[0],"jobs")){
    listjobs(jobs);
    return 1;
  }


  /* buildin bg fg command*/
  if(!strcmp(argv[0],"fg") || !strcmp(argv[0],"bg")){
    do_bgfg(argv);
    return 1;
  } 

  if (!strcmp(argv[0], "&"))
    return 1;

  /* if not a builtin command */
  return 0;     
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
  int jid;
  int pid;
  struct job_t *job;
  char *args = argv[1];
  
  /* condition of cmd exist */
  if (args != NULL){
    
    /* find job according to a correct jid 
     * or return if job does not exist */
    if(args[0] == '%' && isdigit(args[1])) {
      jid = atoi(&args[1]);
      if(!(job = getjobjid(jobs, jid))){
        printf("%s: No such job\n", args);
        return;
      }
    }

    /* find a job according to a correct pid 
     * or return if the process does not exist*/
    else if(isdigit(*argv[1])){
      pid = atoi(&args[0]);
      if (!(job = getjobpid(jobs,pid))) {
        printf("(%s): No such process\n", argv[1]);
        return;
      }
    }
    
    /* incorrect jid and pid */
    else {
      printf("%s: argument must be a PID or %%jobid \n", argv[0]);
      return;
    }
  }
  
  /* not have bg or fg cmd */
  else {
    printf("%s command requires PID or %%jobid argument \n", argv[0]);
    return;
  }


  /* condition of job exists */
  if (job != NULL){
    pid = job -> pid;
    
    /* rerun stopped process */
    if(job -> state == ST){
      
      /* run the stopped process in bg 
       * after getting the SIGCONT (continue sig) */
      if(!strcmp(argv[0], "bg")){
        printf("[%d] (%d) %s", job->jid,job->pid,job->cmdline);
        job -> state =BG;
        kill(-pid, SIGCONT);
      }

      /* run the stopped in fg after getting SIGCONT 
       * wait for existed fg running process terminated or stopped*/
      if(!strcmp(argv[0],"fg")){
        job->state = FG;
        kill(-pid, SIGCONT);
        waitfg(job -> pid);
      }
    }
    
    /* run the bg job on fg */
    if(job -> state == BG){
      if(!strcmp(argv[0], "fg")){
        job -> state = FG;
        waitfg(job ->pid);
      }
    }
  }
  return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
  struct job_t *job;
  job = getjobpid(jobs,pid);

  /* while the state if FG, wait for a second and check again*/
  while(job->state == FG){
    sleep(1);
  }
  return;

}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
  int status;
  pid_t pid;
  
  /* wait for process to terminate or stop, return immediately if no child exited
   * also return if a child stopped */
  while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED )) > 0) {
    
    /* condition of normal termination */
    if (WIFEXITED (status)) {
      deletejob(jobs, pid);
    }
    
    /* condition of terminatioin by a sig */
    if (WIFSIGNALED(status)) {
      printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
      deletejob(jobs, pid);
    }

    /* condtion of stop by a sig */
    if (WIFSTOPPED(status)) {
      getjobpid(jobs, pid) -> state = ST;
      printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
    }
  }
  
  /* condition of incorrect pid */
  if (pid < 0 && errno != ECHILD) {
    printf("waitpid error: %s\n", strerror(errno));
    //unix_error("waitpid error: ");
  }
  return;
}
	
/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
  pid_t pid = fgpid(jobs);
  /* send interrupt sig to fg process with this pid */
  if (fgpid(jobs) != 0){
    kill(-pid, SIGINT);
  }
  return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
  pid_t pid = fgpid(jobs);
  /*  send stop sig to fg process with this pid */
  if (fgpid(jobs) != 0){
    kill(-pid, SIGTSTP);
  }

  return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = UNDEF;
  job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
  int i;

  for (i = 0; i < MAXJOBS; i++)
    clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
  int i, max=0;

  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid > max)
      max = jobs[i].jid;
  return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == 0) {
      jobs[i].pid = pid;
      jobs[i].state = state;
      jobs[i].jid = nextjid++;
      if (nextjid > MAXJOBS)
        nextjid = 1;
      strcpy(jobs[i].cmdline, cmdline);
      if(verbose){
        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
      }
      return 1;
    }
  }
  printf("Tried to create too many jobs\n");
  return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
  int i;

  if (pid < 1)
    return 0;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      clearjob(&jobs[i]);
      nextjid = maxjid(jobs)+1;
      return 1;
    }
  }
  return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
  int i;

  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].state == FG)
      return jobs[i].pid;
  return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
  int i;

  if (pid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid)
      return &jobs[i];
  return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
  int i;

  if (jid < 1)
    return NULL;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].jid == jid)
      return &jobs[i];
  return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
  int i;

  if (pid < 1)
    return 0;
  for (i = 0; i < MAXJOBS; i++)
    if (jobs[i].pid == pid) {
      return jobs[i].jid;
    }
  return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
  int i;

  for (i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid != 0) {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
      switch (jobs[i].state) {
        case BG: 
          printf("Running ");
          break;
        case FG: 
          printf("Foreground ");
          break;
        case ST: 
          printf("Stopped ");
          break;
        default:
          printf("listjobs: Internal error: job[%d].state=%d ", 
              i, jobs[i].state);
      }
      printf("%s", jobs[i].cmdline);
    }
  }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
  printf("Usage: shell [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
  fprintf(stdout, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
  fprintf(stdout, "%s\n", msg);
  exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
  struct sigaction action, old_action;

  action.sa_handler = handler;  
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */

  if (sigaction(signum, &action, &old_action) < 0)
    unix_error("Signal error");
  return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}



