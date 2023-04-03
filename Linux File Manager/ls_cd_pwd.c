//Level 1 - Cougar Fischer
// ls_cd_pwd.c
//Resource: Jovan Araiza, Professor Wang, Yu Nong, Jin Park, Christopher Roberts, Zhila Esfahani, Jimmy Zheng, Nikita Fischer

#include "util_cougar.c"

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

int ls_file(MINODE *mip, char *myname)
{
	int i;
	u16 mode, mask;
	char mydate[32], *s, *cp, ss[32];
	
	mode = mip->INODE.i_mode;
	if(S_ISDIR(mode))
	{
		putchar('d');
	}
	else if(S_ISLNK(mode))
	{
		putchar('l');
	}
	else
	{
		putchar('-');
	}

	mask = 000400;
	for(i=0; i<1; i++)
	{
		if(mode & mask)
		{
			putchar('r');
		}
		else
		{
			putchar('-');
		}
		mask = mask >> 1;

		if(mode & mask)
		{
			putchar('w');
		}
		else
		{
			putchar('-');
			mask = mask >> 1;
		}
	
		if(mode & mask)
		{
			putchar('x');
		}
		else
		{
			putchar('-');
			mask = mask >> 1;
		}

		printf("%4d", mip->INODE.i_links_count);
		printf("%4d", mip->INODE.i_uid);
		printf("%4d", mip->INODE.i_gid);
		printf(" ");

		s = mydate;
		s = (char *)ctime(&mip->INODE.i_ctime);
		s = s + 4;
		strncpy(ss, s, 12);
		ss[12] = 0;

		printf("%s", ss);
		printf("%8ld", mip->INODE.i_size);

		printf("   %s", myname);

		if(S_ISLNK(mode))
		{
			printf(" ->%s", (char *)mip->INODE.i_block);
		}
		printf("\n");
	}
}
/*
int ls_file(MINODE *mip, char *myname)
{
	int i;
	u16 mode, mask;
	char mydate[32], *s, *cp, ss[32];
	time_t mytime = mip->INODE.i_ctime;
	char *s = ctime(&mytime);

	mask = 000400;
	for(i=0; i<1; i++)
	{
		if(mode & mask)
		{
			putchar('r');
		}
		else
		{
			putchar('-');
		}
		mask = mask >> 1;

		if(mode & mask)
		{
			putchar('w');
		}
		else
		{
			putchar('-');
			mask = mask >> 1;
		}
	
		if(mode & mask)
		{
			putchar('x');
		}
		else
		{
			putchar('-');
			mask = mask >> 1;
		}

		printf("%4d", mip->INODE.i_links_count);
		printf("%4d", mip->INODE.i_uid);
		printf("%4d", mip->INODE.i_gid);
		printf(" ");

		s = mydate;
		s = (char *)ctime(&mip->INODE.i_ctime);
		s = s + 4;
		strncpy(ss, s, 12);
		ss[12] = 0;

		printf("%s", ss);
		printf("%8ld", mip->INODE.i_size);

		printf("   %s", myname);

		if(S_ISLNK(mode))
		{
			printf(" ->%s", (char *)mip->INODE.i_block);
		}
		printf("\n");
	}
}*/
/*
int ls_dir(MINODE *mip)
{
	int i;
	char sbuf[BLKSIZE];
	char name[256];
	DIR *dp;
	char *cp;
	MINODE *dirip;

	for(i=0; i<12; i++)
	{
		if(mip->INODE.i_block[i] == 0)
		{
			return 0;
		}
		
		get_block(mip->dev, mip->INODE.i_block[i], sbuf);
		dp = (DIR *)sbuf;
		cp = sbuf;
	
		while(cp < sbuf + BLKSIZE)
		{
			strcpy(name, dp->name);
			name[dp->name_len] = 0;
			dirip = iget(dev, dp->inode);
			ls_file(dirip, name);
			iput(dirip);
			cp+=dp->rec_len;
			dp = (DIR *)cp;
		}
	}
}*/
int ls_dir(MINODE *mip)
{
	int i;
	char sbuf[BLKSIZE];
	char name[256];
	DIR *dp;
	char *cp;
	MINODE *dirip;

	for(i=0; i<12; i++)
	{
		if(mip->INODE.i_block[i] == 0)
		{
			return 0;
		}
		
		get_block(mip->dev, mip->INODE.i_block[i], sbuf);
		dp = (DIR *)sbuf;
		cp = sbuf;
	
		while(cp < sbuf + BLKSIZE)
		{
			strcpy(name, dp->name);
			name[dp->name_len] = 0;
			dirip = iget(dev, dp->inode);
			ls_file(dirip, name);
			iput(dirip);
			cp+=dp->rec_len;
			dp = (DIR *)cp;
		}
	}
}


int ls(char *pathname)
{
	MINODE *mip;
	u16 mode;
	int dev, ino;

	if(pathname[0] == 0)
	{
		ls_dir(running->cwd);
	}
	else
	{
		dev = root->dev;
		ino = getino(pathname);
		if(ino == 0)
		{
			printf("no such file %s\n", pathname);
			return -1;
		}
		mip = iget(dev, ino);
		mode = mip->INODE.i_mode;
		if(!S_ISDIR(mode))
		{
			ls_file(mip, (char *)basename(pathname));
		}
		else
		{
			ls_dir(mip);
		}
		iput(mip);
	}
}

int myCd(char *pathname)
{
	MINODE *mip;
	int ino, dev;
	//char dummy;
	
	if(pathname[0] == 0)
	{
		iput(running->cwd);
		running->cwd = root;
		//iget(root->dev, 2);

		return;
	}
	if(pathname[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		printf("dev %d ", dev);
		dev = running->cwd->dev;
		printf("    A dev %d\n", dev);
	}
	
	printf("before getino\n");
	ino = getino(pathname);
	printf("after getino\n");
	//getc(&dummy);

	if(!ino)
	{
		printf("The directory doesn't exist\n");
		return -1;
	}

	mip = iget(dev, ino);

	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("There is no such directory\n");
		return -1;
	}
	iput(running->cwd);
	running->cwd = mip;
}

void rpwd(MINODE *wd)
{
	char buf[BLKSIZE], myname[256], *cp;
	MINODE *parent, *ip;
	u32 myino, parentino;
	DIR *dp;

	printf("-------------------------rpwd-------------------------\n");
	if(wd == root)
	{
		return;
	}
	
	parentino = findino(wd, &myino);
	parent = iget(dev, parentino);

	findmyname(parent, myino, myname);
	//recursively call rpwd()
	rpwd(parent);

	iput(parent);
	printf("/%s", myname);
	
	return 1;

}

char *pwd(MINODE *wd)
{
	//int pip;
	//int parent_ino;
	printf("---------------------------PWD--------------------------\n");

	printf(" %d ", root->INODE.i_block[0]);
	printf(" %d same? \n", wd->INODE.i_block[0]);

	printf("wd->dev=%d  root->dev=%d  wd->ino=%d  root->ino=%d\n", wd->dev, root->dev, wd->ino, root->ino);

	/*if(wd->dev==root->dev && wd->ino==root->ino)
	{
		printf("\n", wd->dev, wd->ino);
		printf("-----------------------------------------------------\n");
	}
	else
	{
		rpwd(wd);
	}*/

	if(wd == root)
	{
		printf("/\n");
		return;
	}
	else
	{
		rpwd(wd);
		printf("\n");
	}
}
