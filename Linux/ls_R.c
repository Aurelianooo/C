#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#define PATH_LEN 256

void fetchdir(const char *dir, void (*func)(char *))
{
	char name[PATH_LEN];
	struct dirent *sdp;
	DIR *dp;

	if ((dp = opendir(dir)) == NULL)
	{
		fprintf(stderr, "fetchdir: can't open %s\n", dir);
		return;
	}
	while ((sdp = readdir(dp)) != NULL)
	{
		if (strcmp(sdp->d_name, ".") == 0 || strcmp(sdp->d_name, "..") == 0)
			continue;
		if (strlen(dir) + strlen(sdp->d_name) + 2 > sizeof(name))
			fprintf(stderr, "fetchdir: name %s %s too long\n", dir, sdp->d_name);
		else
		{
			sprintf(name, "%s/%s", dir, sdp->d_name);
			(*func)(name);
		}
	}
	closedir(dp);
}

void isfile(char *name)
{
	struct stat sbuf;
	if (stat(name, &sbuf) == -1)
	{
		fprintf(stderr, "isfile: can't access %s\n", name);
		exit(1);
	}
	if ((sbuf.st_mode & S_IFMT) == S_IFDIR)
	{
		fetchdir(name, isfile);
	}
	printf("%8ld %s\n", sbuf.st_size, name);
}

int main(int argc, char *argv[])
{
	if (argc == 1)
		isfile(".");
	else
		while (--argc > 0)
			isfile(*++argv);
	return 0;
}
