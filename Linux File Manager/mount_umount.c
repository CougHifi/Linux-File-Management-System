/***********************************************************************************************************************
	Level 3 - Cougar Fischer
	mount_umount.c
	Resources: Jovan Araiza, Professor Wang, Yu Nong, Jin Park, Christopher Roberts, Zhila Esfahani, Jimmy Zheng, Ryan Neisess, Nikita Fischer
***********************************************************************************************************************/

#include "type.h"
MOUNT mountTable[8];  // set all dev = 0 in init()

int mount(char filesys, char mount_point)
{
	/**************************************************
	1. Ask for filesys (a pathname) and mount_point (a pathname also).
   	If mount with no parameters: display current mounted filesystems.

	2. Check whether filesys is already mounted: 
   	(you may store the name of mounted filesys in the MOUNT table entry). 
   	If already mounted, reject;
   	else: allocate a free MOUNT table entry (whose dev=0 means FREE).

	3. open filesys for RW; use its fd number as the new DEV;
  	 Check whether it's an EXT2 file system: if not, reject.

	4. For mount_point: find its ino, then get its minode:
    	call  ino  = getino(pathname);  to get ino:
    	call  mip  = iget(DEV, ino);    to get its minode in memory;    

	5. Check mount_point is a DIR.  
   	Check mount_point is NOT busy (e.g. can't be someone's CWD)

	6. Record new DEV in the MOUNT table entry;

  	(For convenience, store the filesys name in the Mount table, and also its
                    ninodes, nblocks, bitmap blocks, inode_start block, etc. 
   	for quick reference)

	7. Mark mount_point's minode as being mounted on and let it point at the
   	MOUNT table entry, which points back to the mount_point minode.

	. return 0 for SUCCESS;
	**************************************************/
	
	
}

umount(char *filesys)
{
}
