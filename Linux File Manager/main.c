/**********************************************************************************
	Level 1 - Cougar Fischer
	File: main.c
	Resources: Jovan Araiza, Professor Wang, Yu Nong, Jin Park, Christopher Roberts, Zhila Esfahani, Jimmy Zheng, Nikita Fischer
**********************************************************************************/


//--------------------Level 1--------------------
#include "util_cougar.c"
#include "mkdir.c"
#include "ls_cd_pwd.c"
#include "rmdir.c"
#include "link_unlink_symlink.c"
#include "stat.c"
#include "chmod.c"
//--------------------Level 2--------------------
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write_cp.c"

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

#define BLKSIZE 1024
GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

/********** globals *************/
MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

int fd, dev;
int ninodes, nblocks;    // ninodes, nblocks numbers from SUPER
int bmap, imap, iblock;  // BMAP, IMAP, inodes start block numbers

char gpath[128];         // token strings
int n;                   // number of token strings
char *name[64];          // pointers to token strings

char line[128], cmd[32], pathname[256], pathname2[256], pathname3[256];



/*int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}*/

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}


int decFreeInodes(int dev)
{
  char buf[BLKSIZE];
  // dec free inodes count by 1 in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int ialloc(int dev)  // allocate an inode number
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, imap, buf);
       decFreeInodes(dev);
       return i+1;
    }
  }
  return 0;
}

//int balloc(dev) function, which returns a FREE disk block number
int balloc(int dev)
{
	int i;
	char buf[BLKSIZE];

	get_block(dev, bmap, buf);

	for(i=0; i < nblocks; i++)
	{
		if(tst_bit(buf, i)==0)
		{
			set_bit(buf, i);
			decFreeBlocks(dev);
			
			put_block(dev, bmap, buf);

			return i+1;
		}
	}
	printf("balloc(): no more free blocks available\n");
	return 0;
}
  //initalize function to initalize the ino,refcount,pid,uid,cwd,status, mounted and the mptr.  
int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
}

int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "diskimage";

int main(int argc, char *argv[ ])
{
  int i, ino;
  char buf[BLKSIZE];

  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDWR);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;

  // read SUPER block to verify it's an EXT2 FS
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;
  // verfiy it's an EXT2 FS
	   
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  printf("ninodes = %d nblocks = %d\n", ninodes, nblocks);

  // read Group Descriptor 0 to get bmap, imap and iblock numbers
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblock = gp->bg_inode_table;
  printf("bmp=%d imap=%d iblock = %d\n", bmap, imap, iblock);

/*----------------------------------------------------------
3. DO init(); mount_root(); as in LAB #6
-----------------------------------------------------------*/
  init();  
  mount_root();

  printf("root refCount = %d\n", root->refCount);
  
  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  
  printf("root refCount = %d\n", root->refCount);

  //printf("hit a key to continue : "); getchar();
  while(1){
    printf("Level-1 input command : [mkdir|creat|ls|cd|pwd|rmdir|link|unlink|symlink|chmod|stat|quit]\n");
    printf("Level-2 input command : [open|close|lseek|pfd|read|cat|write|cp|mv]\n");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    printf("root refCount = %d\n", root->refCount);

    if (line[0]==0)
      continue;

    pathname[0] = 0;
    pathname2[0] = 0;
    //pathname3[0] = 0;
    cmd[0] = 0;
    
    sscanf(line, "%s %s %[^\n]", cmd, pathname, pathname2);
    printf("cmd=%s pathname=%s pathname2 = %s\n", cmd, pathname, pathname2);

    if (strcmp(cmd, "mkdir")==0)
	make_dir(pathname);
    if (strcmp(cmd, "creat")==0)
	creat_file(pathname);
    if (strcmp(cmd, "ls")==0)
       ls(pathname);
    if (strcmp(cmd, "cd")==0)
       myCd(pathname);
    if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    if (strcmp(cmd, "rmdir")==0)
	rmDir();
    if (strcmp(cmd, "link")==0)
	myLink(pathname, pathname2);
    if (strcmp(cmd, "unlink")==0)
	myUnlink(pathname);
    if (strcmp(cmd, "symlink")==0)
	mySymlink(pathname, pathname2);
    if (strcmp(cmd, "chmod")==0)
	myChmod(pathname, pathname2);
    if (strcmp(cmd, "stat")==0)
	myStat(pathname);
    if (strcmp(cmd, "open")==0)
	open_file(pathname, pathname2);
    if (strcmp(cmd, "close")==0)
	close_file(pathname);
    if(strcmp(cmd, "lseek")==0)
	mylseek(pathname, pathname2);
    if (strcmp(cmd, "pfd")==0)
	pfd();
    if (strcmp(cmd, "read")==0)
	read_file(pathname, pathname2);
    if (strcmp(cmd, "cat")==0)
	cat(pathname);
    if (strcmp(cmd, "write") == 0)
    {
	int fd, nbytes;
	sscanf(pathname, "%d", &fd);
	//sscanf(pathname2, "%d", &nbytes);
	printf("About to write...\n");
	write_file(fd, pathname2);
    }
    if(strcmp(cmd, "cp") == 0)
	mycp(pathname, pathname2);
    if(strcmp(cmd, "mv")==0)
	mymv(pathname, pathname2);
    if (strcmp(cmd, "quit")==0)
       quit();
  }

  return 0;
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}

/*                   HOW TO mkdir

Assume: command line = "mkdir pathname" 
Extract cmd, pathname from line and save them as globals.

int make_dir()
{
   MINODE *start;		     
1. pahtname = "/a/b/c" start = root;         dev = root->dev;
            =  "a/b/c" start = running->cwd; dev = running->cwd->dev;

2. Let  
     parent = dirname(pathname);   parent= "/a/b" OR "a/b"
     child  = basename(pathname);  child = "c"

   WARNING: strtok(), dirname(), basename() destroy pathname

3. Get the In_MEMORY minode of parent:

         pino  = getino(parent);
         pip   = iget(dev, pino); 

   Verify : (1). parent INODE is a DIR (HOW?)   AND
            (2). child does NOT exists in the parent directory (HOW?);
               
4. call mymkdir(pip, child);

5. inc parent inodes's link count by 1; 
   touch its atime and mark it DIRTY

6. iput(pip);
     
} 
*/

/*int mymkdir(MINODE *pip, char *name)
{
   MINODE *mip;

1. pip points at the parent minode[] of "/a/b", name is a string "c") 

2. allocate an inode and a disk block for the new directory;
        ino = ialloc(dev);    
        bno = balloc(dev);
   DO NOT WORK IN THE DARK: PRINT OUT THESE NUMBERS!!!

3. mip = iget(dev, ino);  load the inode into a minode[] (in order to
   wirte contents to the INODE in memory.

4. Write contents to mip->INODE to make it as a DIR INODE.

5. iput(mip); which should write the new INODE out to disk.

  // C CODE of (3), (4) and (5):
  **********************************************************************
  mip = iget(dev,ino);
  INODE *ip = &mip->INODE;
  Use ip-> to acess the INODE fields:

  i_mode = 0x41ED;		// OR 040755: DIR type and permissions
  i_uid  = running->uid;	// Owner uid 
  i_gid  = running->gid;	// Group Id
  i_size = BLKSIZE;		// Size in bytes 
  i_links_count = 2;	        // Links count=2 because of . and ..
  i_atime = i_ctime = i_mtime = time(0L);  // set to current time
  i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
  i_block[0] = bno;             // new DIR has one data block   
  i_block[1] to i_block[14] = 0;
 
  mip->dirty = 1;               // mark minode dirty
  iput(mip);                    // write INODE to disk


***** create data block for new DIR containing . and .. entries ******
6. Write . and .. entries into a buf[ ] of BLKSIZE

   | entry .     | entry ..                                             |
   ----------------------------------------------------------------------
   |ino|12|1|.   |pino|1012|2|..                                        |
   ----------------------------------------------------------------------

   Then, write buf[ ] to the disk block bno;

7. Finally, enter name ENTRY into parent's directory by 
            enter_name(pip, ino, name);


8. int enter_name(MINODE *pip, int myino, char *myname)
{
 For each data block of parent DIR do { // assume: only 12 direct blocks

     if (i_block[i]==0) BREAK;

(1). get parent's data block into a buf[];
   
(2). EXT2 DIR entries: Each DIR entry has rec_len and name_len. Each entry's
     ideal length is   

        IDEAL_LEN = 4*[ (8 + name_len + 3)/4 ]
 
     All DIR entries in a data block have rec_len = IDEAL_LEN, except the last
     entry. The rec_len of the LAST entry is to the end of the block, which may
     be larger than its IDEAL_LEN.

  --|-4---2----2--|----|---------|--------- rlen ->------------------------|
    |ino rlen nlen NAME|.........|ino rlen nlen|NAME                       |
  --------------------------------------------------------------------------

(3). To enter a new entry of name with n_len, the needed length is

        need_length = 4*[ (8 + n_len + 3)/4 ]  // a multiple of 4

(4). Step to the last entry in a data block (HOW?).
 
    // get parent's ith data block into a buf[ ] 

       get_block(parent->dev, parent->INODE.i_block[i], buf);
  
       dp = (DIR *)buf;
       cp = buf;

       // step to LAST entry in block: int blk = parent->INODE.i_block[i];
       
       printf("step to LAST entry in data block %d\n", blk);
       while (cp + dp->rec_len < buf + BLKSIZE){

          *************************************************
             print DIR record names while stepping through
          **************************************************

          cp += dp->rec_len;
          dp = (DIR *)cp;
       } 
       // dp NOW points at last entry in block
  
     Let remain = LAST entry's rec_len - its IDEAL_LENGTH;

     if (remain >= need_length){
        enter the new entry as the LAST entry and trim the previous entry
        to its IDEAL_LENGTH; 
        goto (6) below.
     } 

                             EXAMPLE:

                                 |LAST entry 
  --|-4---2----2--|----|---------|--------- rlen ->------------------------|
    |ino rlen nlen NAME|.........|ino rlen nlen|NAME                       |
  --------------------------------------------------------------------------
                                                    |     NEW entry
  --|-4---2----2--|----|---------|----ideal_len-----|--- rlen=remain ------|
    |ino rlen nlen NAME|.........|ino rlen nlen|NAME|myino rlen nlen myname|
  --------------------------------------------------------------------------

}

(5).// Reach here means: NO space in existing data block(s)

  Allocate a new data block; INC parent's isze by BLKSIZE;
  Enter new entry as the first entry in the new data block with rec_len=BLKSIZE.

  |-------------------- rlen = BLKSIZE -------------------------------------
  |myino rlen nlen myname                                                  |
  --------------------------------------------------------------------------

(6).Write data block to disk;
}        


creat_file()
{
  This is ALMOST THE SAME AS mkdir() except : 
   (1). its inode's mode field is set to a REGULAR file, 
        permission bits to (default) rw-r--r--, 
   (2). No data block, so size = 0
   (3). links_count = 1;
   (4). Do NOT increment parent's links_count
} 


int my_creat(MINODE *pip; char *name)
{
  Same as mymkdir() except 
    INODE's file type = 0x81A4 OR 0100644
    links_count = 1
    NO data block, so size = 0
    do NOT inc parent's link count.
}  

====================================================================

================ development HEPS =============================

1. Your mkdir/creat may trash the disk iamge (by writing to wrong inode or data
   blocks), which will cause problems later even if your program is correct. So,
   it's better to use a FRESH disk image each time during development.

   Write a sh script "run" or "doit" containing: 

         mkfs disk 1440  # renew the disk image file
         a.out

   Enter run or doit to test YOUR new a.out, so that you use a NEW disk image 
   file each time until YOUR a.out no longer traches the disk image.

2. After running YOUR mkdir/creat commands, it's better to check the results 
   under LINUX. Write a sh script "s" containing
       
         mount -o loop disk /mnt
         ls -l /mnt
         umount /mnt

   so that s will show the disk contents under LINUX.
==============================================================
*/