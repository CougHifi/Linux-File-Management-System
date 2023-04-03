/************************************************************************************************************************************
	Level 2 - Cougar Fischer
	open_close_lseek.c
	Resources: Jovan Araiza, Professor Wang, Yu Nong, Jin Park, Christopher Roberts, Zhila Esfahani, Jimmy Zheng, Ryan Neisess,Nikita Fischer
*************************************************************************************************************************************/

/**************************************************************************************************************************
	Issues:
	In your open() function, should NOT iput(mip), so that the opened file's INODE
	stays in memory. It will be iput() when the file descriptor is closed later.

	In your case, fd -> myoft -> mionde; although minode location did not change
	but that SAME minode slot is taken over by the newly created file ==> ino will change in pfd().
***************************************************************************************************************************/


int open_file(char *pathname, char *pathname2)
{
	int dev, ino, i;
	MINODE *mip;
	OFT *myoft;
	int mode;

	printf("In open_file: %s\n", pathname);
	sscanf(pathname2, "%d\n", &mode);

	if (pathname[0]=='/') 
	{
		dev = root->dev;          // root INODE's dev
	}
	else                  
	{
		dev = running->cwd->dev; 
	} 
	ino = getino(pathname);

	mip = iget(dev, ino);

	if(!S_ISREG(mip->INODE.i_mode))
	{
		printf("Not a regular file\n");
		return -1;
	}

	for(i=0; i < NFD; i++)
	{
		if(running->fd[i] != 0)
		{
			if(running->fd[i]->mptr == mip)
			{
				if(running->fd[i]->mode != 0)
				{
					printf("File is already open in incompatible\n");
					iput(mip);
					return -1;
				}
			}
		}
	}
	
	myoft = (OFT *)malloc(sizeof(OFT));

	myoft->mode = mode;
	myoft->mptr = mip;
	myoft->refCount = 1;

	switch(mode)
	{
         case 0 : myoft->offset = 0;     // R: offset = 0
                  break;
         case 1 : mytruncate(mip);        // W: truncate file to 0 size (we want to overwrite what's here when we open for Write, hence use truncate)
                  myoft->offset = 0;
                  break;
         case 2 : myoft->offset = 0;     // RW: do NOT truncate file
                  break;
         case 3 : myoft->offset =  mip->INODE.i_size;  // APPEND mode
                  break;
         default: printf("invalid mode\n");
                  return(-1);
	}

	for(i=0; i<NFD; i++)
	{
		if(running->fd[i] == 0)
		{
			running->fd[i] = myoft;
			break;
		}
	}

	if(mode != 0)
	{
		mip->INODE.i_mtime = time(0L);
	}
	mip->INODE.i_atime = time(0L);
	mip->dirty = 1;

	//iput(mip);

	return i;
}

int close_file(char *pathname)
{
	int fd;
	OFT *myoft;
	MINODE *mip;

	sscanf(pathname, "%d\n", &fd);

	if(fd < 0 || fd >= NFD) //fd can't be less than 0 or greater than NFD's size (which is 8)
	{
		printf("Invalid fd\n");
		return -1;
	}

	if(running->fd[fd] == 0)
	{
		printf("Did not provide a valid fd\n");
		return -1;
	}

	myoft = running->fd[fd];
	running->fd[fd] = 0;
	myoft->refCount--;
	if (myoft->refCount > 0) 
	{
		return 0;
	}

	// last user of this OFT entry ==> dispose of the Minode[]
	mip = myoft->mptr;
	iput(mip);

	return 0;
}

int mylseek(char *pathname, char *pathname2)
{
	int fd, position;
	OFT *myoft;
	int originalPosition;

	sscanf(pathname, "%d", &fd);
	sscanf(pathname2, "%d\n", &position);

	//From fd, find the OFT entry. 

	if(fd < 0 || fd >= NFD) //fd can't be less than 0 or greater than NFD's size (which is 8)
	{
		printf("Invalid fd\n");
		return -1;
	}

	if(running->fd[fd] == 0)
	{
		printf("Did not provide a valid fd\n");
		return -1;
	}

	myoft = running->fd[fd];

	//change OFT entry's offset to position but make sure NOT to over run either end
	//of the file.
	
	if(position < 0 || position >= myoft->mptr->INODE.i_size)
	{
		printf("Position is invalid\n");
		return -1;
	}

	originalPosition = myoft->offset;
	myoft->offset = position;


	return originalPosition;
}

int pfd() 
{
	int i;
	char mode[6];

	printf(" fd  mode    offset   INODE\n");
	printf(" --  ----    ------   -----\n");


	for(i=0; i <= NFD; i++) 
	{
		if(!running->fd[i])
		{
			continue;
		}

		switch(running->fd[i]->mode)
		{
			case 0: strcpy(mode, "READ");
				break;

			case 1: strcpy(mode, "WRITE");
				break;
			
			case 2: strcpy(mode, "RW");
				break;

			case 3: strcpy(mode, "APPEND");
				break;
		}

		printf(" %d  %s      %d      [%d %d]\n", i, mode, running->fd[i]->offset, dev, running->fd[i]->mptr->ino);
	}
}
//Does this possibly iterate through the NFD to see what files are open and what their fds are? UPDATE: The book states that "The OFT's refCount represents
//the number of processes that share the same instance of an opened file. When a process opens a file, the refCount in the OFT is set to 1."

