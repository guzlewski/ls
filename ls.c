#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define VERSION "1.3"

// Flagi globalne odpowiadają argumentom -l, -R, -t, -h.
// W zmiennej g_year przechowywany jest aktualny rok (potrebny do odpowiedniego formatowania wyjścia).
int g_l;
int g_R;
int g_t;
int g_h;
int g_year;

// Struktura zawiera wszystkie potrzebne informacje o pojedynczym pliku.
typedef struct File
{
	char type;
	char owner[4];
	char group[4];
	char other[4];
	int links;
	int ownerId;
	int groupId;
	long size;
	long int time;
	char name[256];
	int isDir;
} File;

// Funkcja otwiera wskazany folder (path), tworzy tablicę struktur File i uzupełnia ją. Ustawia wskaźnik files na tą tablicę, a do count zapisuje ilość elementów (plików).
void LoadDirectory(char *path, File **files, int *count);

// Funkcja odpowiada za poprawne wykonanie LoadDirectory, obsługę błędów, zwalnianie pamięci, obróbkę danych przed wyświetleniem w zależności od wybranych flag (w szczególności rekurencja).
void ProcessDirectory(char *path);

// Funkcja przetwarza informacje ze struktury File i opdowiednio je wyświetla
void PrintFile(File file);

// Wyświetla nformacje o błędzie w formacie: "ls: 'ŚCIEŻKA' NAZWA_FUNKCJI error" na standardowe wyjście błędów
void PrintError(char *folder, char *type);

// Funckcja łączy 2 fragmenty ścieżki i dodaje między nimi '/' o ile jest to konieczne.
char *PathCombine(char *first, char *second);

// Określa która struktura File jest większa (porównuje czas modyfikacji pliku). a > b <==> a jest nowszy niż b.
int FileCompare(const void *a, const void *b);

// Zamienia unix timestamp na napis. Alokuje pamięć - należy pamiętać o zwolnieniu.
char *TimeToString(unsigned long timestamp);

// Zamienia rozmiar pliku w bajtach na napis w jednostkach czytelnych dla człowieka (K, M, G, T, P) i zaokrągla do 1 miejsca po przecinku. Alokuje pamięć - należy pamiętać o zwolnieniu.
// Przykład 4,9K - 4,9 kilobajtów.
char *SizeBytesToString(unsigned long bytes);

// Funcja zwraca 1 jeśli udało się zmienić jednostkę rozmiaru (*x > 1024), zamienia type na 'wyższy' oraz dzieli x przez 1024.0.
int UpgradeSymbol(double *x, char *type);

// Zaokrągla matematycznie value do 1 miejsca po przecinku
double Round(double value);

void Version();
void Help(char *name);

int main(int argc, char **argv)
{
	int c;
	opterr = 0;

	while ((c = getopt(argc, argv, "lRth")) != -1)
	{
		switch (c)
		{
		case 'l':
			g_l = 1;
			break;
		case 'R':
			g_R = 1;
			break;
		case 't':
			g_t = 1;
			break;
		case 'h':
			g_h = 1;
			break;
		}
	}

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--version"))
			Version();
		else if (!strcmp(argv[i], "--help"))
			Help(argv[0]);
	}

	time_t s = time(NULL);
	struct tm *current_time = localtime(&s);
	g_year = current_time->tm_year;

	if (optind < argc)
		ProcessDirectory(argv[optind]);
	else
		ProcessDirectory(".");

	return 0;
}

void LoadDirectory(char *path, File **files, int *count)
{
	DIR *dir;
	struct dirent *dp;
	struct stat statbuf;
	int i = 0;

	if ((dir = opendir(path)) == NULL)
	{
		PrintError(path, "opendir");
		return;
	}

	while ((dp = readdir(dir)) != NULL)
		i++;

	closedir(dir);

	File *tab = (File *)malloc(i * sizeof(File));
	i = 0;

	if ((dir = opendir(path)) == NULL)
	{
		PrintError(path, "opendir");
		free(tab);
		return;
	}

	while ((dp = readdir(dir)) != NULL)
	{
		char *filepath = PathCombine(path, dp->d_name);

		if (stat(filepath, &statbuf) == -1)
		{
			PrintError(filepath, "stat");
			free(filepath);
			continue;
		}

		free(filepath);
		tab[i].isDir = 0;

		switch (dp->d_type)
		{
		case DT_BLK:
			tab[i].type = 'b';
			break;
		case DT_CHR:
			tab[i].type = 'c';
			break;
		case DT_DIR:
			tab[i].type = 'd';
			tab[i].isDir = 1;
			break;
		case DT_FIFO:
			tab[i].type = 'p';
			break;
		case DT_LNK:
			tab[i].type = 'l';
			break;
		case DT_REG:
			tab[i].type = '-';
			break;
		case DT_SOCK:
			tab[i].type = 's';
			break;
		default:
			tab[i].type = ' ';
			break;
		}

		tab[i].owner[0] = (statbuf.st_mode & S_IRUSR) ? 'r' : '-';
		tab[i].owner[1] = (statbuf.st_mode & S_IWUSR) ? 'w' : '-';
		tab[i].owner[2] = (statbuf.st_mode & S_IXUSR) ? 'x' : '-';
		tab[i].owner[3] = '\0';

		tab[i].group[0] = (statbuf.st_mode & S_IRGRP) ? 'r' : '-';
		tab[i].group[1] = (statbuf.st_mode & S_IWGRP) ? 'w' : '-';
		tab[i].group[2] = (statbuf.st_mode & S_IXGRP) ? 'x' : '-';
		tab[i].group[3] = '\0';

		tab[i].other[0] = (statbuf.st_mode & S_IROTH) ? 'r' : '-';
		tab[i].other[1] = (statbuf.st_mode & S_IWOTH) ? 'w' : '-';
		tab[i].other[2] = (statbuf.st_mode & S_IXOTH) ? 'x' : '-';
		tab[i].other[3] = '\0';

		tab[i].links = statbuf.st_nlink;
		tab[i].ownerId = statbuf.st_uid;
		tab[i].groupId = statbuf.st_gid;
		tab[i].size = statbuf.st_size;
		tab[i].time = statbuf.st_mtime;

		strncpy(tab[i].name, dp->d_name, 256);
		i++;
	}
	closedir(dir);

	if (g_t)
		qsort(tab, i, sizeof(File), FileCompare);

	*count = i;
	*files = tab;
}

void ProcessDirectory(char *path)
{
	int count = -1, dirs = 0, i;
	File *files = NULL;

	LoadDirectory(path, &files, &count);

	if (count < 0)
		return;

	for (i = 0; i < count; i++)
	{
		if (files[i].isDir)
			dirs++;
	}

	File tab[dirs];
	dirs = 0;

	if (g_R)
		printf("%s:\n", path);

	for (i = 0; i < count; i++)
	{
		PrintFile(files[i]);
		if (files[i].isDir && strcmp(files[i].name, ".") != 0 && strcmp(files[i].name, "..") != 0)
			tab[dirs++] = files[i];
	}

	if (!g_l)
		printf("\n");

	if (g_R)
		printf("\n");

	free(files);

	if (g_R)
	{
		for (i = 0; i < dirs; i++)
		{
			char *temp = PathCombine(path, tab[i].name);
			ProcessDirectory(temp);
			free(temp);
		}
	}
}

void PrintFile(File file)
{
	if (g_l)
	{
		struct passwd *pws = getpwuid(file.ownerId);
		struct group *grs = getgrgid(file.groupId);

		char *time = TimeToString(file.time);

		if (!g_h)
		{
			printf("%c%s%s%s\t%d\t%s\t%s\t%ld\t%s\t%s\n", file.type, file.owner, file.group, file.other, file.links, (pws == NULL || pws->pw_name == NULL) ? "deleted-user" : pws->pw_name, (grs == NULL || grs->gr_name == NULL) ? "deleted-group" : grs->gr_name, file.size, time, file.name);
		}
		else
		{
			char *size = SizeBytesToString(file.size);
			printf("%c%s%s%s\t%d\t%s\t%s\t%s\t%s\t%s\n", file.type, file.owner, file.group, file.other, file.links, (pws == NULL || pws->pw_name == NULL) ? "deleted-user" : pws->pw_name, (grs == NULL || grs->gr_name == NULL) ? "deleted-group" : grs->gr_name, size, time, file.name);
			free(size);
		}

		free(time);
	}
	else
	{
		printf("%s    ", file.name);
	}
}

void PrintError(char *folder, char *type)
{
	char buffer[256];
	snprintf(buffer, 256, "ls: '%s' %s error", folder, type);
	perror(buffer);
}

char *PathCombine(char *first, char *second)
{
	int firstlen = strlen(first), secondlen = strlen(second);
	int len = firstlen + 1 + secondlen + 1;
	char *combined = (char *)malloc(len * sizeof(char));

	strcpy(combined, first);

	if (firstlen != 0 && first[firstlen - 1] != '/')
	{
		combined[firstlen] = '/';
		strcpy(combined + firstlen + 1, second);
		combined[len - 1] = '\0';
	}
	else
	{
		strcpy(combined + firstlen, second);
		combined[len - 2] = '\0';
	}

	return combined;
}

int FileCompare(const void *a, const void *b)
{
	if (((File *)a)->time == ((File *)b)->time)
		return 0;
	else
		return ((File *)a)->time < ((File *)b)->time;
}

char *TimeToString(unsigned long timestamp)
{
	time_t current_time = timestamp;
	struct tm *time_info = localtime(&current_time);
	char *timeString = (char *)malloc(256 * sizeof(char));

	if (time_info->tm_year != g_year)
		strftime(timeString, 256, "%d %b %G", time_info);
	else
		strftime(timeString, 256, "%d %b %R", time_info);

	return timeString;
}

char *SizeBytesToString(unsigned long bytes)
{
	if (bytes < 1024)
	{
		char *tab = (char *)malloc(5 * sizeof(char));
		sprintf(tab, "%lu", bytes);
		return tab;
	}
	else
	{
		char type = ' ';
		double converted = (double)bytes;

		while (UpgradeSymbol(&converted, &type))
			;

		converted = Round(converted);
		UpgradeSymbol(&converted, &type);

		char *tab = (char *)malloc(15 * sizeof(char));
		sprintf(tab, "%.1f%c", converted, type);
		return tab;
	}
}

int UpgradeSymbol(double *x, char *type)
{
	if (*x < 1024 || *type == 'P')
		return 0;
	else
		*x = *x / 1024.0;

	switch (*type)
	{
	case ' ':
		*type = 'K';
		break;
	case 'K':
		*type = 'M';
		break;
	case 'M':
		*type = 'G';
		break;
	case 'G':
		*type = 'T';
		break;
	case 'T':
		*type = 'P';
		break;
	}

	return 1;
}

double Round(double value)
{
	return ((int)(10 * value + .5)) / 10.0;
}

void Version()
{
	printf("Custom ls version %s\n\n", VERSION);
	printf("Coded by guzlewski 2019\n");
	exit(0);
}

void Help(char *name)
{
	printf("Usage: %s [OPTIONS] [PATH]\n", name);
	printf("Available options:\n");
	printf("\t-l - display long format\n");
	printf("\t-R - list subdirectories recursively\n");
	printf("\t-t - sort files by last modified date, newest first\n");
	printf("\t-h - print file size in human-readable format (K, M, G, T, P)\n");
	printf("\t--version - print information about version and author\n");
	printf("\t--help - print available commands\n");
	printf("Lists current directory by default.\n");
	exit(0);
}
