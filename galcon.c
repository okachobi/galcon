#define VERSION "linux-586-elf" 
#define REVDATE "8/05/96"
#define AUTHOR  "Byron Guernsey"

/*
 * GALCON.C -- Galactic Conquest
 *
 * Version update Notes
 *
 * v2.2	 by Stan Bartsch (date unknown)
 * v2.21 by David E. Brooks Jr (29-Dec-86)
 *       - Changed displays to allow names to be displayed instead
 *         of player's numbers.  Also, added <K>wick command, Convienient
 *         means of changing certain game parameters and fixed bug in
 *         root() function.  Time displays were changed to a shorter format.
 *
 * v2.22 by David E. Brooks Jr (16-Jan-87)
 *	 - Added the Zend (send) command.
 * 
 * v2.23 by David E. Brooks Jr (11-Mar-87)
 *       - Changed initialization code to deny entrance to the game
 *         after the first 20 minutes have elapsed.
 *
 * v3.00 by Todd Ambrose (July 1988)
 *       - Added VT100 mode, accessed by the 'V' command. Added
 *         both long and short galaxy maps accessed by 'L' and 'R'.
 *         Made WARP fleets that arrive 1/2 the time a normal fleet
 *         takes. Added pirate fleets that randomly attack certain
 *         fleets. Added a private msg toggle for all prv msgs
 *         to send to a specific person. Also added Harold's
 *         WHO, and an auto-clear for detached players.
 *         And, added a penalty for returning players.
 * v3.5 by Jim Hall (Feb 1989)
 *      -  Added the ansi color codes for cga and ega users.
 *         Uses the ; command to display ansi colors on the galaxy map
 * v3.6 by Jim Hall (June 1989)  
 *         Tournment game - made game 60 min and limited players to 2
 *         Created new reslib called  star  and compiled 2nd version
 *         as Tourn.c

*
* v4.0a Modifications by Byron Guernsey (18-June-89)
*	o Repaired 39 planet unconditional win soplayers are booted upon win
*	o Added VTUPDATE(x) and code to make use of this map update feature
*	  which updates a single planet.
*	o Added Obser option which can be used to spectate either tourney
*	  or normal games.
*	o Added PSET() function and rewrote PLUPD() to make use of the function
*	  and inserted an '*' command to toggle AUTO-UPDATE on and off so
*	  that the Planets Owned list may be automatically updated or switched
*	  off. Note that using 'Y' or 'I' will automatically switch '*' off
*	  Rewrote some display routines to use PSET() and display all planets
*	o Repaired entry code so game cannot crash during entry unless players
*	  enter extremely close together like within the same 1/4 of a second
*   o Notes: Auto player eject code for non-active players may need repaired
*	  so it disqualifys only detatched users. (not Hard)
*   o Corrected Jim Halls misspelling of "Tourament" to "Tournament" (grin)
*
* ------------
* v5.0  Rewritten for Unix by Rob Miracle (8-3-89)
* v5.1a Rob Miracle (8-13-89)
*       o Added better planet updateing (AWETOE now updates ever 30 seconds)
*       o Added Help command
*       o Worked on the clear game code.
*
* v5.2 Byron Guernsey 5/30/92
*	o Removed Blinking attributes
*	o Forced QUIT signal to be ignored
* v6.0 Byron Guernsey 2/3/16
*       o Various minor changes to compile on modern Linux (ubunutu 14.04)
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <termio.h>
#include <malloc.h>
#include <string.h>

#define GAMESGRP   100
#define GALCONSEG  1002
#define ROOT       0
#define ROB        502
#define JIM        501


struct _sm {
  long  start;
  long  alive[8];
  int   xpos[40],
        ypos[40],
        zpos[40],
        owner[40],
        produ[40],
        ships[40],
        game,
        twicej[15],
        twiceg[15],
        meslok,
        mesnum,
        combat[10][2],
        comnum,
        pppn[20],
        pnppn[20];
  long  lstup[39],
        credit[10];
  char  names[10][20],
        public[5][100],
        status[10],
        scaner[16][16];
} *sm;


int shmid;


/*Declare all my global variables */

int  inchar,
     stack[99][2],
     toggle,
     twice,
     pirsiz,
     pirflt;
long events[99],
     pirtim;
int  myp,
     mypn,
     mykb;
char watch,
     vtmode,
     ansi,
     vtloo,
     mymes,
     mycom,
     mynum,
     home;
char awetow;
int  numpl[10],
     numpr[10],
     numsh[10];
char scr[80], 
     scr2[30];
long game_len,
     lsttim;
char x_siz,
     y_siz,
     z_siz;
long prod_time, 
     trav_time;

int  GID;
int  UID;
int  flags,flagsave;
struct termios tdes;
struct termios tsave;
char inbuf[2];
int  inlen;


initterm() {
  flags = fcntl(0,F_GETFL,0);
  flagsave = flags;
  tcgetattr(0, &tdes);
  tcgetattr(0, &tsave);

  tdes.c_lflag &= ~ICANON;
  tdes.c_cc[VMIN] = 1;
  tdes.c_cc[VTIME] = 2;
  
  tcsetattr(0, TCSANOW, &tdes);
  fcntl(0,F_SETFL,flags & ~O_NDELAY);
}

restterm() {
  fcntl(0,F_SETFL,flagsave);
  tcsetattr(0, TCSANOW, &tsave);
}

char getch() {
  char inchar;

  fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);
  inbuf[0] = 0;
  inchar = 0;
  inlen = read(fileno(stdin),inbuf,1);
  if(inlen <= 0) inchar = -1;
  else inchar = inbuf[0];
  fcntl(fileno(stdin), F_SETFL, flags);
  return(inchar);
}

char getchr() {
  char inchar;
  
  fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);

  while(read(fileno(stdin),inbuf,1) <= 0);
  fcntl(fileno(stdin), F_SETFL, flags);
  inchar = inbuf[0];
  return(inchar);
}

/*
 *
 */
struct _sm *get_main_region() {
  struct shmid_ds buf;
  int uid;
  int gid;

  uid = getuid();
  gid = getgid();

  setuid(0);
  setgid(0);

  shmid = shmget(GALCONSEG,sizeof(struct _sm),0770 |IPC_CREAT|IPC_EXCL);
  if(shmid < 0) {
    printf("\nGalcon Segment available.  Attaching\n");
    shmid = shmget(GALCONSEG,sizeof(struct _sm),0);
  } else {
    printf("\nCreating new Galcon segment...");
    shmctl(shmid,IPC_STAT,&(buf));
    buf.shm_perm.gid = GAMESGRP;
    if(shmctl(shmid,IPC_SET,&(buf))) {
      perror("Can't change GID of Main");
      exit(0);
    }
  }
  setuid(uid);
  setgid(gid);
  return((struct _sm *)shmat(shmid,0,0));
}

/*
 * detach
 */
void detach() {
  shmdt((char *) sm);
}

/*
 *
 */
char *sdate(value)
  long value; {
  long timex,whole,decimal;
  char *ptr;
  static char buf[30];

  if(value == 0L) timex = time(0) - sm->start;
  else timex = value - sm->start;
  whole = timex/60L;
  decimal = timex%60L;
  sprintf(buf,"%02ld.%02ld",whole,decimal);
  return(buf);
}

/*
 *
 */
setrand() {
  srand(getpid()+time(0));
}

/*
 *
 */
int irand(val)
  int val; {
    
  return(rand() % val);
}

/*
 *
 */
int prandom(range)
  int range; {
  return(rand() % range + 1);
}


/*
 *
 */
chkend() {
  int i,j,k;

  j = 0;
  k = 0;
  if(!watch) {
    for(i = 0;i < 39;i++)
       if(sm->owner[i] == mynum) j++;
    for(i = 0;i < 99;i++)
       if(stack[i][0] != 0) k++;
    if(j == 0 && k == 0) {
      printf("\nYou have lost this round.\n");
      i=0;
      while(sm->pppn[i] != 0) 
        i++;
      sm->pppn[i] = myp;
      sm->pnppn[i] = mypn;
      restterm();
      exit(0);
    }
    if(j==39) {
      printf("\nYou have completely won the war!!\n");
      sm->start=0;
      watch=1;
      sm->game=0;
      chkend();
    }
  }
  
  if(time(0) < sm->start+game_len) return(0);
  
  if(vtmode) {
    nrmscr();
    cls();
  }
  printf("\nThe War is Over. . .");
  printf("\nThe points break down as follows:\n");
  for(i=0;i<10;i++) {
    numpl[i]=0; numpr[i]=0; numsh[0]=0;
  }
  for(i=0;i<39;i++)
    if(sm->owner[i]!=-1) {
      numpl[sm->owner[i]]++; 
      numpr[sm->owner[i]] = numpr[sm->owner[i]] + sm->produ[i];
      numsh[sm->owner[i]] = numsh[sm->owner[i]] + sm->ships[i]; 
    }
  for(i=0;i<10;i++)
    if((sm->names[i][0] != '\0') && (sm->names[i][0] != 1))
      printf("%20s] Planets:%3d production:%4d Ships:%5d  Score:%6d\n",
        sm->names[i],numpl[i],numpr[i],numsh[i],
        (numpl[i]*10+numpr[i]*5+numsh[i]));
  sm->game--;
  restterm();
  exit(0);
}

/*
 * who() -- Display the names and PPN's of all player's in the game
 */
who() {
  int i;

  for(i = 0; i < 8; i++) {
    if (sm->names[i][0] != '\0')
      printf("Player #%d %20s]\n",i,sm->names[i]);
  }
  printf("\n* Denotes an observer\n");
}

/*
 * prompt() -- Display a prompt
 */
prompt() {
  if(vtmode) gotoxy(14,16);
  else printf("\nCommand: ");
  fflush(stdout);
}

/*
 *
 */
wrapup() {
  int i,k;

  if(sm->names[mynum][0] != 1) {
    lock();
    k = sm->mesnum + 1;
    if(k > 4) k = 0;
    if(!watch) {
      sprintf(sm->public[k],"  #%2d %s] has left the battle.",
        mynum, sm->names[mynum]);
      sm->public[k][1] = 'X';
    }
    else {
      sprintf(sm->public[k],"  #%2d %s] has terminated observation.",
        mynum, sm->names[mynum]);
        sm->public[k][1] = 'O';
    }
    sm->public[k][0] = mynum;
    sm->mesnum = k;
    sm->meslok--;
  }
  if(vtmode) {
    nrmscr();
    cls();
  }
  if(!twice && !sm->twicej[14] && !watch){
    i=0;
    while(sm->twicej[i]) i++;
    sm->twicej[i] = myp;
    sm->twiceg[i] = mypn;
  }
  for(i = 0;i < 39;i++) 
    if(sm->owner[i] == mynum) {
      sm->owner[i] = -1;
      if(sm->produ[i] != 10)
        sm->produ[i] = prandom(10);
    }
  sm->names[mynum][0] = '\0';

  sm->game--;
  detach();
  offprivs();
  restterm();
  printf("\nBye Bye\n");
  exit(1);
}

/*
 *
 */
individual() {
  int i,j;
 
  printf("\nSummary of player #:");
  i = getnum(&j);
  printf("\n");
  if(j > 0 && j < 10) 
    printf("Name: %s]\n",sm->names[j]);
  for(i = 0;i < 39;i++)
    if(sm->owner[i] == j)
      printf("Planet #%2d  Distance from #%2d = %4d\n",
       i,home,prange(home,i));

  if(sm->owner[home]!=mynum)
    printf("\nNOTE: You no longer control your home world!\n");
}

/*
 * Initialize the Game Board
 */
setup() {
  int i,temp;
  int xx,yy;
  int locx,locy,j;
  char tmp[16][16];
  
  sm->mesnum = sm->comnum = sm->meslok = 0;
  sm->start = time(0);
  for(i = 0;i < 10;i++)
    sm->names[i][0] = '\0';
  for(i = 0;i < 20;i++)
    sm->pppn[i] = 0;
  for(i = 0;i < 15;i++)
    sm->twicej[i] = 0;
 
/* Clear Galaxy */
  for(i=1;i<16;i++) {
    for(j=1;j<16;j++) {
      sm->scaner[i][j] = -1;
      tmp[i][j]=0;
    }
  }
  for(i = 0;i < 39;i++) {
    sm->owner[i] = -1;
    /* This portion checks to affirm that no other planets are in the    */
    /* same position in the galaxy. It may slow the program setup a bit  */
    temp=0;
    while(!temp){
      xx  = prandom(x_siz);
      yy  = prandom(y_siz);
      if(tmp[xx+1][yy+1] == 0) {
        sm->xpos[i] = xx;
        sm->ypos[i] = yy;
        tmp[xx+1][yy+1] = 1;
        temp = 1;
      }
    }
    temp = 0;
    locx = sm->xpos[i];
    locy = sm->ypos[i];
    sm->zpos[i] = prandom(z_siz);
    sm->produ[i] = prandom(10);
    sm->lstup[i] = time(0);
    sm->ships[i] = prandom(sm->produ[i]*25);
    sm->scaner[locx][locy] = i;
  } 
  /* Get 4 Planets RANDOMLY and make their prod to 25 */
  for (i = 0;i < 4;i++){
    j = prandom(39);
    sm->produ[j] = 20;
  }
  for(i = 0;i < 10;i++)
     sm->names[i][0] = '\0';
} 

/*
 *
 */
area() {
  int base,vol,i;

  printf("\nBase Planet: ");
  i = getnum(&base);
  if(base < 0 || base > 38)
     return(0);
  printf("Range: ");
  i = getnum(&vol);

  for(i = 0;i < 39;i++)
   if(prange(base,i) <= vol) {
      printf("Planet #%3d Range=%3d ",i,prange(base,i));
      if(sm->owner[i] == (-1))
        printf("Own= *Neutral*\n");
      else
        printf("Own = %s]\n",sm->names[sm->owner[i]]);
   }
}
  
/*
 *
 */
update() {
  long now;
  int i,j,k;
 
  now = time(0);
  sm->alive[mynum] = now;
  for(i = 0;i < 8;i++){
    if(sm->alive[i] < (now-420L) && sm->alive[i] != 0 && 
       sm->names[i][0] != '\0') {
      sm->alive[i] = 0;
      for(j=0;j<38;j++) 
        if(sm->owner[j] == i) sm->owner[j] = -1;
      lock();
      k = sm->mesnum + 1;
      if(k > 4) k = 0;
      sprintf(sm->public[k],"  %s] has been cleared.",sm->names[i]);
      sm->public[k][1] = 'X';
      sm->public[k][0] = 8;
      sm->mesnum = k;
      sm->meslok--;
      sm->names[i][0] = '\0';
      sm->game--;
    }
  }

  for(i=0;i<39;i++) {
    if(sm->lstup[i] < (now-45L)) {
      sm->lstup[i] = now;
      sm->ships[i] = sm->ships[i] + sm->produ[i]*1;
      if(sm->owner[i] == -1 && sm->ships[i] > 250) sm->ships[i]=250;
      if(sm->ships[i] > 50*sm->produ[i] && sm->produ[i] < 10) sm->produ[i]++;
      if(sm->ships[i] > 1500) sm->ships[i]=1500;
    }
  }
}

/*
 *
 */
galaxy() {
  int i;

  printf("\nGalaxy status at %s\n\n",sdate(0L));
  for (i=0;i<39;i++) {
    printf("#%2d (%2d,%2d) ",i, sm->xpos[i], sm->ypos[i]);
    if (sm->owner[i] == (-1))
      printf("*Neutral*\n");
    else
      printf("%s]\n",sm->names[sm->owner[i]]);
  }
  printf("\n");
}

/*
 *
 */
oneplanet() {
  int i;
  
  i = -1;
  while(i < 0 || i > 38) {
    printf("\nWhich Planet: ");
    getnum(&i);
  }
  if(sm->owner[i] == (-1))
    printf("#%2d owner= *Neutral*\n",i);
  else
    printf("#%2d owner= %s] ",i,sm->names[sm->owner[i]]);
  if(sm->owner[i]==mynum)
    printf(" Prod:%3d Ships:%5d\n",sm->produ[i],sm->ships[i]);
  else
    printf("\n");
}

/*
 *
 */
fleets() {
 int i,j,k,l,m,flg;
 int despla,desown,desarm,myarm;
 long now;
 
 now=time(0);
 if(pirtim != 0L && pirtim < now) {
   pirtim=0L;
   if(vtmode) gotoxy(24,1);
   printf("\nYour fleet #%d has been attacked by pirates!\n",pirflt);
   printf("Your fleet has: %d\n",stack[pirflt][0]);
   printf("Pirate's fleet: %d\n",pirsiz);
   if(pirsiz>stack[pirflt][0]) {
     printf("Your fleet has been destroyed!\n");
     events[pirflt]=0L;
   }
   else {
     stack[pirflt][0]=stack[pirflt][0]-(pirsiz/3);
     printf("%d ships escaped from the pirates!\n",stack[pirflt][0]);
   }
   pirsiz=0;
   if(vtmode) prompt();
 }
 for(i=0;i<99;i++)
   if(events[i]!=0L)
      if(events[i]<now) {
        myarm=stack[i][0];
        despla=stack[i][1];
        desown=sm->owner[despla];
        desarm=sm->ships[despla];
        if(mynum==desown) {
          sm->ships[despla]=desarm+myarm;
          if(!awetow) {
            if(vtmode) gotoxy(24,1);
            printf("\nPlanet %d receives reinforcements\n",despla);
          }
          else
            if(vtmode && awetow) { 
              pset(despla); 
              prompt(); 
            }
        }
        else {
          if(vtmode) gotoxy(24,1);
          printf("\nInvasion alert! Your fleet arrived at %d",despla);
          printf("\nYour fleet has: %d\nDefending fleet:%d\n",myarm,desarm);
          flg=0;
          lock();
          while(myarm>0 && desarm>0) {
            l=10;
            l=l*myarm/desarm;
            if(l>18) l=18;
            if(l<3) l=3;
            if(flg=0) {
              flg=irand(5)+1;
              if(flg<=3) goto PREPARED;
            }
            while( (m=irand(myarm)+1) < myarm/2);
            for(j=0;j<m;j++)
              if(desarm>0) 
            if(irand(20)<=l) desarm--; 
PREPARED:
            if(desarm>0) {
              while( (m=irand(desarm)+1) < desarm/2);
              for(j=0;j<m;j++)
                if(myarm>0)
                  if(irand(20)>=l) myarm--;
            }
          }
          if(myarm>0) {
            printf("You WON! %d ships hold planet #%d\n",myarm,despla);
            sm->owner[despla]=mynum;
            sm->ships[despla]=myarm;
            sm->produ[despla]=sm->produ[despla]-irand(4);
            if (sm->produ[despla]<1) sm->produ[despla]=1;
            if(vtmode) { 
              vtupdate(despla); 
            }
          }
          else {
            printf("You LOST! %d enemy ships hold planet #%d\n",desarm,despla);
            sm->ships[despla]=desarm;
          }
          k = sm->comnum;
          k++;
          if(k > 9) k=0;
          sm->combat[k][0]=mynum;
          sm->combat[k][1]=despla;
          sm->comnum=k;
          sm->meslok--;
        }
        events[i]=0L;
      }
}

/*
 *
 */
lock() {
  int i;

  i = 0;
  if(sm->meslok < 0 || sm->meslok > 20) sm->meslok = 0;
  while(sm->meslok > 0 && i < 5) {
    i++;
    sleep(2);
  }
  sm->meslok = 0;
  sm->meslok++;
}

/*
 *
 */
getpub() {
  char *ptr;
  int k,type,i,tmp;

  i = 0;

  k = mymes;
  while(k != sm->mesnum) {
    k++;
    if(k > 4) k = 0;
    i = 1;
    if(sm->public[k][0] != mynum) {
      if(sm->public[k][1] == 'P' || 
         sm->public[k][1] == mynum || 
         sm->public[k][1] == 'E' || 
         sm->public[k][1] == 'X' || 
         sm->public[k][1] == 'O') {
        type=sm->public[k][1];
        if(type != 'P' && type != 'O' && type != 'X' && 
           type != 'C' && type != 'E') {
          type = 'I';
        }
        if(vtmode) gotoxy(24,1);
        if(type == 'I') {
          printf("\nPrivate message from %s]\n",
            sm->names[sm->public[k][0]]);
        } 
        else {
          if(type=='P') {
            printf("\nPublic message by %s]\n",
              sm->names[sm->public[k][0]]);
          } 
          else {
            if(type=='E' && vtmode) {
              for(tmp=0;tmp<39;tmp++) {
                if(sm->owner[tmp]==sm->public[k][0]) vtupdate(tmp);
              }
            }
            else {
              if(type=='X' && vtmode) vtscan();
              else printf("\n");
            }
          }
        }
        if(vtmode) gotoxy(24,1);
        printf("%s\n\n",sm->public[k]+2);
      }
    }
  }
  mymes = k;
  if(i && vtmode) prompt();
}

/*
 *
 */
getcom() {
  int k;
  int defpl,attak,own;

  k = mycom;
  while(k!=sm->comnum) {
    k++;
    if(k>9) k=0;
    attak=sm->combat[k][0];
    own=sm->owner[sm->combat[k][1]];
    defpl=sm->combat[k][1];
    if(attak!=mynum) {
      if(vtmode) gotoxy(24,1);
      printf("\nInvasion attempt!\nAttacker:%s]\nPlanet #%d\n",
        sm->names[attak],defpl);
      if(own==-1) printf("It remains Neutral\n");
      if(own==attak) {
        printf("%s] has gained the planet.\n",sm->names[own]);
        if(vtmode) { 
          vtupdate(defpl);  
          pset(defpl);
        }
			}
      if(own!=attak && own!=-1) 
        printf("%s] retains control.\n",sm->names[own]);
      if(own==mynum) {
        printf("%d ships still hold your planet %d\n",
          sm->ships[defpl],defpl);
        if(vtmode && awetow) 
          pset(defpl);
      }
    }
  }
  mycom=k;
  prompt();
}

/*
 *
 */
prange(i,j)
  int i,j; {
  float root();
  float truerange;
  long tmp1,tmp2,tmp3;
  int test;

  tmp1 = sm->xpos[i] - sm->xpos[j];
  tmp1 = tmp1 * tmp1;

  tmp2 = sm->ypos[i] - sm->ypos[j];
  tmp2 = tmp2 * tmp2;

  tmp3 = sm->zpos[i] - sm->zpos[j];
  tmp3 = tmp3 * tmp3;

  truerange = root(tmp1 + tmp2 + tmp3);
  test = (int) truerange;
  if((float) test < truerange) test++;
  return(test);
}

/*
 *
 */
compute() {
  int k;
  int i,j;

  printf("\nPlanet #:");
  k = getnum(&i);
  printf("\nPlanet #:");
  k = getnum(&j);
  k = prange(i,j);
  printf("Range from %d to %d = %d\n",i,j,k);
}

/*
 *
 */
float root(x)
  long x; {
  int z;
  float e;
  double y;
  
  if(x == 0) return(0.0);
  y = x / 2.0;
  z = 0;
  e = 1.0;
  while(((y * y) != x) && (e > 0.001)) {
    z++;
    y = (y + x / y) / 2;
    e = y * y - x;
    if(e < 0.0) e = -e;
  }
  return(y);
}

/*
 *
 */
flts() {
  int i, fltflag;

  fltflag = 0;
  printf("\nFleet  Dest  Size  Arrival\n");
  for(i=0;i<99;i++)
    if(events[i] != 0L) {
       fltflag = 1;
       printf("  %2d    %2d   %4d  %5s\n",
         i,stack[i][1],stack[i][0],sdate(events[i]));
    }
  if(fltflag == 0)
    printf("\nNo Fleets currently in flight.\n");
}

/*
 *
 */ 
broadcast() {
  char tmp[512];
  char buf[512];
  int ptr,k;

  ptr=2;
  printf("\nEnter Broadcast Message:\n>  ");
  fflush(stdout);
  restterm();
  fgets(buf, 511, stdin);
  initterm();
  tmp[1] = 'P';
  tmp[0] = mynum;
  tmp[2] = 0;
  buf[97] = 0;
  lock();
  k = sm->mesnum+1;
  if(k > 4) k=0;
  sm->public[k][0] = tmp[0];
  sm->public[k][1] = tmp[1];
  strcpy(sm->public[k]+2,buf);
  sm->mesnum = k;
  sm->meslok--;
  printf("Message Sent.\n");
}

/*
 *
 */
prvmsg() {
  char tmp[100],buf[512];
  int ptr,k, player;

  ptr=2;
  printf("\nEnter Private Message:\n>  ");
  fflush(stdout);
  restterm();
  fgets(buf, 511, stdin);
  initterm();
  if(toggle>-1) player=toggle;
  else {
    printf("\nMessage to (player NUMBER):");
    k=getnum(&ptr);
    player = ptr;
  }
  if ((player < 0) || (player > 10) || (sm->names[player] == '\0')) {
    printf("Illegal player number.\n");
    return(0);
  }
  tmp[1]=player;
  tmp[0]=mynum;
  tmp[2]=0;
  buf[99] = 0;
 
  lock();
  k=sm->mesnum+1;
  if(k>4)k=0;
  sm->public[k][0] = tmp[0];
  sm->public[k][1] = tmp[1];
  strcpy(sm->public[k]+2,buf);
  sm->mesnum=k;
  sm->meslok--;
  printf("\nMessage Sent to %s.\n",sm->names[player]);
}

/*
 *
 */
send() {
  int i,j,k;
  int from,to;

  i = -1;
  while(i == -1) {
    printf("\nFrom world:");
    i = getnum(&from);
    if(from < 0 || from > 38) i = -1;
  }
  if(sm->owner[from]!=mynum) {
    printf("\nYou do not own that world!\n");
    return(0);
  }
  j = -1;
  while(j == -1) {
    printf("To world:");
    j = getnum(&to);
    if(to < 0 || to > 38) j = -1;
  }
  
  for (k=0;events[k]!=0L && k<100; k++);
  if(k == 100) {
    printf("\nSorry, too many fleets in the air!\n");
    return(0);
  }
  
  printf("Number to send (%d) :",sm->ships[from]);
  i = getnum(&j);
 
  if(j>sm->ships[from] || j<1) {
    printf("Invalid fleet size!");
    return(0);
  }
  
  if(sm->owner[from]!=mynum) {
    printf("\nYou have lost control of that world!!!\n");
    return(0);
  }

  stack[k][0]=j;
  sm->ships[from]=sm->ships[from]-j;
  stack[k][1]=to;

  events[k]=time(0)+prange(from,to)*trav_time;
  printf("Fleet will arrive at %s\n",sdate(events[k]));
  if(vtmode && awetow) pset(from);
  pirate(k);
}
 
/*
 * kwicklist command
 */
kwick() {
  int idx;

  printf("Kwick list:\n\n");
  for(idx = 0; idx <= 38; idx++) {
    if (sm->owner[idx] == mynum)
      printf("#%2d Prod:%3d Ships:%5d\n",idx,sm->produ[idx],sm->ships[idx]);
  }
}

/*
 * fetch a number from the keyboard
 */
int getnum(x)
  int *x; {
  char buffer[99];
  int flg,k,j;

  flg = -1;
  fflush(stdout);
  while(flg == -1) {
    flg=0;
    restterm();
    fgets(buffer, 98, stdin);
    initterm();
    if(atoi(buffer) == 0 && buffer[0] != '0') {
      flg = -1;
      printf("\nA Number is Expected\n");
      fflush(stdout);
    }
  }
  j = sscanf(buffer,"%d",x);
}

/*
 * getppn() -- fetch the user's PPN number 
 */

getppn(xx,yy,zz)
  int *xx, *yy, *zz;  {
/*
 *  Set the _PROJ and _PROG values
 */

  char *buffy;
 
  *xx = getgid();
  *yy = getuid();
  buffy = (char *) ttyname(0);
  if(strcmp(buffy,"console") == 0) *zz = 0;
  else *zz = atoi(buffy+8);
}

/*
 * zend() -- send a message to a selected keyboard.  Added at the request
 *           of Jim Hall
 */
zend() {
  int keyboard;
  char msg[1024], zendit[1024];
  char tty[13];

/* Okay, fetch the keyboard # to zend to */
  gotoxy(24,1);
  printf("Send to (tty): ");
  fflush(stdout);
  fgets(tty, 12, stdin); 
  if(tty[0] == 0)
    return(0);   /* no go if null string */
  printf("Enter your message\n>");
  fflush(stdout);
  fgets(msg, 1023, stdin); 
  sprintf(zendit,"/usr/local/bin/send %s %c%s%c",tty,34,msg,34);
  system(zendit);
  return(0);
}

/*
 *
 */
clrgam() {
  char text[100];

  fflush(stdout);
  if(UID == JIM || UID == ROB || UID == ROOT) {
    printf("(Cleargame): Are you sure? ");
    fflush(stdout);
    fgets(text, 99, stdin);
    if(!strcmp(text,"YES")) {
      sm->start = 0;
    }
  }
}

/*
 * scan() - Displays map in nonvt mode.
 */
scan() {
  int x,y;
  for(y=1;y<15;y++) {
    for(x=1;x<16;x++) {
      putchar(' ');
      if(sm->scaner[x][y]<0) printf(". ");
      else printf("%2d",sm->scaner[x][y]);
    }
  putchar('\n');
  }
}

/*
 * setit() - VTMODE Screen Setup.
 */
setit() {
  int i;
  cls();
  for(i=1;i<15;i++){
    gotoxy(i,36);
    printf(".  .  .  .  .  .  .  .  .  .  .  .  .  .  .");
  }
  gotoxy(14,1);
  printf("=============>  =====[     ]=======");
}

/*
 * updtim() - Update gametime in VTMODE.
 */

updtim() {
  gotoxy(14,23);
  printf("%s",sdate(0L));
}

/*
 *
 */
cls() {
  gotoxy(1,1);
  printf("\033[J");
}

/*
 *
 */
gotoxy(x,y)
  char x,y; {

  printf("\033[%d;%dH",x,y);
  return(0);
}

/*
 *
 */
setscr() {
  printf("\033[15;24r");
}

/*
 *
 */
nrmscr() {
  printf("\033[0;24r");
}

/*
 * autosd() - Sends multiple fleets from all planets to a certain
 *            planet.
 */

autosd() {
  int k,i,to,j,howmch;
  int cd[40]; /* Record of planets sent from */

  i = -1;
  while(i == -1) {
    printf("Autosend to which planet: ");
    i=getnum(&to);
    if(to<0 || to>38) i = -1;
  }
  for(k=0;k<39;k++) {
    cd[k]=0;
    if(sm->owner[k]==mynum && k!=to) {
      cd[k]=1;
      i = -1;
      while(i== -1) {
        printf("#%2d - %4d: ",k,sm->ships[k]);
        i = getnum(&howmch);
        if(howmch<0 || howmch>sm->ships[k]) i= -1;
        else 
          if(howmch>0) {
            for (j=0;events[j]!=0L && j<100; j++);
            if(j==100) {
              printf("\nSorry, too many fleets in the air!\n");
              return(0);
            }
            if(sm->owner[k]!=mynum) printf("\nYou have lost control of world #%d\n",k);
            else {       
              stack[j][0]=howmch;
              sm->ships[k]=sm->ships[k]-howmch;
              stack[j][1]=to;
              events[j]=time(0)+prange(k,to)*trav_time;
              printf("Fleet will arrive at %s\n",sdate(events[j]));
              pirate(j);
            }
          }
      }
    }
  }
  if(vtmode && awetow) {
    for(k=0;k<39;k++) { 
      if(cd[k]) pset(k); 
    }
  }
}

/*
 *
 */
blink() {
  printf("\033[5m");
	return(0);
}

/*
 *
 */
dim() {
  if (vtloo) {
    printf("\033[0m");
  }
  else {
    printf("\033[2m");
    printf("\033[8m");
  }
}

/*
 *
 */
normal() {
  printf("\033[0m");
  if (vtloo) printf("\033[1m");
	return(0);
}

/*
 * prvtog() - Toggles who to send private messages to.
 */
prvtog() {
  if(toggle>-1) {
    toggle= -1;
    printf("Private message toggle is off.\n");
    return(0);
  }
  do {
    printf("Enter user number to private msg to: ");
    getnum(&toggle);
    if(toggle<0 || toggle>7) toggle= -1;
  } while(toggle<0);
  printf("Toggle set to send to %s]",sm->names[toggle]);
}

/*
 * warp() - Send warp fleets. Arrive 1/2 the time a normal fleet
 *          takes, and uses 10% of the fleet.
 */
warp() {
  int i,j,k;
  int from,to;

  i= -1;
  while(i == -1) {
    printf("\nWarp from world:");
    i = getnum(&from);
    if(from <0 || from >38) i= -1;
  }
  if(sm->owner[from]!=mynum) {
    printf("\nYou do not own that world!\n");
    return(0);
  }
  j = -1;
  while(j == -1) {
    printf("Warp to world:");
    j = getnum(&to);
    if(to < 0 || to > 38) j= -1;
  }
  
  for(k=0;events[k]!=0L && k<100; k++);
  
  if(k==100) {
    printf("\nSorry, too many fleets in the air!\n");
    return(0);
  }

  printf("Number to send (%d) :",sm->ships[from]);
  i = getnum(&j);

  if(j > sm->ships[from] || j < 100) {
     printf("Invalid fleet size!");
     return(0);
  }
  
  if(sm->owner[from]!=mynum) {
    printf("\nYou have lost control of that world!!!\n");
    return(0);
  }
  
  stack[k][0] = j - j / 10;
  sm->ships[from] = sm->ships[from] - j;
  stack[k][1] = to;
  
  events[k] = time(0) + prange(from,to) * (trav_time/2);
  printf("Warp fleet will arrive at %s carrying %d ships\n",
    sdate(events[k]),j - j / 10);
  if(vtmode && awetow) pset(from);
} 
/*
 * plupd() - Lists player's planets in VTMODE
 */
plupd() {
  int x,y,i;
  clrjnk();
  normal();
  for(i=0;i<39;i++) {
    if(i < 13) { 
      x = i + 1;
      y = 1;
    }
    else 
      if(i < 26) { 
        x = i - 12;  
        y = 13;
      }
      else { 
        x = i - 25;  
        y = 25;
      }
    gotoxy(x,y);
    printf("%2d)",i);
    if(sm->owner[i]==mynum) pset(i);
  }
  prompt();
}

/*
 * clrjnk() - Clear out the left side box in VTMODE
 */
clrjnk() {
  int i;
  for(i=0;i<14;i++) {
    gotoxy(i,35);
    printf("\033[1K");
  }
  gotoxy(14,1);
  printf("\033[1K");
}

/*
 * PSET() Update a single planet within the Planet/Prod/Ship chart 
 */
pset(pl)
  int pl; {
  int x,y;

  if(pl < 13) {
    x = pl + 1;
    y = 4;
  }
  else 
    if(pl < 26) { 
      x = pl - 12;
      y = 16; 
    }
    else { 
      x= pl - 25;
      y = 28;
    }
  gotoxy(x,y);
  if(sm->owner[pl]==mynum) {
    if(ansi) {
      if(sm->ships[pl]<=0) { 
        printf("\033[31m"); 
        /*blink(); */
      }
      else 
        if(sm->ships[pl]>=1 && sm->ships[pl]<200) {
          normal();
          printf("\033[31m");
        } else
        if(sm->ships[pl]>=200 && sm->ships[pl]<1500) { 
          normal();  
          printf("\033[35m");
        }
        else { 
          normal();  
          printf("\033[32m"); 
        }
    }
    else {
      if(sm->ships[pl]<=0) { 
        dim(); 
        /*blink(); */
      }
      else 
        if(sm->ships[pl]>=1 && sm->ships[pl]<1500) dim();
        else normal();
    }
    printf("%2d %4d",sm->produ[pl],sm->ships[pl]);
  }
  else printf("       ");
  normal();
}
 
/*
 * vtscan() - Shows the map in VTMODE
 */
vtscan() {
  char x;
  int i;
  
  for(i=0;i<39;i++) {
    gotoxy(sm->ypos[i],36+(3*sm->xpos[i])-3);
    if(ansi) {
      x = 31 + sm->owner[i];
      if(x > 36) x = 36;
      printf("\033[%dm",x);
      if(x<31) normal();
    }
    else {
      dim();
      if(sm->owner[i]==mynum) normal();
      else 
        if(sm->owner[i]>-1) blink();
      if (sm->owner[i] == -1 ) dim();
    }
    printf("%2d",i);
    normal();
  }
  dim();
  normal();
  prompt();
}

/*
 * vtflts() - Shows the fleets in the left side box in VTMODE.
 */
vtflts() {
  int i,j,col,row;
  clrjnk();

  gotoxy(1,1);
  printf("Dst  Size  Arriv  Dst  Size  Arriv");
  col=1;
  row=1;
  j=0;
  for(i=0;i<99;i++) {
    if(events[i]!=0L) {
      j++;
      if(j < 25) {
        row++;
        if(row > 13) {
          row=2;
          col=col+18;
        }
        gotoxy(row,col);
        printf(" %2d  %4d  %s",stack[i][1],stack[i][0],sdate(events[i]));
      }
    }
  }
  gotoxy(14,34);
  if(j>24) {
    blink();
    putchar('*');
    normal();
  }
  else putchar('=');
  prompt();
}

/*
 * shtscn() - Short range map for 40 col users.
 */
shtscn() {
  int pl,xstrt,xend,ystrt,yend,x,y;

  pl= -1;
  while(pl== -1) {
    printf("Enter planet: ");
    getnum(&pl);
    if(pl<0 || pl>38) pl= -1;
  }
  xstrt=sm->xpos[pl]-5;
  xend=sm->xpos[pl]+5;
  if(xstrt<1) {
    xend=xend+(1-xstrt);
    xstrt=1;
  }
  if(xend>15) {
    xstrt=xstrt-(xend-15);
    xend=15;
  }
  ystrt=sm->ypos[pl]-5;
  yend=sm->ypos[pl]+5;
  if(ystrt<1){
    yend=yend+(1-ystrt);
    ystrt=1;
  }
  if(yend>14){
    ystrt=ystrt-(yend-14);
    yend=14;
  }
  for(y=ystrt;y<yend+1;y++){
    for(x=xstrt;x<xend+1;x++){
      putchar(' ');
      if(sm->scaner[x][y]== -1) printf(". ");
      else printf("%2d",sm->scaner[x][y]);
    }
    putchar('\n');
  }
}

/*
 * vtindiv() - Lists a player's planets in VTMODE.
 */
vtindiv() {
  int i,j,b,col,row;

  j= -1;
  while(j== -1) {
    printf("Summary of player: ");
    getnum(&j);
    if(j<0 || j>7) j= -1;
  }
  clrjnk();
  gotoxy(1,1);
  printf("Summary of %s]\n\n",sm->names[j]);
  printf("Planet  Planet  Planet  Planet");
  col=3;
  row=3;
  b=0;
  for(i=0;i<39;i++){
    if(sm->owner[i]==j){
      b++;
      if(b>10){
        b=1;
        row=3;
        col=col+8;
      }
      row++;
      gotoxy(row,col);
      printf("#%2d",i);
    }
  }
  prompt();
}

/*
 * pirate() - Random pirate fleets.
 */
pirate(i)
  int i; {
  int ispir;
  
  ispir=irand(39);
  if(pirtim!=0L) return(0);
  if(ispir!=25) return(0);
  while(pirsiz<20) pirsiz=irand(80);
  pirflt=i;
  pirtim=events[i]-20L;
}

/*
 * syswho() - Harold's WHO
 */
syswho() {
  system("/usr/local/bin/who");
}

/*
 * vtupdate(x) - updates a single planet x, on the map when vtmode is on 
 * where x is an integer between 0 and 38.
 */
vtupdate(x)
  int x; {
	char i;
  
  gotoxy(sm->ypos[x],36+(3*sm->xpos[x])-3);
	if(ansi){
    i=31+sm->owner[x];
		if(i>36) i=36;
		printf("\033[%dm",i);
		if(i<31) normal();
  }
  else {
		dim();
    if(sm->owner[x]==mynum) normal();
    else if(sm->owner[x]>-1) blink();
    if(sm->owner[x] == -1) dim();
  }
  printf("%2d",x);
	normal();
  if(sm->owner[x]==mynum && awetow!=0) pset(x);
	prompt();
}

/* 
 * onprivs()
 */
onprivs() {
  setgid(100);
}

/*
 * offprivs()
 */
offprivs() { 
  setgid(GID);
}

helpme() {
  gotoxy(24,1);
  printf("Quick Help\n");
  printf("a - area      b - broadcast  c - compute    d - warp send  f - fleets\n");
  printf("g - galaxy    h - help       i - one player j - system who k - kwik list\n");
  printf("l - scanner   m - pvt toggle n - no VT100   o - one planet p - pvt msg\n");
  printf("q - quit      r - short scan s - send fleet t - time       u - update\n");
  printf("v - VT100 md  w - who        x - auto send  * - auto update toggle\n");
  printf("; - VT100Colr ? = dim vt100  e - Cleargame  n - Nonvt100\n");
  prompt();
}

/*
 *
 */
void trap(int sig) {
  wrapup();
}

/*
 *
 */
main() {
  long atol();
  long now;
  char *ctime();
  int i,j,k;
  long years;
  long ii,udtm;

  GID = getgid();
  UID = getuid();
  onprivs();
  signal(SIGINT, trap);
  signal(SIGHUP, trap);
  /*signal(SIGQUIT,SIG_IGN);*/
  toggle= -1;
  sm = (struct _sm *) get_main_region();
  initterm();
  printf("\n");
  printf("GalCon (%s) Revised %s by %s\n",VERSION,REVDATE,AUTHOR);
  printf("\n");
 
  getppn(&myp,&mypn,&mykb);
 
  game_len  = 3600L;        /* changed to desired values and recompile */
  x_siz     = 15;
  y_siz     = 14;
  z_siz     = 1;            /* Make Galaxy 2-D */
  prod_time = 45L;
  trav_time = 30L;

  printf("Game length is %ld minutes\n",game_len/60);

  setrand();
  while(sm->game == 122) sleep(1); /* Sleep while game is being setup */
  if(sm->game < 0 || sm->game > 10) sm->game = 0;
  if(sm->meslok < 0 || sm->meslok > sm->game) sm->meslok = 0;
  if(sm->game == 0) sm->game = 122;
   /* If game is empty, set setup flag (122) and setup */
  lock();
  if(sm->game == 122 || sm->lstup[0]+300L < time(0) || sm->lstup[0] > time(0)) {
    setup();
  }
 
  for (j = 0;j < 99;j++) 
    events[j] = 0L;
   
  for(i = 0;i < 20;i++)
    if(sm->pppn[i] == myp && sm->pnppn[i] == mypn) {
      printf("\nAllowed to reenter for observation...\n");
      watch=1;
    }
  fflush(stdout);
  if(!watch) {
  	printf("\nO)bserve or P)lay Galcon: P\010");
    fflush(stdout);
    watch = getchr();
    watch = toupper(watch);
    if(watch == 'O') { 
      watch = 1; 
      putchar('O');
    }
    else watch = 0;
  }
  for(i=0;i<15;i++) {
    if(sm->twicej[i] == myp && sm->twiceg[i] == mypn) twice = 1;
  }
  mymes = sm->mesnum; 
  mycom = sm->comnum;
 
  if(sm->game == 122) /* If setup flag is set you are the first player */
    sm->game = 1;
  else
    sm->game++;
   
  home = 0;
  mynum = 0;
  for(home=0;sm->names[home][0]!='\0';home++);
  sm->names[home][0]='\n';
  mynum = home;
  home = -1;
  sm->meslok--;

  sm->alive[mynum]=0;
  k = 0;
  sm->names[mynum][k] = 1;
  while(sm->names[mynum][0]==1) {
    k=0;
    printf("\n\nEnter your name, please: ");
    fflush(stdout);
    restterm();
    fgets(scr,79, stdin);
    initterm();
    scr[9] = 0; /* force an EOS */
    for(i=0;i<9;i++)
      if(scr[i]=='*') scr[0]=42;
    if(scr[0]!='*') {
      if(!watch) {
        sprintf(scr2," %-9s [%3d,%-3d",scr,myp,mypn);
      }
      else
        sprintf(scr2,"*%-9s [%3d,%-3d",scr,myp,mypn);
			
      for (k=0; k<20; k++)
        sm->names[mynum][k] = scr2[k];
    } else  printf("\n?Name may not contain *'s.\n");
  } 
  lock();
  k = sm->mesnum + 1;
  if(k > 4) k = 0;
  if(!watch) {
    sprintf(sm->public[k],"  #%2d %s] has joined the battle.",mynum,sm->names[mynum]);
    sm->public[k][1]='E';
  }
  else {
    sprintf(sm->public[k],"  #%2d %s] has entered tournament for observation.",mynum,sm->names[mynum]);
    sm->public[k][1]='O';
  }
  sm->public[k][0]=mynum;
  sm->mesnum = k;
  printf("Help will be found in HELP GAMES GALCON.\n");
  printf("\n\n");
  home = -1;
  if(!watch) {
    while(home== -1) {
      k=irand(39);
      if (sm->owner[k]== -1) {
        sm->owner[k]=mynum;
        if(twice) sm->ships[k]=250;
        else sm->ships[k]=450;
        if(twice) sm->produ[k]=5;
        else sm->produ[k]=15;
        home=k;
      }
    }
    printf("\nYour home planet is #%d, located at (%d,%d)\n",
        home,sm->xpos[home],sm->ypos[home]);
  }
  sm->meslok--;
  prompt();
  inchar = 0;
  udtm = time(0L);
  while(1 == 1) {
    fflush(stdout);
    inchar = getch();
    if(inchar != -1) {
      if(vtmode) gotoxy(24,1);
      inchar = tolower(inchar);
      if(!vtmode || inchar!='y' && inchar!='u' && inchar!='l') putchar('\n');
    }
    switch(inchar) {
      case -1 : update();
                sleep(1);
                break;
      case 'b': broadcast(); break;
      case 'i': if(vtmode) { 
                  vtindiv(); awetow=0; 
                } else individual();
                break;
      case 'k': kwick(); break;
      case 'o': oneplanet(); break;
      case 'p': prvmsg(); break;
      case 'e': clrgam(); break;
      case 't': printf("\nCurrent time is %s\n",sdate(0L));
                years= (time(0) - sm->start)/60L;
                printf("Years           %ld\n",years);
                break;
      case 'a': area(); break;
      case 'm': prvtog(); break;
      case 'f': flts(); break;
      case 'g': galaxy(); break;
      case 'c': compute(); break;
      case 'w': who(); break;
      case 's': send(); break;
/*
      case 'z': zend(); break; */   /* [2.22] */
      case 'q': /* nodelay(stdscr,FALSE); */
                printf("\n%cAre you sure you want to Quit? ",7);
                fflush(stdout);
                k=getchr();
                k=tolower(k);
                if(k=='y') {
                  wrapup();
                }
                break;
      case 'l': if(vtmode) vtscan();
                else scan();
                break;
      case '?': if(!vtloo) vtloo=1;
                break;
      case ';': if(!ansi) ansi=1;
      case 'v': if(!vtmode){
		   vtloo=1;
                   vtmode=1;
                   setscr();
                   setit();
                   vtscan();
                   plupd();  
                   gotoxy(24,1);
                   printf("VT100 Mode Activated.\n");
                }
      case '*': if(!vtmode) {
                  printf("Autoupdate failed - Not in VT100 mode\n");
                }
                else {
                  if(!awetow) { 
                    awetow=1;
                    plupd();
                    gotoxy(24,1);
                    printf("Auto-Planet update on.\n");
                  }
                  else {  
                    awetow=0;
                    gotoxy(24,1);
                    printf("Auto-Planet update off.\n");
                  }
                }
                break;
      case 'n': if(vtmode){
                  vtmode=0;     
                  awetow=0;
                  ansi=0;
                  vtloo=0;
                  nrmscr();
                  gotoxy(24,1);
                  printf("VT100 Mode Disabled.\n");
                }
                break;
      case 'u': if(vtmode) plupd();
                break;
      case 'x': autosd(); break;
      case 'd': warp(); break;
      case 'y': if(vtmode) vtflts();        
                else flts();
                awetow=0;
                break;
      case 'r': shtscn(); break;
      case 'j': syswho(); break;
      case 'h': helpme(); break;
    }        
    fleets();
    chkend();
    if(mymes!=sm->mesnum) getpub();
    if(mycom!=sm->comnum) getcom();
    now=time(0);
    if(vtmode && lsttim<=(now-5L)){           
      lsttim=now;
      updtim();
      prompt(); 
    }
    now = time(0);
    if(vtmode && awetow && udtm <= (now - 30L)) {
      udtm = now;
      for(ii=0;ii<39;ii++) 
        if(sm->owner[ii] == mynum) pset(ii);
      prompt();
    }
    if(inchar!= -1) prompt();
  } 
}

