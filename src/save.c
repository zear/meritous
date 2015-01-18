//
//   save.c
//
//   Copyright 2007, 2008 Lancer-X/ASCEAI
//
//   This file is part of Meritous.
//
//   Meritous is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Meritous is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Meritous.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <zlib.h>

#include "levelblit.h"
#include "mapgen.h"
#include "demon.h"
#include "tiles.h"

#if defined(WITH_HOME_DIR)
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

char *homeDir = NULL;

gzFile Filefp;
int game_load = 0;

int fpos = 0;

void getHomeDir()
{
#if defined(WITH_HOME_DIR)
	if(homeDir != NULL)
	{
		free(homeDir);
		homeDir = NULL;
	}

	homeDir = (char *)malloc(strlen(getenv("HOME")) + strlen("/.meritous") + 1);
	strcpy(homeDir, getenv("HOME"));
	strcat(homeDir, "/.meritous");
	mkdir(homeDir, 0755); // create $HOME/.meritous if it doesn't exist
#endif
}

void freeHomeDir()
{
	if(homeDir != NULL)
	{
		free(homeDir);
		homeDir == NULL;
	}
}

static inline unsigned char EChar(unsigned char c)
{
	return c ^ 0x55 ^ (fpos & 0xFF);
}

#define DChar EChar

void FWChar(unsigned char i)
{
	gzputc(Filefp, EChar(i));
	fpos++;
}

unsigned char FRChar()
{
	unsigned char c = DChar(gzgetc(Filefp));
	fpos++;
	return c;
}

void FWInt(int val)
{
	unsigned char data[5];
	size_t i;

	data[4] = val < 0;
	if (val < 0) val = -val;

	for (i = 0; i < 4; i++) {
		data[i] = val & 0xFF;
		val >>= 8;
	}

	for (i = 0; i < sizeof(data); i++) {
		data[i] = EChar(data[i]);
		fpos++;
	}
	gzwrite(Filefp, data, sizeof(data));
}

int FRInt()
{
	int val = 0;
	unsigned char data[5];
	size_t i;

	gzread(Filefp, data, sizeof(data));
	for (i = 0; i < sizeof(data); i++) {
		data[i] = DChar(data[i]);
		fpos++;
	}

	for (i = 0; i < 4; i++) {
		val |= data[i] << (i * 8);
	}

	if (data[4] != 0) val = -val;
	return val;
}

void SaveGame(char *filename)
{
	fpos = 0;

	Filefp = gzopen(filename, "wb9");
	FWChar(0x7C);
	WriteMapData();
	WriteCreatureData();
	WritePlayerData();

	gzclose(Filefp);
}

void LoadGame(char *filename)
{
	unsigned char parity;
	fpos = 0;
#if defined(WITH_HOME_DIR)
	char filePath[50];

	if(homeDir == NULL)
	{
		getHomeDir();
	}

	strcpy(filePath, homeDir);
	strcat(filePath, "/");
	strcat(filePath, filename);
	Filefp = gzopen(filePath, "rb");

#else
	Filefp = gzopen(filename, "rb");
#endif
	parity = FRChar();
	if (parity != 0x7C) {
		fprintf(stderr, "Parity byte in error (%x != 0x7C)\nAborting\n", parity);
		exit(2);
	}
	game_load = 1;
}

void CloseFile()
{
	gzclose(Filefp);
}

void DoSaveGame()
{
#if defined(WITH_HOME_DIR)
	char filePath[50];

	if(homeDir == NULL)
	{
		getHomeDir();
	}

	strcpy(filePath, homeDir);
	strcat(filePath, "/SaveFile.sav");
	SavingScreen(0, 0.0);
	SaveGame(filePath);
#else
	SavingScreen(0, 0.0);
	SaveGame("SaveFile.sav");
#endif
}

int IsSaveFile()
{
	FILE *fp;
#if defined(WITH_HOME_DIR)
	char filePath[50];

	if(homeDir == NULL)
	{
		getHomeDir();
	}

	strcpy(filePath, homeDir);
	strcat(filePath, "/SaveFile.sav");

	if ((fp = fopen(filePath, "rb")) != NULL) {
		fclose(fp);
		return 1;
	}
#else
	if ((fp = fopen("SaveFile.sav", "rb")) != NULL) {
		fclose(fp);
		return 1;
	}
#endif
	return 0;
}
