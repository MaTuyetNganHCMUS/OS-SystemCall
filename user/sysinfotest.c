#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"


void
sinfo(struct sysinfo *info) {
  if (sysinfo(info) < 0) {
    printf("FAIL: sysinfo failed");
    exit(1);
  }
}

//
// use sbrk() to count how many free physical memory pages there are.
//
int
countfree()
{
  uint64 sz0 = (uint64)sbrk(0);
  struct sysinfo info;
  int n = 0;

  while(1){
    if((uint64)sbrk(PGSIZE) == 0xffffffffffffffff){
      break;
    }
    n += PGSIZE;
  }
  sinfo(&info);
  if (info.freemem != 0) {
    printf("FAIL: there is no free mem, but sysinfo.freemem=%ld\n",
      info.freemem);
    exit(1);
  }
  sbrk(-((uint64)sbrk(0) - sz0));
  return n;
}

void
testmem() {
  struct sysinfo info;
  uint64 n = countfree();
  
  sinfo(&info);

  if (info.freemem!= n) {
    printf("FAIL: free mem %ld (bytes) instead of %ld\n", info.freemem, n);
    exit(1);
  }
  
  if((uint64)sbrk(PGSIZE) == 0xffffffffffffffff){
    printf("sbrk failed");
    exit(1);
  }

  sinfo(&info);
    
  if (info.freemem != n-PGSIZE) {
    printf("FAIL: free mem %ld (bytes) instead of %ld\n", n-PGSIZE, info.freemem);
    exit(1);
  }
  
  if((uint64)sbrk(-PGSIZE) == 0xffffffffffffffff){
    printf("sbrk failed");
    exit(1);
  }

  sinfo(&info);
    
  if (info.freemem != n) {
    printf("FAIL: free mem %ld (bytes) instead of %ld\n", n, info.freemem);
    exit(1);
  }
}

void
testcall() {
  struct sysinfo info;
  
  if (sysinfo(&info) < 0) {
    printf("FAIL: sysinfo failed\n");
    exit(1);
  }

  if (sysinfo((struct sysinfo *) 0xeaeb0b5b00002f5e) !=  0xffffffffffffffff) {
    printf("FAIL: sysinfo succeeded with bad argument\n");
    exit(1);
  }
}

void testproc() {
  struct sysinfo info;
  uint64 nproc;
  int status;
  int pid;
  
  sinfo(&info);
  nproc = info.nproc;

  pid = fork();
  if(pid < 0){
    printf("sysinfotest: fork failed\n");
    exit(1);
  }
  if(pid == 0){
    sinfo(&info);
    if(info.nproc != nproc+1) {
      printf("sysinfotest: FAIL nproc is %ld instead of %ld\n", info.nproc, nproc+1);
      exit(1);
    }
    exit(0);
  }
  wait(&status);
  sinfo(&info);
  if(info.nproc != nproc) {
      printf("sysinfotest: FAIL nproc is %ld instead of %ld\n", info.nproc, nproc);
      exit(1);
  }
}

// void testbad() {
//   int pid = fork();
//   int xstatus;
  
//   if(pid < 0){
//     printf("sysinfotest: fork failed\n");
//     exit(1);
//   }
//   if(pid == 0){
//       sinfo(0x0);
//       exit(0);
//   }
//   wait(&xstatus);
//   if(xstatus == -1)  // kernel killed child?
//     exit(0);
//   else {
//     printf("sysinfotest: testbad succeeded %d\n", xstatus);
//     exit(xstatus);
//   }
// }

void
testopenfiles()
{
  struct sysinfo info;
  uint64 base;
  int fd1, fd2, d;

  sinfo(&info);
  base = info.nopenfiles;

  // open two separate file descriptors (each should allocate a file struct)
  fd1 = open("README", 0);
  if(fd1 < 0){
    printf("sysinfotest: open README failed\n");
    exit(1);
  }
  sinfo(&info);
  if(info.nopenfiles != base+1){
    printf("sysinfotest: FAIL nopenfiles %ld instead of %ld after first open\n", info.nopenfiles, base+1);
    exit(1);
  }

  fd2 = open("README", 0);
  if(fd2 < 0){
    printf("sysinfotest: open README second failed\n");
    exit(1);
  }
  sinfo(&info);
  if(info.nopenfiles != base+2){
    printf("sysinfotest: FAIL nopenfiles %ld instead of %ld after second open\n", info.nopenfiles, base+2);
    exit(1);
  }

  // dup should not create a new file struct (only increment ref)
  d = dup(fd1);
  if(d < 0){
    printf("sysinfotest: dup failed\n");
    exit(1);
  }
  sinfo(&info);
  if(info.nopenfiles != base+2){
    printf("sysinfotest: FAIL nopenfiles %ld instead of %ld after dup\n", info.nopenfiles, base+2);
    exit(1);
  }

  // close fd2 (its file struct ref should go to 0)
  if(close(fd2) < 0){
    printf("sysinfotest: close failed\n");
    exit(1);
  }
  sinfo(&info);
  if(info.nopenfiles != base+1){
    printf("sysinfotest: FAIL nopenfiles %ld instead of %ld after close fd2\n", info.nopenfiles, base+1);
    exit(1);
  }

  // close fd1 (still referenced by dup), should not free file struct yet
  if(close(fd1) < 0){
    printf("sysinfotest: close fd1 failed\n");
    exit(1);
  }
  sinfo(&info);
  if(info.nopenfiles != base+1){
    printf("sysinfotest: FAIL nopenfiles %ld instead of %ld after close fd1\n", info.nopenfiles, base+1);
    exit(1);
  }

  // close dup; now the original file struct should be freed
  if(close(d) < 0){
    printf("sysinfotest: close dup failed\n");
    exit(1);
  }
  sinfo(&info);
  if(info.nopenfiles != base){
    printf("sysinfotest: FAIL nopenfiles %ld instead of %ld after close dup\n", info.nopenfiles, base);
    exit(1);
  }
}

int
main(int argc, char *argv[])
{
  printf("sysinfotest: start\n");
  testcall();
  testmem();
  testproc();
  testopenfiles();
  printf("sysinfotest: OK\n");
  exit(0);
}
