/****************************************************************/
/* Copyright (c) 1998 Luc Van Oostenryck. All rights reserved.	*/
/*								*/
/****************************************************************/

#include "Prolog.h"
#include "pl-pred.h"


int unify(register cell_t *d1, register cell_t *d2)
{ d1=deref(d1);

  debut:
  if (d1==d2) goto OK;			// same object

  switch(get_tag(d2))
  { case ref_tag: d2=d2->celp;
                  goto debut;
    case var_tag: if (is_var(d1) && (d1 > d2))	// youngest var
                  { mkrefp(d1,d2);
                    trail(d1);
                    goto OK;
                  }
                  else
                  { mkrefp(d2,d1);
                    trail(d2);
                    goto OK;
                  }

    case ato_tag: if (is_var(d1))
                  { mkrefp(d1,d2);
                    trail(d1);
                    goto OK;
                  }
                  else goto KO;

    case int_tag: if (d1->val==d2->val) goto OK;
                  if (is_var(d1))
                  { d1->val=d2->val;
                    trail(d1);
                    goto OK;
                  }
                  else goto KO;

    case flt_tag: if (is_var(d1))
                  { mkrefp(d1,d2);
                    trail(d1);
                    goto OK;
                  }
                  else
		  if (is_flt(d1) && (get_flt(d1)==get_flt(d2)))
                    goto OK;
                  else
                    goto KO;

    case fun_tag: if (d1->val==d2->val)
                  { if (isfun(FUN(dot,2),d2))
                    { if (!unify(d1+1,d2+1))
                        goto KO;
                      d1=deref(d1+2);
                      d2=d2+2;
                      goto debut;
                    }
                    else
                    { int n=get_arity(d2);
                      for (;n>1;n--)
		        if (!unify(++d1,++d2))
                          goto KO;

                      d1=deref(d1+1);
                      d2=d2+1;
                      goto debut;
                    }
                  }
                  else
                  if (is_var(d1))
                  { mkrefp(d1,d2);
                    trail(d1);
                    goto OK;
                  }
                  else  goto KO;

    }

  OK: return(1);
  KO: return(0);
}



void write_binding(void)
{ int i, flag=0;

  for (i=0; i<nbr_fv; i++)
    if (freevar[i] && freevar[i][0]!='_')
      { Sprintf("\n%s = ",freevar[i]);
        pl_write(STK[i+6].celp);
        flag=1;
      }

 if (!flag) Sprintf("Yes\n");
 else       Sprintf("\n");

 return;
}

#ifdef INTERACTIVE
int next_goal(void)
{ int c;
  print_time();
  c=GetSingleChar();

  get_time(&t0);

  return(c==';');
} 
#else
int next_goal(void)
{ return(0); }
#endif	// INTERACTIVE

#ifdef TIME_OF_DAY
void print_time(void)
{ double t;

  t=t1.tv_sec+(t1.tv_usec/1000000.0)-(t0.tv_sec+(t0.tv_usec/1000000.0));
  printf("time elapsed = %#.2f sec\n",t);

  return;
}
#else
void print_time(void)
{ double tu, ts;

  tu=(t1.utime-t0.utime)/1000000.0;
  ts=(t1.stime-t0.stime)/1000000.0;

  printf("time elapsed : user=%#.2f / system=%#.2f / clock=%#.2f sec\n",tu,ts,tu+ts);

  return;
}
#endif


static void *fp, *hp, *btp;
void save_regs(void)
{ fp=FP; hp=HP; btp=BTP; }
void restore_regs(void)
{ FP=fp; HP=hp; BTP=btp; }

#define _warning(fm,args...)       do { save_regs(); fflush(0); fprintf(stderr,"[Warning: " fm "]\n" , ## args); restore_regs(); return(0); } while(0)

// FIXME : add floating number
int eval_(cell_t *c, int *n)
{ int n2; //, n2;

  debut:
  switch(get_tag(c))
  { case ref_tag: c=c->celp;
                  goto debut;
    case var_tag: _warning("Unbound variable in arithmetic expression");
                  return(0);
    case ato_tag: _warning("Unknow arithmetic operator: %s/%d",AtomName(get_atom(c)),0);
                  return(0);
    case int_tag: *n=get_val(c);
		  return(1);
    case fun_tag: { fun_t f=get_fun(c);	// FIXME : hash-table
                    if (f==FUN(plus,2))
		    { if (eval_(c+1,n) && eval_(c+2,&n2))
		      { *n+=n2; succeed; }
		      else fail;
                    }
                    else
                    if (f==FUN(minus,2))
		    { if (eval_(c+1,n) && eval_(c+2,&n2))
		      { *n-=n2; succeed; }
		      else fail;
                    }
                    else
                    if (f==FUN(star,2))
		    { if (eval_(c+1,n) && eval_(c+2,&n2))
		      { *n*=n2; succeed; }
		      else fail;
                    }
                    else
                    if (f==FUN(div,2) || f==FUN(divide,2))
		    { if (eval_(c+1,n) && eval_(c+2,&n2))
		       { *n/=n2; succeed; }
		       else fail;
                    }
                    else
                    if (f==FUN(minus,1))
		    { if (eval_(c+1,n))
		      { *n=-*n; succeed; }
		      else fail;
                    }
                    else
                    if (f==FUN(_max,2))
		    { if (eval_(c+1,n) && eval_(c+2,&n2))
		      { *n=((*n>n2) ? *n : n2); succeed; }
		      else fail;
                    }
                    else
                    if (f==FUN(_min,2))
		    { if (eval_(c+1,n) && eval_(c+2,&n2))
		      { *n=((*n<n2) ? *n : n2); succeed; }
		      else fail;
                    }
                    else
                    if (f==FUN(_mod,2))
		    { if (eval_(c+1,n) && eval_(c+2,&n2))
		      { *n=*n % n2; succeed; }
		      else fail;
                    }
                    else
                      _warning("Unknow arithmetic operator: %s/%ld",FunName(f), FunArity(f));
                  }                  
  }

  fail;		// Suppress compiler warning
}  


int can_unify(cell_t *a, cell_t *b)
{ int r;
  mark_t m;
  Mark(m);
  r=unify(a,b);
  Undo(m);
  return(r);
}

int not_unify(cell_t *a, cell_t *b)
{ return(! can_unify(a,b)); }


#ifdef TIME_OF_DAY
void get_time(struct timeval *tv)
{ gettimeofday(tv,0); }
#else
#include <sys/resource.h>       // for getrusage()
void get_time(time__t *t)
{ struct rusage usage;

  getrusage(RUSAGE_SELF, &usage);

  t->utime=usage.ru_utime.tv_sec*1000000.0+usage.ru_utime.tv_usec;
  t->stime=usage.ru_stime.tv_sec*1000000.0+usage.ru_stime.tv_usec;
}
#endif
