/****************************************************************/
/* Copyright (c) 1998 Luc Van Oostenryck. All rights reserved.	*/
/*								*/
/****************************************************************/

#include <stdlib.h>		// For exit(3)
#include <unistd.h>		// For pipe(2)
#include <errno.h>
#include <sys/types.h>		// For waitpid(2)
#include <sys/wait.h>		// idem
#include <fcntl.h>		// For fcntl(2)

#include "pl-stream_impl.h"

#ifndef	HAVE_PIPE
#error  "This module need the pipe(2) system call"
#endif

/******************/
/* Pipe functions */
/******************/

static
int Swrite_pipe(Shndl_t hndl, const void *s, int n)
{ return(write(hndl.fd,s,n));
}

static
int Sread_pipe(Shndl_t hndl, void *s, int n)
{ return(read(hndl.fd,s,n));
}

static
int Sclose_pipe(pl_stream S)
{ int wstatus;
  pid_t pid;

  if (close(S->hndl.fd) < 0)
    return(-1);

  do { pid = waitpid(S->pid, &wstatus, 0);
     }
  while (pid == -1 && errno == EINTR);

  if (pid == -1)
    return(-1);

  return(wstatus);			// FIXME : is this adequate ??
}

static
int open_pipe(const char *cmd, Smode_t mode, pid_t *pid_p)
{ int	fd[2];
  int child_fd, parent_fd, std_fd;
  pid_t	pid;

/* open the pipe */
  if (pipe(fd) < 0)
    return -1;

  if (mode == SM_WRITE)
  { child_fd  = fd[0];
    parent_fd = fd[1];
    std_fd    = STDIN_FILENO;
  }
  else
  if (mode == SM_READ)
  { child_fd  = fd[1];
    parent_fd = fd[0];
    std_fd    = STDOUT_FILENO;
  }
  else
    return(-1);

/* forking */
  pid = fork();
  if (pid < 0)
  { /* fork failed */
    close(child_fd);
    close(parent_fd);
    return(-1);
  }
  else
  if (pid == 0)
  { /* Child */

    close(parent_fd);

    if (child_fd != std_fd)
    { if (dup2(child_fd,std_fd) < 0)
        _exit(127);
      close(child_fd);
    }

    execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
    _exit(127);
  }
  else
  { /* Parent */

    close(child_fd);
    fcntl(parent_fd, F_SETFD, FD_CLOEXEC);

    *pid_p = pid;
    return(parent_fd);
  }

  // return(0);	// never reached : Make compiler happy
}

Sfun_t pipe_functions =
{ Sread_pipe,
  Swrite_pipe,
  Sclose_pipe,
  0,
  0
};

// PRE  : mode == SM_READ || mode == SM_WRITE
pl_stream Sopen_pipe(const char *cmd, Smode_t mode, int flags)
{ pl_stream	S;
  int fd;
  pid_t	pid;

/* some check */
  if (cmd == 0)
    return(0);
  if (mode != SM_READ && mode != SM_WRITE)
  { PL_warn("Cannot open a pipe in `append' mode");
    return(0);
  }

  fd = open_pipe(cmd, mode, &pid);
  if ( fd == -1 || !(S=Snew_stream()))
  { // FIXME : errmsg
    return(0);
  }

// FIXME : initialize
  S->hndl.fd = fd;
  S->pid = pid;
  S->type = ST_PIPE;
  S->mode = mode;
  S->funs = &pipe_functions;

  if ( !S_setbuf(S,0,0,(flags & SF_BUFFERING)))
    return(0);

  flags &= ~(SF_BUFFERING);
  S->flags |= flags;

  return(S);
}


