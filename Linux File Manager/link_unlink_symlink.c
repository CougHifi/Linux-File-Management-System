/***********************************************************************
	Level 1 - Cougar Fischer
	link_unlink_symlink.c
	Resources: Jovan Araiza, Professor Wang, Yu Nong, Jin Park, Christopher Roberts, Zhila Esfahani, Jimmy Zheng,
	 Ryan Neisess,Nikita Fischer
***********************************************************************/

/********************************************
Issues: (1)In you main():
pathname2 is NULL ===> link old new
                                              but new = NULL string
(2) I link file 1 to again to abc and when I unlink file1 and again, it works. But when I try to unlink the last one, abc, it runs into an infinite loop
	->The CWD number is changed to 24 whenever I do the third unlink. I'm not sure why this is the case.

(3) My symlink has the hi -> but then the right side of the arrow has a strange character when it's supposed to say newdir 

Fixed:
(1) Fixed!! It turns out I was missing my third entry in my sscanf in my main function. Once I put that in, it fixed the issue.

(2)Technically not fixed yet, but I found out that if I do 2 unlinks, it works fine. But when I do 3 unlinks, it blows up and loops infinitely. Try putting a getchar within my iteration loop to see how far it goes. It might be going beyond the bounds of the block. UPDATE: FIXED! I was missing an iput in my getino() function in util_cougar.c

(3) Fixed!! Turns out I didn't have a strcpy in my symlink function. Without the strcpy, my ls function was not getting the mip->INODE.i_block information.


**********************************************/
void myLink(char *oldPathname, char *newPathname)
{
	if(strlen(oldPathname)==0)
	{
		printf("Link of old pathname is required\n");
		return;
	}

	char pNewPathname[BLKSIZE], cNewPathname[BLKSIZE];
	char *ptr_pathname;
	char *ptr_pNewPathname, *ptr_cNewPathname;

	int ino_oldPathname;
	int ino_newPathname;

	MINODE *mip;
	MINODE *new_parent_mip;

	strcpy(pNewPathname, newPathname);
	strcpy(cNewPathname, newPathname);
	
	ino_oldPathname=getino(oldPathname);
	mip=iget(running->cwd->dev, ino_oldPathname);

	//check if the pathname is reg or link file
	if(S_ISDIR(mip->INODE.i_mode))
	{
		printf("Can't link to the directory\n");
		iput(mip);
		return;
	}
	ptr_pNewPathname = dirname(pNewPathname);	//parent directory of pathname2
	ptr_cNewPathname = basename(cNewPathname);	//child of pathname2

	ino_newPathname=getino(ptr_pNewPathname);
	if(ino_newPathname==0)
	{
		printf("New path's parent's dir doesn't exist\n");
		iput(mip);
		return;
	}

	new_parent_mip=iget(running->cwd->dev, getino(ptr_pNewPathname));
	
	if(!S_ISDIR(new_parent_mip->INODE.i_mode))
	{
		printf("New path's parent is not DIR\n");
		iput(mip);
		iput(new_parent_mip);
		return;
	}

	if(getino(newPathname)!=0)	//check if new path exists
	{
		printf("New pathname is already existing\n");
		iput(new_parent_mip);
		iput(mip);
		return;
	}

	mip->INODE.i_links_count++;
	mip->dirty=1;

	enter_name(new_parent_mip, ino_oldPathname, ptr_cNewPathname);
	//mip->INODE.i_links_count++;
	iput(new_parent_mip);
	iput(mip);

	return;
}
//Reducing the block size.
void mytruncate(MINODE *mip)
{
	INODE *ip = &mip->INODE;
	int blocks = ip->i_size/BLKSIZE + 1;
	int remaining = blocks;
	int i;
	char buf[BLKSIZE];
	char *cp;
	DIR *dp;

	for(i=0; i<12; i++)
	{
		bdealloc(running->cwd->dev, ip->i_block[i]);
		remaining--;
		ip->i_block[i] = 0;
	}
	if(remaining > 0)	//we have things in the indirect blocks
	{
		get_block(dev, ip->i_block[12], buf);
		cp = buf;
		dp = (DIR *)buf;

		while(cp < buf + BLKSIZE && remaining && dp->inode)
		{
			bdealloc(running->cwd->dev, dp->inode);
			cp += 4;
			dp = (DIR *)cp;
			remaining--;
		}
		bdealloc(running->cwd->dev, ip->i_block[12]);
		ip->i_block[12] = 0;
	}
	
	if(remaining > 0)	//this means there is something in the double indirect blocks
	{
		get_block(dev, ip->i_block[13], buf);
		cp = buf;
		dp = (DIR *)buf;

		while(cp < buf + BLKSIZE && remaining && dp->inode)
		{
			get_block(dev, dp->inode, buf);
			while(cp < buf + BLKSIZE && remaining && dp->inode)
			{
				bdealloc(running->cwd->dev, dp->inode);		
				cp += 4;
				dp = (DIR *)cp;
				remaining--;
			}
			bdealloc(running->cwd->dev, dp->inode);
		}
		bdealloc(running->cwd->dev, ip->i_block[13]);
		ip->i_block[13] = 0;
	}

	ip->i_size = 0;
	mip->dirty = 1;
	ip->i_atime = time(0L);
	ip->i_mtime = time(0L);
	ip->i_ctime = time(0L);

	//iput(mip);
	
}
//unlink a file and deleting the file if its link count reaches 0.
void myUnlink(char *pathname)
{
	char parentName[BLKSIZE], childName[BLKSIZE];
	char *pName, *cName;

	if(strlen(pathname)==0)
	{
		printf("Unlink needs pathname\n");
		return;
	}

	printf("Unlink pathname\n");

	MINODE *mip;
	int ino_pathname;
	ino_pathname=getino(pathname);

	printf("ino_pathname: %d\n", ino_pathname);
	mip=iget(running->cwd->dev, ino_pathname);

	mip->INODE.i_links_count--;
	mip->dirty=1;

	if(mip->INODE.i_links_count==0)
	{
		mytruncate(mip);
		idealloc(running->cwd->dev, mip->ino);
	}
	strcpy(parentName, pathname);
	strcpy(childName, pathname);
	pName=dirname(parentName);
	cName=basename(childName);

	int pino=getino(pName);
	MINODE *pip;
	pip=iget(running->cwd->dev, pino);

	rm_child(pip, cName);	//remove child from the parent dir
	iput(mip);
	iput(pip);
	return;
}

void mySymlink(char *pathname, char *newPathname)//creates a Symbolic link from newpath to oldpath.
{
	char pNewPathname[BLKSIZE], cNewPathname[BLKSIZE];
	char *ptr_pNewPathname, *ptr_cNewPathname;
	int ino_newPathname;

	if(strlen(pathname)==0)
	{
		printf("Symlink needs pathname\n");
		return;
	}

	if(getino(newPathname)!=0)
	{
		printf("New pathname already exists\n");
		return;
	}

	strcpy(pNewPathname, newPathname);
	strcpy(cNewPathname, newPathname);
	
	ptr_pNewPathname=dirname(pNewPathname);	//parent dir of pathname2
	ptr_cNewPathname=basename(cNewPathname);	//child of pathname2

	MINODE *mip;
	int ino_pathname;

	mip=iget(running->cwd->dev, getino(pathname));
	if(!S_ISDIR(mip->INODE.i_mode) && !S_ISREG(mip->INODE.i_mode))
	{
		printf("Pathname-1 should be either a DIR or a REG file\n");
		
		iput(mip);
		return;
	}

	MINODE *pip;
	pip=iget(running->cwd->dev, getino(ptr_pNewPathname));
	/*if(!S_ISDIR(pip->INODE.i_mode))	//new path's parent dir is not a directory
	{
		iput(mip);
		iput(pip);
		return;
	}*/

	creat_file(newPathname);	//create

	ino_pathname=getino(newPathname);
	mip=iget(running->cwd->dev, ino_pathname);	//new path = mip

	mip->INODE.i_mode=0xA000;
	mip->INODE.i_mode=(mip->INODE.i_mode & 0xF000) | 0777;	//change to link and give permission
	
	//strcpy(mip->INODE.i_block, pathname);

	int symBno=mip->INODE.i_block[0];
	mip->dirty=1;

	char buf[BLKSIZE];
	memset(buf, '\0', BLKSIZE);
	get_block(running->cwd->dev, symBno, buf);
	
	if(strlen(pathname) < 84)
	{
		strncpy(buf, pathname, strlen(pathname));
		buf[strlen(pathname)] = '\0';
		mip->INODE.i_size=strlen(pathname);
		put_block(running->cwd->dev, symBno, buf);
	}
	else
	{
		strncpy(buf, pathname, 83);
		buf[83] = '\0';
		mip->INODE.i_size=strlen(pathname);
		put_block(running->cwd->dev, symBno, buf);
	}
	
	iput(mip);
	iput(pip);

	return;
}
