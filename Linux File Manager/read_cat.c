/**********************************************************************************************************************************
	Level 2 - Cougar Fischer
	read_cat.c
	Resources: Jovan Araiza, Professor Wang, Yu Nong, Jin Park, Christopher Roberts, Zhila Esfahani, Jimmy Zheng, Ryan Neisess,Nikita Fischer
***********************************************************************************************************************************/
//Reading the file
int read_file(char *pathname, char *pathname2)
{
	int fd, nbytes;
	sscanf(pathname, "%d", &fd);
	sscanf(pathname2, "%d", &nbytes);

	if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)
	{
		printf("Invalid mode\n");
		return -1;
	}

	char buf[nbytes+1];
	int n = myread(fd, buf, nbytes);
	buf[n] = 0;
	printf("%s\n", buf);
	return n;
}
/*behaves exactly the same as the read(fd, buf, nbytes) syscall of Unix/Linux. It tries to read nbytes from fd to buf[ ], and returns the 
     actual number of bytes read.*/

	 /*Is this optimized enough? Or do I need to improve on this?*/
int myread(int fd, char *buf, int nbytes)
{
	int count = 0, *dibptr;
	int available, lblk, start_byte, filesize, remaining, blk, bytes_to_copy;
	char readbuf[BLKSIZE], debuf[BLKSIZE], ibuf[BLKSIZE];
	
	OFT *myoft = running->fd[fd];
	if (myoft == 0)
	{
		printf("Error: Invalid fd\n");
	}
	MINODE *mip = myoft->mptr;
	filesize = mip->INODE.i_size;
	if(nbytes>filesize)
		nbytes=filesize;
	if(myoft->offset >= filesize)
	{
		return 0;
	}
	available = filesize - myoft->offset;//number of bytes still available in file.

	while(nbytes > 0 && available > 0)
	{
	
		lblk = myoft->offset/BLKSIZE; //tells us which block to read from
		start_byte = myoft->offset % BLKSIZE; //tells us how far in we are in the block
		
		if(lblk < 12) //direct
		{
			printf("Direct\n");
			blk = mip->INODE.i_block[lblk];
		}
		else if(lblk >= 12 && lblk < 256 + 12) //indirect
		{
			printf("Indirect\n");
			get_block(mip->dev, mip->INODE.i_block[12], debuf);
			
			int *cp = debuf;
			blk = cp[lblk - 12];
		}
		else //double indirect
		{
			printf("Double Indirect\n");
			get_block(mip->dev, mip->INODE.i_block[13], debuf);
			dibptr = (int *)debuf + ((lblk - 268)/256); //this gives us the right index in the block that we want to read
			
			get_block(mip->dev, *dibptr, debuf);
			dibptr = (int *) debuf + ((lblk - 268)%256); // this gives us the right offset inside the block

			blk = *dibptr;
		}
		printf("Reading...\n");
		get_block(mip->dev, blk, readbuf);
		
		char *cp = (readbuf + start_byte);
		remaining = BLKSIZE - start_byte;
		
		//if(nbytes < remaining)
		//{
		//	bytes_to_copy = nbytes;
		//}
		//else
		//{
		//	bytes_to_copy = remaining;
		//}


		if(nbytes <= remaining && nbytes <= available)
		{
			bytes_to_copy = nbytes;
		}
		else if(remaining <= nbytes && remaining <= available)
		{
			bytes_to_copy = remaining;
		}
		else
		{
			bytes_to_copy = available;
		}

		memcpy((buf + count), cp, bytes_to_copy);
		myoft->offset += bytes_to_copy;
		count += bytes_to_copy;
		available -= bytes_to_copy;
		nbytes -= bytes_to_copy;
		cp += bytes_to_copy;
	}

	if(myoft->offset > myoft->mptr->INODE.i_size)
	{
		myoft->offset = myoft->mptr->INODE.i_size; 
	}
	
	//iput(mip);

	return count;
}

/*int read_file(char *pathname, char *pathname2)
{
	/*************************************************
	 Preparations: 
	ASSUME: file is opened for RD or RW;
	ask for a fd  and  nbytes to read;
	verify that fd is indeed opened for RD or RW;
	return(myread(fd, buf, nbytes));
	*************************************************

	printf("Please provide an fd and the amount of bytes you would like to read\n");
	sscanf(pathname, "%d", &fd);
	sscanf(pathname2, "%d", &nbytes);

	if(running->fd[fd] == 0 || running->fd[fd] == 2) //if fd is open for RD or RW
	{
		continue;
	}
	return(myread(fd, buf, nbytes));
}*/

int cat(char *filename)
{
	char *mybuf, dummy = 0;	//a null char at end of mybuf[]
	int n;

	int fd = open_file(filename, "0");

	if(fd < 0)
	{
		return -1;
	}
	int size = running->fd[fd]->mptr->INODE.i_size;
	
	mybuf = malloc(size);

	myread(fd, mybuf, size);

	printf("%s\n", mybuf);

	free(mybuf);

	char temp[10];
	sprintf(temp, "%d", fd);
	close_file(temp);

	/*****************************************
	1. int fd = open filename for READ;
	2. while( n = read(fd, mybuf[1024], 1024)){
       		mybuf[n] = 0;             // as a null terminated string
       		// printf("%s", mybuf);   <=== THIS works but not good
       		spit out chars from mybuf[ ] but handle \n properly;
   	} 
	3. close(fd);
	*****************************************/
}
