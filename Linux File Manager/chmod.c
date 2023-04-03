/***********************************************************************
	Level 1 - MISC Functions - Cougar Fischer
	chmod.c
	Resources: Nikita Fischer, Jin Park
***********************************************************************/

#include <math.h>

/*
2. chmod filename mode: (mode = |rwx|rwx|rwx|, e.g. 0644 in octal)
         get INODE of pathname into memroy:
             ino = getino(pathname);
             mip = iget(dev, ino);
             mip->INODE.i_mode |= mode;
         mip->dirty = 1;
         iput(mip);
*/
//converting the values from octal to decimal.
int octalToDecimal(int n)
{
	int num = n;
	int dec_value = 0;

	//Initializing base value to 1, i.e. 8^0
	int base = 1;

	int temp = num;

	while(temp)
	{
		//Extracting last digit
		int last_digit = temp % 10;
		temp = temp/10;

		//Multiplying last digit with appropriate base value & adding it to dec_value
		dec_value += last_digit * base;

		base = base * 8;
	}
	return dec_value;
}

void myChmod(char *mode, char *pathname)
{
	//0 --- No permission
	//1 --x Execute permission
	//2 -w- Write permission
	//3 -wx Execute and write permission: 1 (execute) + 2 (write) = 3
	//4 r-- Read permission
	//5 Read and execute permission: 4 (read) + 1 (execute) = 5
	//6 rw- Read and write permission: 4 (read) + 2 (write) = 6
	//7 rwx All permissions: 4 (read) + 2 (write) + 1 (execute) = 7

	if(strlen(pathname)==0)
	{
		printf("Chmod needs a pathname\n");
		return;
	}
	if(getino(pathname)==0)
	{
		printf("Pathname doesn't exist\n");
		return;
	}

	if(strlen(mode)==0) // no mode input
	{
		printf("Change mode should be typed\n");
		fgets(mode, 128, stdin);
		mode[strlen(mode)-1] = 0;
	}
	
	if(strlen(mode)!=4)
	{
		printf("4 digits should be entered\n");
		return;
	}

	if(mode[0]!='0')
	{
		printf("First digit should be 0\n");
		return;
	}

	MINODE *mip;

	int num;
	int ino_pathname;

	num=atoi(mode);	//num has permission - this is octal & we need to convert it to decimal form

	if(num>777)
	{
		printf("Mode is not correct. In octal form, 0000~0777\n");
		return;
	}
	num = octalToDecimal(num);

	ino_pathname=getino(pathname);

	mip=iget(running->cwd->dev, ino_pathname);
		//adding the file type in octal form but num stores them in decimal form
	if(S_ISREG(mip->INODE.i_mode))
	{
		num += 0100000;	//regular file = 100000 in octal
	}
	if(S_ISDIR(mip->INODE.i_mode))
	{
		printf("Before num = %d\n", num);
		num += 0040000;	//directory = 40000 in octal
		printf("After num = %d\n", num);
	}
	if(S_ISLNK(mip->INODE.i_mode))
	{
		num+=0120000;	//symlink = 120000 in octal
	}
	//now the num had type & permission in octal form

	printf("num = %d\n", num);
	mip->INODE.i_mode = num;

	mip->dirty = 1;
	iput(mip);

	return;
}
