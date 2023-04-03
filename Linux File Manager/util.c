/***********************************************************************************
	Level 1 - Cougar Fischer
	util.c
	Resources: Jin Park, Jimmy Zheng, Professor Wang, Yu Nong
*******************************************************************************/

//util.c file contains functions: getino(pathname); iget(dev, ino); iput(mip);
//Their usage has the following pattern:

//int ino     = getino(char *pathname);  
//MINODE *mip = iget(dev, ino);

// USE the INODE in minode

//iput(mip)

#ifndef UTIL
#define UTIL

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char   gpath[128];
extern char   *name[64];
extern int    n;

extern int    fd, dev;
extern int    ninodes, nblocks;
extern int    bmap, imap, iblock;
extern char   line[128], cmd[32], pathname[256];

int get_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	write(fd, buf, BLKSIZE);
}

MINODE *iget(int dev, int ino)
{
	MINODE *mip;
	char buf[BLKSIZE];
	int blk, disp;
	INODE *ip;
	int i;

	for(i=0; i<NMINODE;i++)
	{
		mip = &minode[i];
		if(mip->dev == dev && mip->ino == ino)
		{
			mip->refCount++;
			printf("Found dev: %d ino: %d at minode[%d]\n", dev, ino, i);
			return mip;
		}
	}

	for(i=0; i<NMINODE;i++)
	{
		mip = &minode[i];
		if(mip->refCount == 0)
		{
			//printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
			mip->refCount = 1;
			mip->dev = dev;
			mip->ino = ino;
			// get INODE of ino to buf
			//blk = (ino-1) / 8 + iblk;
			blk = (ino-1) / 8 + iblock;
			disp = (ino-1) % 8;

			get_block(dev, blk, buf);
			ip = (INODE *)buf + disp;
			// copy INODE to mp->INODE
			mip->INODE = *ip;

			printf("Load dev: %d ino: %d into minode[%d]\n", dev, ino, i);
			return mip;
		}
	}

	printf("No more free minodes\n");
	return 0;
}

void iput(MINODE *mip)
{
	int ino = 0;
	int blk, disp;
	char buf[BLKSIZE];
	INODE *ip;

	if(mip == 0)
	{
		return;
	}
	
	mip->refCount--;
	if(mip->refCount > 0)
	{
		return;
	}
	if(!mip->dirty)
	{
		return;
	}
	/* write back */
 	printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

	blk = ((mip->ino-1) / 8) + iblock;
	disp = (mip->ino-1) % 8;

	/* first get the block containing this inode */
	get_block(mip->dev, blk, buf);

	ip = (INODE *)buf + disp;
	*ip = mip->INODE;
	put_block(mip->dev, blk, buf);
}
/*The getino function first uses the tokenize() function to break up 
pathname into component strings.*/
void tokenize(char *pathname)
{
	strcpy(gpath, pathname);
	char *s;
	n = 0;
	s = strtok(pathname, "/"); //do the strtok here
	while(s)
	{
		name[n] = s;
		s = strtok(0, "/");
		n++;
	}
	name[n] = NULL;

}
/*The getino() function implements the file system tree traversal algorithm. It
returns the INODE number (ino) of a specified pathname.*/
int getino(char *pathname)
{
	MINODE *mip;
	INODE *ip;
	char buf[BLKSIZE];
	int ino;
	int i;

	if(strcmp(pathname, "/") == 0)
	{
		return 2;
	}

	if(pathname[0] == '/')
	{
		mip = iget(dev, 2);
	}
	else
	{
		printf("Cwd inode number: %d\n", running->cwd->ino);
		mip = iget(running->cwd->dev, running->cwd->ino);
	}
	strcpy(buf, pathname);
	tokenize(buf);

	for (i=0; i < n; i++){
             ino = search(mip, name[i]);
             //iput(mip);
             if (ino==0){
		iput(mip);
                printf("can't find %s\n", name[i]); 
                return 0;
             }
             mip = iget(dev, ino);   // ip -> new INODE
         }
	iput(mip); //without this, the refCount is off
	return ino;	
}

int search(MINODE *mip, char *name)
{
        char sbuf[BLKSIZE], temp[256];
	printf("Enter search name is %s!\n", name);
        DIR *dp;
        char *cp;
        int i;
	int dummy = 0;
	INODE *ip = &(mip->INODE);
 
	for(i=0; i < 12; i++)
	{
		if(ip->i_block[i] == 0)
		{
			break;
		}
		printf("i_block[i] number: %d\n", ip->i_block[i]);
		get_block(dev, ip->i_block[i], sbuf);

		dp = (DIR *)sbuf;
		cp = sbuf;
		dummy = 0;
		printf("Enter while loop\n");

		while(cp < sbuf + BLKSIZE)
		{
			dummy++;
			printf("=============================================\n");
			printf("	iteration %d	\n", dummy);
			printf("=============================================\n");
			printf("dp->name = %s\n", dp->name);
			printf("dp->name_len = %d\n", dp->name_len);
			//gets(dummy);
			strncpy(temp, dp->name, dp->name_len);
			printf("after strcpy\n");
			temp[dp->name_len] = 0;
			printf("temp = %s!\n", temp);
			if(strcmp(name, temp) == 0)
			{
				printf("Found the file named %s with ino %d\n", name, dp->inode);
				return dp->inode;
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;
			getchar();
		}
	}

	return 0;

        /*for (i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
            if (ip->i_block[i] == 0)
               break;
            // YOU SHOULD print i_block[i] number here
	    printf("i_block[i] number: %d\n", ip->i_block[i]);
            get_block(dev, sbuf, ip->i_block[i]);

            dp = (DIR *)sbuf;
            cp = sbuf;
 
            while(cp < sbuf + BLKSIZE){
               	strncpy(temp, dp->name, dp->name_len);
               	temp[dp->name_len] = 0;
		//printf("hello\n");
		printf("temp = %s  ", temp);
	    	if(strcmp(name, temp) == 0)
	    	{
               		printf("Found the file named %s with ino %d\n", name, dp->inode);
	       		return dp->inode;

	    	}
	    	cp += dp->rec_len;
            	dp = (DIR *)cp;
           }
        }*/
	//return 0;
}

int findmyname(MINODE *parent, int myino, char *myname)
{
	char *cp, c, sbuf[BLKSIZE];
	int i;
	DIR *dp;
	INODE *ip;

	ip=&(parent->INODE);

	for(i=0; i<12; i++)
	{
		if(ip->i_block[i] == 0)
		{
			return 0;
		}
		get_block(dev, ip->i_block[i], sbuf);
		dp = (DIR *)sbuf;
		cp = sbuf;
			
			while(cp < sbuf + BLKSIZE)
			{
				c = dp->name[dp->name_len];
				dp->name[dp->name_len]==0;
				if(myino == dp->inode)
				{
					strcpy(myname, dp->name);
					return -1;
				}
				dp->name[dp->name_len] = c;
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
	}
	return 0;

	/*char buf[BLKSIZE];
	int i;

	for(i=0; i<12; i++)
	{
		if(parent->INODE.i_block[i] != 0)
		{
			get_block(parent->dev, parent->INODE.i_block[i], buf);
			DIR *dir = (DIR *)&buf;
			int pos = 0;
			
			while(pos < BLKSIZE)
			{
				char dirname[dir->name_len + 1];
				strncpy(dirname, dir->name, dir->name_len);
				dirname[dir->name_len] = '\0';
				if(dir->inode == myino)
				{
					strcpy(myname, dirname);
					myname[strlen(myname)-1] = 0;
					return 0;
				}
				char *loc = (char *)dir;
				loc += dir->rec_len;
				pos += dir->rec_len;
				dir = (DIR *)loc;
			}
		}
	}
	return -1;*/
}

int findino(MINODE *mip, u32 *myino)
{
	//fill myino with ino of .
	//return ino of ..
	int i;
	char *cp, c, sbuf[BLKSIZE];
	DIR *dp;
	INODE *ip;

	ip = &(mip->INODE);
	*myino = mip->ino;

	for(i=0; i<12; i++)
	{
		if(ip->i_block[i] == 0)
		{
			return 0;
		}
		get_block(dev, ip->i_block[i], sbuf);
		dp = (DIR *)sbuf;
		cp = sbuf;
			
			while(cp < sbuf + BLKSIZE)
			{
				c = dp->name[dp->name_len];
				dp->name[dp->name_len]==0;
				if(!strcmp(dp->name, ".."))
				{
					return dp->inode;
				}
				dp->name[dp->name_len] = c;
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
	}
	return 0;

	//return ino of ..

	/*//fill myino with ino of .
	myino = search(mip, ".");
	return search(mip, "..");
	//return ino of ..*/
}

int decFreeBlocks(int dev)
{
	char buf[BLKSIZE];
	
	//dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, 2, buf);
}

#endif
