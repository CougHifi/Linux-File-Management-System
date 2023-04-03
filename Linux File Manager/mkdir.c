/**********************************************************************************
	Level 1 - Cougar Fischer
	mkdir.c
	Resources: Jin Park, Ryan Neisess, Christopher Roberts, Professor Wang, Nikita Fischer
**********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util_cougar.c"


//It enters a [ino, name] as a new dir_entry into a parent directory. 
int enter_name(MINODE *pip, int myino, char *myname)
{
	char buf[BLKSIZE], newBuf[BLKSIZE], *cp;
	int bno;
	int need_length, remain, old_last_entry_ideal_length;
	need_length = 4*((8+strlen(myname)+3)/4);	//rec length of myname(child)
	int i, j;
	
	for(j=0; j<12; j++)
	{
		//get latest block number
		if(pip->INODE.i_block[j]==0)
		{
			i=j-1;
			break;
		}
	}
	get_block(running->cwd->dev, pip->INODE.i_block[i], buf);	//get the latest block into a buf
	dp = (DIR *)buf;
	cp = buf;

	while(cp+dp->rec_len < buf + BLKSIZE)	//make dp point to last entry
	{
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}	//now dp points at last entry in block
	old_last_entry_ideal_length = 4*((8+dp->name_len+3)/4);	//using a variable to store ideal length of last entry
	remain = dp->rec_len - old_last_entry_ideal_length;	//calculate how many rec_len can be used after the last entry

	if(remain >= need_length)	//if remain rec_len is greater than or equal to need_length which is the rec_len needed for child
	{
		dp->rec_len=old_last_entry_ideal_length;	//change old last entry's rec_len
		
		cp+=dp->rec_len;
		dp=(DIR *)cp;	//step forward
		dp->rec_len=remain;	//put all info (rec_len=remain, inode = myino(allocated), name)
		dp->inode = myino;
		dp->file_type=2;
		dp->name_len=strlen(myname);
		strncpy(dp->name, myname, strlen(myname));

		put_block(running->cwd->dev, pip->INODE.i_block[i], buf);	//write back and return
		return 0;
	}
	else	//if the block is full, we need a new block
	{
		bno = balloc(running->cwd->dev);	//allocate new block number
		pip->INODE.i_block[i+1]=bno;	//assign bno to the next block

		pip->INODE.i_size += BLKSIZE;	//set info
		get_block(running->cwd->dev, pip->INODE.i_block[i+1], newBuf);	//should I use new buf?
		dp = (DIR *)newBuf;
		cp = newBuf;
		dp->rec_len = BLKSIZE; //rec_len will be 1024
		dp->inode = myino;
		dp->name_len = strlen(myname);
		dp->file_type=2;
		strncpy(dp->name, myname, strlen(myname));	//child will be only first entry
	
		put_block(running->cwd->dev, pip->INODE.i_block[i+1], newBuf);	//write back
		return 0;
	}
}

void mymkdir(MINODE *pip, char *child) //(1) from mymkdir instructions
{
	MINODE *mip;
	int ino, bno;
	char buf[BLKSIZE], *cp;
	int i;

	ino = ialloc(running->cwd->dev);	//(2) allocate an inode & a disk block for the new directory
	bno = balloc(running->cwd->dev);

	mip = iget(running->cwd->dev, ino);	//load the inode into a minode[] (in order to write contents to the INODE in memory)
	
	INODE *ip=&mip->INODE;	//assign all info
	ip->i_mode = 0x41ED;	// OR 040755: DIR type and permissionn
	ip->i_uid = running->uid;// Owner uid
	ip->i_gid = running->gid;// Group Id
	ip->i_size = BLKSIZE;// Size in bytes
	ip->i_links_count = 2;// Links count=2 because of . and .
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);// set to current time
	ip->i_blocks = 2;// LINUX: Blocks count in 512-byte chunk
	ip->i_block[0] = bno;	// new DIR has one data block
	
	for(i=1; i < 15; i++)	//set rest = 0
	{
		ip->i_block[i]=0;
	}
	mip->dirty=1;
	iput(mip);	//3, 4, 5 from mymkdir instructions

	//write . and .. entries into a buf[] of BLKSIZE
	//.
	get_block(running->cwd->dev, bno, buf);	//get_block[0] into buf
	cp = buf;	//and make . and ..
	dp = (DIR *)cp;
	dp->inode = ino;
	dp->name_len = 1;
	dp->file_type = 2;
	strncpy(dp->name, ".", 1);
	dp->rec_len = 12;

	//..
	cp = cp + dp->rec_len;
	dp = (DIR *)cp;
	dp->inode = pip->ino;
	dp->name_len = 2;
	dp->file_type = 2;
	strncpy(dp->name, "..", 2);
	dp->rec_len=1012;

	put_block(running->cwd->dev, bno, buf);	//and write back to dist using put_block

	enter_name(pip, ino, child); //and call enter_name with parent dir minode pointer, new inode and child name
}

void make_dir(char *pathname)
{
	if(strlen(pathname) == 0)	//check if pathname is entered
	{
		printf("mkdir requires a pathname\n");
		return;
	}

	MINODE *mip, *pip;

	char parent_pathname[BLKSIZE], child_name[BLKSIZE];
	char *pname, *cname;
	int dev, pino;

	strcpy(parent_pathname, pathname);
	strcpy(child_name, pathname);

	pname = dirname(parent_pathname);	//store parent dirname & child name of pathname
	cname = basename(child_name);

	if(pathname[0] == '/')	//if pathname begins with / then use minode pointer to point to root
	{
		dev=root->dev;
		mip=iget(running->cwd->dev, 2);
	}
	else	//else, minode points to the current working directory
	{
		dev = running->cwd->dev;
		mip = iget(running->cwd->dev, running->cwd->ino);
	}

	pino = getino(pname);
	pip = iget(dev, pino);

	if(pip->INODE.i_mode != 0x41ED)	//if parent dir is not a directory, print error & return
	{
		printf("Parent node is not a DIR\n");
		iput(mip);
		iput(pip);
		return;
	}
	if(getino(pathname)!=0)	//if child is existing, then print error & return
	{
		printf("Child exists\n");
		iput(mip);
		iput(pip);
		return;
	}
	mymkdir(pip, cname);	//else, call mymkdir function with minode pointer to parent dir with child name

	pip->INODE.i_links_count = pip->INODE.i_links_count+1; //after call mymkdir, increment parent inode's link count by 1

	pip->INODE.i_atime=time(0l);	//touch it's atime
	pip->dirty=1;	//and mark it DIRTY

	iput(pip); //(6) from instructions
	iput(mip);
}

/*creat_file()
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
   under LINUX. Write a sh script "s" containing*/

void my_creat(MINODE *pip, char *name)
{
	MINODE *mip;
	int ino, bno;
	char buf[BLKSIZE], *cp;
	DIR *dp;
	int i;

	ino = ialloc(dev);
	bno = balloc(dev);

	mip = iget(dev, ino);
	INODE *ip = &mip->INODE;

	ip->i_mode = 0x81A4;
	ip->i_uid = running->uid;
	ip->i_gid = running->gid;
	ip->i_size = 0;
	ip->i_links_count = 1;
	ip->i_atime = time(0L);
	ip->i_ctime = time(0L);
	ip->i_mtime = time(0L);	//set to current time
	ip->i_blocks = 0;	//LINUX: Blocks count in 512 byte chunks

	ip->i_block[0] = bno;	//new DIR has one data block

	for(i=1; i<15; i++)
	{
		ip->i_block[i] = 0;
	}
	mip->dirty = 1;	//mark minode dirty
	iput(mip);	//write INODE to disk

	//enter name into parent dir
	enter_name(pip, ino, name);

	return;
}

void creat_file(char* name)
{
	char *parent, *child;
	char pname[100];
	char cname[100];

	strcpy(pname, name);
	strcpy(cname, name);

	MINODE *pip;
	int pino;

	if(strlen(name)==0)
	{
		printf("path is required\n");
		return;
	}

	parent = dirname(pname);	//get parent name and child name
	child = basename(cname);

	pino = getino(parent);	//use minode pointer to point parent dirname
	pip = iget(dev, pino);

	if(pip->INODE.i_mode != 0x41ED)	//if parent is not dir, return
	{
		printf(" parent inode is not a dir \n");
		iput(pip);
		return;
	}
	
	if(getino(name)>0)	//check if child exists. If so, return
	{
		printf("getino(%s) = %d\n", name, getino(name));
		printf(" child already exists \n");
		iput(pip);
		return;
	}

	my_creat(pip, child);	//call my_creat function with parent dir mino pointer with child name
	pip->INODE.i_atime = time(0l);	//update parent dir's access time
	pip->dirty = 1;

	iput(pip);
}
       
