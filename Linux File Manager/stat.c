/**************************************************************
	Level 1 - MISC Functions - Cougar Fischer
	stat.c
	Resources: Jin Park, Nikita Fischer
**************************************************************/

/*
1. stat filename: 
        struct stat myst;
        get INODE of filename into memory:
            int ino = getino(pathname);
            MINODE *mip = iget(dev, ino);
            copy dev, ino to myst.st_dev, myst.st_ino;
            copy mip->INODE fields to myst fields;
        iput(mip);
*/

void myStat(char *pathname)
{
	if(strlen(pathname)==0)	//current stat
	{
		MINODE *mip;
		mip=iget(running->cwd->dev, running->cwd->ino);
		printf("stat root\n");
		printf("***************stat***************\n");
		printf("dev = %d ino = %d mod = %d \n", mip->dev, mip->ino, mip->INODE.i_mode);
		printf("uid = %d gid = %d nlink = %d \n", mip->INODE.i_uid, mip->INODE.i_gid, mip->INODE.i_links_count);
		char *tempTime = ctime((time_t*)&mip->INODE.i_mtime);
		printf("size = %d time = %.16s \n", mip->INODE.i_size, tempTime);

		iput(mip);
		return;
	}
	if(getino(pathname)==0)
	{
		printf("Can't find the pathname\n");
		return;
	}

	printf("Stat pathname = %s\n", pathname);

	MINODE *mip;
	mip = iget(running->cwd->dev, getino(pathname));
	printf("stat root\n");
	printf("***********stat*************\n");
	printf("dev = %d ino = %d mod = %d\n", mip->dev, mip->ino, mip->INODE.i_mode);
	printf("uid = %d gid = %d nlink = %d\n", mip->INODE.i_uid, mip->INODE.i_gid, mip->INODE.i_links_count);
	char *tempTime = ctime((time_t*)&mip->INODE.i_mtime);
	printf("size = %d time = %.16s \n", mip->INODE.i_size, tempTime);

	iput(mip);

	return;
}
