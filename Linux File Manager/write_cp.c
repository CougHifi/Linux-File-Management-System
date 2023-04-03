/**********************************************************************************************************************************
	Level 2 - Cougar Fischer
	write_cp.c
	Resources: Jovan Araiza, Professor Wang, Yu Nong, Jin Park, Christopher Roberts, Zhila Esfahani, Jimmy Zheng, Ryan Neisess
***********************************************************************************************************************************/
OFT *myoft;
INODE *ip;
MINODE *mip;
int write_file(int fd, char buf[])
{
	int nbytes;
	if(running->fd[fd])
	{
		if(running->fd[fd]->mode > 0 && running->fd[fd]->mode <= 3)
		{
			nbytes = strlen(buf);
			return(mywrite(fd, buf, nbytes));
		}
	}
}

int mywrite(int fd, char buf[], int nbytes)
{
	int count = 0, *dibptr;
	int available, lblk, start_byte, filesize, remaining, blk, bytes_to_copy;
	char writebuf[BLKSIZE], debuf[BLKSIZE], ibuf[BLKSIZE], zbuf[BLKSIZE];
	myoft = running->fd[fd];
	mip = myoft->mptr;
	ip = &mip->INODE;
	while(nbytes > 0)
	{
		lblk = myoft->offset / BLKSIZE;
		start_byte = myoft->offset % BLKSIZE;

		//write it for indirect and double indirect

		if(lblk < 12)	//direct blocks
		{
			if(mip->INODE.i_block[lblk] == 0)	//if there's no data block yet
			{
				mip->INODE.i_block[lblk] = balloc(mip->dev);	//must allocate a block
			}
			blk = mip->INODE.i_block[lblk];	//blk should be a disk block now
		}
		else if(lblk >= 12 && lblk < 256 + 12)	//indirect blocks
		{
			/*****************************************
			HELP INFO:
              		if (i_block[12] == 0){
                  		allocate a block for it;
                  		zero out the block on disk !!!!
             		 }
              		get i_block[12] into an int ibuf[256];
             		 blk = ibuf[lbk - 12];
              		if (blk==0){
                 		allocate a disk block;
                 		record it in i_block[12];
             		 }
              		.......
			******************************************/
			int *cp;
			if(mip->INODE.i_block[12] == 0)
			{
				mip->INODE.i_block[12] = balloc(mip->dev);	//allocate a block
				get_block(mip->dev, mip->INODE.i_block[12], ibuf);
				cp = ibuf;
				for(int i=0; i <=255; i++)
				{
					cp[i] = 0; //zero out block
				}
				put_block(mip->dev, mip->INODE.i_block[12], ibuf); 
				
			}
			get_block(mip->dev, mip->INODE.i_block[12], ibuf);	//get i_block[12] into an int ibuf[256]
			
			
			blk = cp[lblk - 12];

			if(blk == 0)
			{
				cp[lblk - 12] = balloc(mip->dev);//allocate a block
				put_block(mip->dev, mip->INODE.i_block[12], ibuf);
				blk = cp[lblk - 12]; //blk should be disk block now
								
			}
		}
		else	//double indirect blocks
		{
			int inum;
			int dib;

			if(mip->INODE.i_block[13] == 0)
			{
				mip->INODE.i_block[13] = balloc(mip->dev);//allocate a block			
				get_block(mip->dev, mip->INODE.i_block[13], ibuf);
				int *cp = ibuf;
				for(int i=0; i <=255; i++)
				{
					cp[i] = 0; //zero out block
				}
				put_block(mip->dev, mip->INODE.i_block[13], ibuf);
			}
			inum = mip->INODE.i_block[13];
			get_block(mip->dev, mip->INODE.i_block[13], ibuf);
			dibptr = (int *)ibuf + ((lblk - 268)/256); //this gives us the right index in the block that we want to read
			dib = *dibptr;
			if(dib == 0)
			{
				*dibptr = balloc(mip->dev);//allocate
				dib = *dibptr;
				put_block(mip->dev, mip->INODE.i_block[13], ibuf);
				get_block(mip->dev, dib, zbuf);
				int *cp = zbuf;
				for(int i=0; i <=255; i++)
				{
					cp[i] = 0; //zero out block
				}
				put_block(mip->dev, dib, zbuf);
			}
			get_block(mip->dev, dib, ibuf);
			dibptr = (int *) ibuf + ((lblk - 268)%256); // this gives us the right offset inside the block
			int dibx = *dibptr;
			if(dibx == 0)
			{
				*dibptr = balloc(mip->dev);//allocate
				dibx = *dibptr;
				put_block(mip->dev, dib, ibuf);				
				get_block(mip->dev, dibx, zbuf);
				int *cp = zbuf;
				for(int i=0; i <=255; i++)
				{
					cp[i] = 0; //zero out block
				}
				put_block(mip->dev, dibx, zbuf);
			}
			blk = dibx;
		}
		//if(lblk >= 268)
		//{
		//	printf("lblk = %d blk = %d\n",lblk, blk);
		//	getchar();
		//}
		//All cases come to here: write to data block
		get_block(mip->dev, blk, writebuf);	//read disk block into wbuf[]

		char *cp = writebuf + start_byte;	//cp points at start_byte in wbuf[]
		remaining = BLKSIZE - start_byte;	//number of bytes remain in this block
		
		/* Pseudocode from Wang's site
		while(remaining > 0)	//write as much as remaining allows
		{
			*cp++ = *cq++;	// cq points at buff[]
			nbytes--;
			remaining--;	dec counts
			myoft->offset++;
			
			if(offset > INODE.i_size)	//especially for RW|APPEND mode
			{
				mip->INODE.i_size++;	//inc file size (if offset >filesize)
			}

			if(nbytes <= 0)
			{
				break;
			}
		}*/
		if(nbytes <= remaining)
		{
			bytes_to_copy = nbytes;
		}
		else
		{
			bytes_to_copy = remaining;
		}

		memcpy(cp, (buf + count), bytes_to_copy);
		//memcpy(cp, buf, bytes_to_copy);
		myoft->offset += bytes_to_copy;
		count += bytes_to_copy;
		available -= bytes_to_copy;
		nbytes -= bytes_to_copy;
		cp += bytes_to_copy;
		

		
		put_block(mip->dev, blk, writebuf);	
	}

	if(myoft->offset > myoft->mptr->INODE.i_size)
	{
		myoft->mptr->INODE.i_size = myoft->offset; 
	}
	mip->dirty=1;	//mark mip dirty for iput()
	mip->INODE.i_size = myoft->offset;
	printf("wrote %d char into file descriptor fd = %d\n", nbytes, fd);
	return nbytes;

}

/********************************************************
As in read(), the above inner while(remain > 0) loop can be optimized:
Instead of copying one byte at a time and update the control variables on each 
byte, TRY to copy only ONCE and adjust the control variables accordingly.

REQUIRED: Optimize the write() code in your project.
********************************************************/

int mycp(char *src, char *dest) //still not working correctly - maybe problem is in mywrite
{
	int fd, gd;
	char buf[BLKSIZE], temp[BLKSIZE];
	/**************************************
	1. fd = open src for READ;

	2. gd = open dst for WR|CREAT; 

   	NOTE:In the project, you may have to creat the dst file first, then open it 
           for WR, OR  if open fails due to no file yet, creat it and then open it
           for WR.

	3. while( n=read(fd, buf[ ], BLKSIZE) ){
         write(gd, buf, n);  // notice the n in write()
	**************************************/

	fd = open_file(src, "0");
	if(fd < 0)
	{
		printf("Could not access the file\n");
		return -1;
	}
	creat_file(dest);
	gd = open_file(dest, "1");
	if(gd < 0)
	{
		printf("Could not access the file\n");
		return -1;
	}
	printf("copying now...\n");
	while(n = myread(fd, buf, BLKSIZE))
	{
		printf("writing..\n");
		mywrite(gd, buf, n);
	}
	printf("done copying\n");
	sprintf(temp, "%d", fd);
	close_file(temp);
	sprintf(temp, "%d", gd);
	close_file(temp);
}

int mymv(char *src, char *dest)
{
	/**************************************
	1. verify src exists; get its INODE in ==> you already know its dev
	2. check whether src is on the same dev as src

              	CASE 1: same dev:
	3. Hard link dst with src (i.e. same INODE number)
	4. unlink src (i.e. rm src name from its parent directory and reduce INODE's
               link count by 1).
                
              	CASE 2: not the same dev:
	3. cp src to dst
	4. unlink src
	**************************************/
	int srcino;	
	MINODE *mip;
	srcino = getino(src);
	if(!srcino)
	{
		printf("Source does not exist\n");
		return -1;
	}
	
	myLink(src, dest);
	//mip = iget(dev, srcino);
	//mip->INODE.i_links_count--;
	myUnlink(src);
	return 0;
}
