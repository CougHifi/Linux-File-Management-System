/*************************************************************************
	Level 1 - Cougar Fischer
	rmdir.c
	Resources: Jin Park, Jimmy Zheng, Professor Wang, Nikita Fischer
*************************************************************************/



int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // inc free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

void idealloc(int dev, int ino)  // deallocate an ino number
{
  int i;  
  char buf[BLKSIZE];

  if (ino > ninodes){
    printf("inumber %d out of range\n", ino);
    return;
  }

  // get inode bitmap block
  get_block(dev, bmap, buf);
  clr_bit(buf, ino-1);

  // write buf back
  put_block(dev, imap, buf);

  // update free inode count in SUPER and GD
  incFreeInodes(dev);
}

void bdealloc(int dev, int blk) // deallocate a blk number
{
  // WRITE YOUR OWN CODE to deallocate a block number blk
	char buf[BLKSIZE];
	
	if(blk>nblocks)
	{
		printf("Bit %d out of range\n", blk);
		return;
	}
	get_block(dev, bmap, buf);
	clr_bit(buf, blk-1);
	put_block(dev, bmap, buf);
	incFreeInodes(dev);
}

//check if directory is empty
int isDirEmpty(MINODE *mip)
{
	char buf[1024];
	INODE *ip = &mip->INODE;
	char *cp;
	char name[64];
	DIR *dp;

	//if link count is greater than 2, it has files
	if(ip->i_links_count > 2)
	{
		return 1;
	}
	else if(ip->i_links_count == 2)
	{
		//link count of 2 could still have files
		if(ip->i_block[1])
		{
			get_block(dev, ip->i_block[1], buf);
			cp = buf;
			dp = (DIR *)buf;

			while(cp < buf + 1024)
			{
				strncpy(name, dp->name, dp->name_len);
				name[dp->name_len] = 0;

				if(strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
				{
					return 1;	//not empty
				}
			}
		}
	}
	else
	{
		return 0;	//is empty
	}
}
//removes the dir_entry of name from a parent directory minode pointed by pmip.
void rm_child(MINODE *parent, char *name)
{
	int i;
	INODE *pip = &parent->INODE;
	DIR *dp;
	DIR *prevdp;
	DIR *lastdp;
	char buf[1024];
	char *cp;
	char temp[64];
	char *lastcp;
	int start, end;

	printf("name: %s\n", name);
	printf("pip->i_size = %d\n", pip->i_size);

	//go through the blocks of the parent, find the node to remove
	for (i=0; i<12; i++)
	{
		if(pip->i_block[i] == 0) //segmentation fault
		{
			return;
		}

		get_block(dev, pip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)buf;

		while(cp < buf + BLKSIZE)
		{
			//copy the name of this file into temp for all chars
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			
			printf("dp -> %s\n", temp);

			if(strcmp(temp, name)==0)
			{
				printf("found it\n");
		
				if(cp + dp->rec_len == buf + 1024)	//remove the last entry
				{
					//just the last entry
					prevdp->rec_len += dp->rec_len;
					put_block(dev, pip->i_block[i], buf);
				}
				else if(cp == buf && cp + dp->rec_len == buf + BLKSIZE)
				{
					//the first and only entry
					free(buf);
					bdealloc(dev, ip->i_block[i]);
	
					pip->i_size -= BLKSIZE;

					while(pip->i_block[i + 1] && i + 1 < 12)
					{
						i++;
						get_block(dev, pip->i_block[i], buf);
						get_block(dev, pip->i_block[i - 1], buf);
					}
				}
				else	//middle of the block
				{
					//set lastdp equal to the pointer
					lastdp = (DIR *)buf;
					lastcp = buf;

					//step into the last entry
					while(lastcp + lastdp->rec_len < buf + BLKSIZE)
					{
						//move the pointer until we reach the end
						printf("lastdp -> %s\n", lastdp->name);
						lastcp += lastdp->rec_len;
						lastdp = (DIR *)lastcp;
					}

					lastdp->rec_len += dp->rec_len;

					start = cp + dp->rec_len;
					end = buf + BLKSIZE;

					memmove(cp, start, end - start);	//move memory left
					put_block(dev, pip->i_block[i], buf);
				}
				parent->dirty = 1;
				//iput(parent);
				return;
			}
			prevdp = dp;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}

	}

	return;

	/*char name_temp[BLKSIZE];
	strcpy(name_temp, name);
	char *cp, buf[BLKSIZE];
	char *cp2, buf2[BLKSIZE];
	DIR *dp2;
	int loopNum = 0;
	int prev_rec;
	int i, q;

	for(i=0; i<12; i++)
	{
		if(pip->INODE.i_block[i]==0)
		{
			printf("Invalid request\n");
			return;
		}
		
		get_block(pip->dev, pip->INODE.i_block[i], buf);
		get_block(pip->dev, pip->INODE.i_block[i], buf2);

		cp = buf;
		dp = (DIR *)buf;

		//Case 1: when the element is the first and only entry in the block
		if(dp->rec_len==1024)
		{
			pip->INODE.i_size -= BLKSIZE; //reduce parent's file size by BLKSIZE
			bdealloc(running->cwd->dev, pip->INODE.i_block[i]); //deallocate the block

			for(q=i; q<11; q++)
			{
				pip->INODE.i_block[q]=pip->INODE.i_block[q+1];
			}
			return;
		}
		//Case 2: when the element is the LAST entry in the block
		get_block(running->cwd->dev, pip->INODE.i_block[i], buf);	//get the latest block into a buf
		dp = (DIR *)buf;
		cp = buf;

		while(cp+dp->rec_len < buf + BLKSIZE)	//make dp point to last entry
		{
			cp += dp->rec_len;
			dp = (DIR *)cp;

		}	//now dp points at last entry in block


		if((strcmp(dp->name, name_temp)==0 && dp->rec_len > 4*((8+dp->name_len+3)/4)))
		{
			dp->rec_len+=dp->rec_len;
			dp->inode=0;
			dp->file_type = 0;	//remove the entries
			dp->name_len = 0;
			strcpy(dp->name, " ");
			put_block(running->cwd->dev, pip->INODE.i_block[i], buf);
			pip->dirty=1;
			return;
		}

			
		//Might not need the cp2 and the dp2 - seems like the teacher only uses 1 of each not 2 of each
		cp2 = buf2;
		dp2 = (DIR *)buf2;

		cp = cp+dp->rec_len;	//from here, dp goes one step further than dp2 does
		dp = (DIR *)cp;

		while(cp<buf+BLKSIZE)
		{
			if((strcmp(dp->name, name_temp)==0 && dp->rec_len > 4*((8+dp->name_len+3)/4)))
			{
				dp->rec_len+=dp->rec_len;
				dp->inode=0;
				dp->file_type = 0;	//remove the entries
				dp->name_len = 0;
				strcpy(dp->name, " ");
				put_block(running->cwd->dev, pip->INODE.i_block[i], buf2);
				pip->dirty=1;
				return;
			}
			
			if(strcmp(dp2->name, name_temp)==0)
			{
				while(cp+dp->rec_len<buf+BLKSIZE)
				{
					dp2->file_type=dp->file_type;
					dp2->inode=dp->inode;
					dp2->name_len=dp->name_len;	//move to the left except the last loop
					dp2->rec_len=dp->rec_len;
					strncpy(dp2->name, dp->name, dp->name_len);

					cp=cp+dp->rec_len;
					dp=(DIR *)cp;

					cp2=cp2+dp2->rec_len;
					dp2=(DIR *)cp2;

				}
				
				dp->file_type=dp->file_type;
				dp2->inode=dp->inode;
				dp2->name_len = dp->name_len; 	//move to the left except the last loop
				dp2->rec_len+=dp->rec_len;
				strncpy(dp2->name, dp->name, dp->name_len);

				printf("middle entry is now done\n");
	
				put_block(running->cwd->dev, pip->INODE.i_block[i], buf2);
				return;
			}

			cp=cp+dp->rec_len;
			dp=(DIR *)cp;

			cp2=cp2+dp2->rec_len;
			dp2=(DIR *)cp2;
			loopNum++;
		}
	}*/
}

int rmDir()
{
	int i;
	int ino, parent_ino;
	MINODE *mip;
	MINODE *pmip;
	INODE *ip;
	INODE *pip;
	char temp[64], child[64];

	if(!pathname)
	{
		printf("No pathname was given\n");
		return;
	}

	strcpy(temp, pathname);
	strcpy(child, basename(temp));

	ino = getino(pathname);
	printf("Pathname: %s ino = %d\n", pathname, ino);
	mip = iget(dev, ino);

	if(!mip)
	{
		printf("Mip does not exist\n");
		return;
	}

	if(running->uid != 0 && running->uid != mip->INODE.i_uid)	//check if it's the super user
	{
		printf("You are not a superuser.\n");	//step 4
		iput(mip);
		return;
	}

	//check if dir
	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("Not a directory\n");
		return;
	}

	//check if empty, besides the . and ..
	if(isDirEmpty(mip))
	{
		printf("Directory is not empty\n");
		return 0;
	}

	ip = &mip->INODE;

	parent_ino = findino(mip, &ino);
	printf("ino = %d\n parent ino = %d\n", ino, parent_ino);
	//set pmip to point at the parent minode
	pmip = iget(dev, parent_ino);
	//pip points directly at the parent inode for removal tasks
	pip = &pmip->INODE;

	//Assume passed above checks
	//Deallocate its block and inode
	for(i=0; i<12 && ip->i_block[i] != 0; i++)
	{
		bdealloc(dev, ip->i_block[i]);
	}
	//blocks for the dir to remove have now have been deallocated
	//deallocate child inode
	idealloc(dev, ino);

	rm_child(pmip, child);	//remove entry from parent dir
	pip->i_links_count--;	//decrement pip's link_count by 1
	pip->i_atime = time(0L);	//touch pip's atime, mtime fields
	pip->i_mtime = time(0L);
	pmip->dirty = 1;	//mark pip dirty

	//write parent changes to disk
	iput(pmip);
	//write changes to deleted directory to disk
	mip->dirty = 1;
	iput(mip);

	return;
}

/*void Myrmdir(char *pathname)
{
	if(strcmp(pathname, ".")==0 || strcmp(pathname, "..")==0)	//check if the pathname is . or ..
	{
		printf("You can't remove . or ..\n");
		return;		//if they are, return
	}
	
	if(strlen(pathname)==0 || getino(pathname)==0)	//if there is no input given, then return
	{
		printf("Right rmdir pathname is required\n");
		return;
	}

	char parent_pathname[BLKSIZE], child_name[BLKSIZE];
	int parent_ino;
	char *ptr_parent_pathname, *ptr_child_name;

	strncpy(parent_pathname, pathname, strlen(pathname));
	strncpy(child_name, pathname, strlen(pathname));

	ptr_parent_pathname=dirname(parent_pathname);
	ptr_child_name=basename(child_name);

	MINODE *mip;
	int ino_rmdir;
	ino_rmdir=getino(pathname);	//(2)

	mip=iget(running->cwd->dev, ino_rmdir);	//(3)

	if(running->uid != 0 && running->uid != mip->INODE.i_uid)	//check if it's the super user
	{
		printf("You are not a superuser.\n");	//step 4
		iput(mip);
		return;
	}

	//check the DIR type
	if(S_ISDIR(mip->INODE.i_mode)==0)	//check if the pathname inode is a directory
	{
		printf("It is not a directory\n");
		iput(mip);
		return;
	}

	//check if it's busy
	if(mip->refCount > 1)
	{
		printf("Directory is currently busy\n");	//check if the directory is busy using refCount
		printf("RefCount = %d\n", mip->refCount);
		iput(mip);
		return;
	}

	//check if it's empty
	char *cp, buf[BLKSIZE];
	int j =0;
	//int empty_check = 0;
	int i;

	for(i=0; i<12; i++)
	{
		if(mip->INODE.i_block[i]==0)
		{
			j=i-1;	//get the latest i-th block
			break;
		}
	}

	get_block(mip->dev, mip->INODE.i_block[j], buf);
	cp = buf;
	dp = (DIR *)buf;
	
	/*while(cp<buf+BLKSIZE)
	{
		cp = cp+dp->rec_len;	//count the number of elements in the directory
		dp=(DIR *)cp;
		empty_check++;
	}*/

	//check if empty
	/*if(mip->INODE.i_links_count > 2)
	{
		printf("Directory is not empty\n");
		iput(mip);	
		return;
	}

	for(i=0; i<12;i++)
	{
		if(mip->INODE.i_block[i]==0)
		{
			continue;
		}
		bdealloc(mip->dev, mip->INODE.i_block[i]);
	}
	idealloc(mip->dev, mip->ino);
	iput(mip);

	parent_ino = getino(ptr_parent_pathname);	//(7)
	MINODE *parent_mip;
	parent_mip = iget(mip->dev, parent_ino);

	rm_child(parent_mip, ptr_child_name);

	parent_mip->INODE.i_links_count--;
	parent_mip->INODE.i_atime=time(0L);
	parent_mip->INODE.i_mtime=time(0L);
	parent_mip->dirty=1;
	iput(parent_mip);
	return;
	
}*/

/*
========================================================================
  
Assume: command line = "rmdir pathname"

1. Extract cmd, pathname from line and save them as globals.

   Do NOT rmdir . or .. or /

int rmdir()
{
  2. get inumber of pathname: 
         ino = getino(pathname) 
  3. get its minode[ ] pointer:
         mip = iget(dev, ino);

  4. check ownership 
       super user : OK
       not super user: uid must match
 
  ------------------------------------------------------------------------
  5. check DIR type (HOW?), not BUSY (HOW?), is empty:

     HOW TO check whether a DIR is empty:
     First, check link count (links_count > 2 means not empty);
     However, links_count = 2 may still have FILEs, so go through its data 
     block(s) to see whether it has any entries in addition to . and ..

     if (NOT DIR || BUSY || not empty): iput(mip); retunr -1;

  6. ASSUME passed the above checks.
     Deallocate its block and inode
     for (i=0; i<12; i++){
         if (mip->INODE.i_block[i]==0)
             continue;
         bdealloc(mip->dev, mip->INODE.i_block[i]);
     }
     idealloc(mip->dev, mip->ino);
     iput(mip); (which clears mip->refCount = 0);
     

  7. get parent DIR's ino and Minode (pointed by pip);
         pip = iget(mip->dev, parent's ino); 

  8. remove child's entry from parent directory by

        rm_child(MINODE *pip, char *name);
           
        pip->parent Minode, name = entry to remove

  9. decrement pip's link_count by 1; 
     touch pip's atime, mtime fields;
     mark pip dirty;
     iput(pip);
     return SUCCESS;
}

// rm_child(): remove the entry [INO rlen nlen name] from parent's data block.

int rm_child(MINODE *parent, char *name)
{
   1. Search parent INODE's data block(s) for the entry of name

   2. Erase name entry from parent directory by
    
  (1). if LAST entry in block{
                                         |remove this entry   |
          -----------------------------------------------------
          xxxxx|INO rlen nlen NAME |yyy  |zzz                 | 
          -----------------------------------------------------

                  becomes:
          -----------------------------------------------------
          xxxxx|INO rlen nlen NAME |yyy (add zzz len to yyy)  |
          -----------------------------------------------------

      }
    
  (2). if (first entry in a data block){
          deallocate the data block; modify parent's file size;

          -----------------------------------------------
          |INO Rlen Nlen NAME                           | 
          -----------------------------------------------
          
          Assume this is parent's i_block[i]:
          move parent's NONZERO blocks upward, i.e. 
               i_block[i+1] becomes i_block[i]
               etc.
          so that there is no HOLEs in parent's data block numbers
      }

  (3). if in the middle of a block{
          move all entries AFTER this entry LEFT;
          add removed rec_len to the LAST entry of the block;
          no need to change parent's fileSize;

               | remove this entry   |
          -----------------------------------------------
          xxxxx|INO rlen nlen NAME   |yyy  |zzz         | 
          -----------------------------------------------

                  becomes:
          -----------------------------------------------
          xxxxx|yyy |zzz (rec_len INC by rlen)          |
          -----------------------------------------------

      }
    
  3. Write the parent's data block back to disk;
     mark parent minode DIRTY for write-back
}
*/

